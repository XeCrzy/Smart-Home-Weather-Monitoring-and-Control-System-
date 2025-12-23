#include "client_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>
#include <time.h>
#include "include/util/LogUtil.h"
#include "include/third_party/cjson/cJSON.h"
#include "include/util/JSONUtil.h"
// 内部状态
static client_c_config_t client_config = {
    .server_ip = SERVER_IP,
    .server_port = SERVER_PORT,
    .client_id = CLIENT_C_ID,
    .weather_callback = NULL,
    .command_callback = NULL,
    .status_callback = NULL
};

static int tcp_socket = -1;
static pthread_t tcp_thread = 0;
static volatile bool tcp_running = false;
static volatile client_c_state_t client_state = CLIENT_C_DISCONNECTED;
static pthread_mutex_t tcp_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool debug_mode = false;

// 内部函数声明
static void* tcp_client_thread_func(void* arg);
static bool tcp_connect_to_server(void);
static void tcp_disconnect(void);
static bool tcp_send_identity(void);
static bool tcp_receive_message(char* buffer, int buffer_size, int timeout_ms);
static void update_state(client_c_state_t new_state);
static void parse_and_handle_message(const char* message);
static command_type_t parse_command_type(const char* message);

// 初始化客户端C
bool client_c_init(client_c_config_t* config) {
    if (config == NULL) {
        printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 配置不能为NULL\n");
        return false;
    }
    
    // 复制配置
    if (config->server_ip) {
        client_config.server_ip = config->server_ip;
    }
    if (config->server_port > 0) {
        client_config.server_port = config->server_port;
    }
    if (config->client_id) {
        client_config.client_id = config->client_id;
    }
    if (config->weather_callback) {
        client_config.weather_callback = config->weather_callback;
    }
    if (config->command_callback) {
        client_config.command_callback = config->command_callback;
    }
    if (config->status_callback) {
        client_config.status_callback = config->status_callback;
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 初始化成功\n");
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 服务器地址: %s:%d\n", 
              client_config.server_ip, client_config.server_port);
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 客户端ID: %s\n", client_config.client_id);
    
    return true;
}

// 启动客户端C
bool client_c_start(void) {
    if (tcp_running) {
        printfLog(EN_LOG_LEVEL_WARNING, "Client_C: 已经在运行\n");
        return true;
    }
    
    update_state(CLIENT_C_CONNECTING);
    tcp_running = true;
    
    // 创建TCP客户端线程
    int ret = pthread_create(&tcp_thread, NULL, tcp_client_thread_func, NULL);
    if (ret != 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 创建线程失败: %s\n", strerror(ret));
        tcp_running = false;
        update_state(CLIENT_C_ERROR);
        return false;
    }
    
    // 分离线程，使其在退出时自动释放资源
    pthread_detach(tcp_thread);
    
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 客户端线程已启动\n");
    return true;
}

// 停止客户端C
void client_c_stop(void) {
    if (!tcp_running) {
        return;
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 正在停止...\n");
    tcp_running = false;
    
    // 关闭socket以唤醒阻塞的recv
    if (tcp_socket >= 0) {
        shutdown(tcp_socket, SHUT_RDWR);
        close(tcp_socket);
        tcp_socket = -1;
    }
    
    update_state(CLIENT_C_DISCONNECTED);
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 已停止\n");
}

// TCP客户端线程函数
static void* tcp_client_thread_func(void* arg) {
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: TCP客户端线程启动\n");
    
    // 连接重试机制
    int retry_count = 0;
    const int max_retries = 10;
    
    while (tcp_running) {
        if (tcp_connect_to_server()) {
            retry_count = 0; // 连接成功，重置重试计数
            
            // 发送身份标识
            if (!tcp_send_identity()) {
                printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 发送身份标识失败\n");
                tcp_disconnect();
                continue;
            }
            
            // 主消息循环
            char buffer[4096];
            while (tcp_running && tcp_socket >= 0) {
                memset(buffer, 0, sizeof(buffer));
                
                if (tcp_receive_message(buffer, sizeof(buffer), 5000)) {
                    // 处理接收到的消息
                    parse_and_handle_message(buffer);
                } else {
                    // 接收超时或错误
                    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                        // 超时或中断，继续循环
                        continue;
                    } else {
                        printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 接收消息错误: %s\n", strerror(errno));
                        break;
                    }
                }
            }
        } else {
            retry_count++;
            if (retry_count < max_retries) {
                printfLog(EN_LOG_LEVEL_WARNING, "Client_C: 连接失败，%d秒后重试 (%d/%d)\n", 
                          retry_count, retry_count, max_retries);
                sleep(retry_count); // 指数退避
            } else {
                printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 达到最大重试次数\n");
                update_state(CLIENT_C_ERROR);
                break;
            }
        }
        
        // 断开连接
        tcp_disconnect();
        
        // 如果还在运行状态，等待一会儿再重连
        if (tcp_running) {
            sleep(2);
        }
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: TCP客户端线程退出\n");
    return NULL;
}

// 连接到服务器
static bool tcp_connect_to_server(void) {
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 正在连接到服务器 %s:%d\n", 
              client_config.server_ip, client_config.server_port);
    
    // 创建socket
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 创建socket失败: %s\n", strerror(errno));
        return false;
    }
    
    // 设置socket选项
    int opt = 1;
    setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 设置接收超时
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(tcp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // 准备服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(client_config.server_port);
    
    if (inet_pton(AF_INET, client_config.server_ip, &server_addr.sin_addr) <= 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 无效的IP地址\n");
        close(tcp_socket);
        tcp_socket = -1;
        return false;
    }
    
    // 连接服务器
    if (connect(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 连接失败: %s\n", strerror(errno));
        close(tcp_socket);
        tcp_socket = -1;
        return false;
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 连接服务器成功\n");
    update_state(CLIENT_C_CONNECTED);
    return true;
}

// 断开连接
static void tcp_disconnect(void) {
    if (tcp_socket >= 0) {
        close(tcp_socket);
        tcp_socket = -1;
        update_state(CLIENT_C_DISCONNECTED);
        printfLog(EN_LOG_LEVEL_INFO, "Client_C: 已断开连接\n");
    }
}

// 发送身份标识
static bool tcp_send_identity(void) {
    if (tcp_socket < 0) {
        return false;
    }
    
    // 发送身份标识 "CLIENT_C"
    pthread_mutex_lock(&tcp_mutex);
    int bytes_sent = send(tcp_socket, "CLIENT_C", 8, 0);
    pthread_mutex_unlock(&tcp_mutex);
    
    if (bytes_sent < 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 发送身份标识失败: %s\n", strerror(errno));
        return false;
    } else {
        printfLog(EN_LOG_LEVEL_INFO, "Client_C: 已发送身份标识: CLIENT_C\n");
        return true;
    }
}

// 接收消息（带超时）
static bool tcp_receive_message(char* buffer, int buffer_size, int timeout_ms) {
    if (tcp_socket < 0 || buffer == NULL || buffer_size <= 0) {
        return false;
    }
    
    // 设置接收超时
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(tcp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    int bytes_received = recv(tcp_socket, buffer, buffer_size - 1, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        // 去除可能的换行符
        if (buffer[bytes_received - 1] == '\n') {
            buffer[bytes_received - 1] = '\0';
        }
        
        return true;
    } else if (bytes_received == 0) {
        printfLog(EN_LOG_LEVEL_WARNING, "Client_C: 服务器关闭连接\n");
        return false;
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 接收数据错误: %s\n", strerror(errno));
        }
        return false;
    }
}

// 解析命令类型
static command_type_t parse_command_type(const char* message) {
    if (message == NULL) {
        return CMD_UNKNOWN;
    }
    
    if (strstr(message, "LED_ON") || strstr(message, "led_on")) {
        return CMD_LED_ON;
    } else if (strstr(message, "LED_OFF") || strstr(message, "led_off")) {
        return CMD_LED_OFF;
    } else if (strstr(message, "BUZZER_ON") || strstr(message, "buzzer_on")) {
        return CMD_BUZZER_ON;
    } else if (strstr(message, "BUZZER_OFF") || strstr(message, "buzzer_off")) {
        return CMD_BUZZER_OFF;
    }
    
    return CMD_UNKNOWN;
}

// 解析和处理消息
static void parse_and_handle_message(const char* message) {
    if (message == NULL || strlen(message) == 0) {
        return;
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 收到消息: %s\n", message);
    
    // 检查是否是服务器确认
    if (strstr(message, "CONNECTED") || strstr(message, "connected")) {
        printfLog(EN_LOG_LEVEL_INFO, "Client_C: 服务器确认连接\n");
        return;
    }
    
    // 检查是否是命令
    command_type_t cmd_type = parse_command_type(message);
    if (cmd_type != CMD_UNKNOWN) {
        // 写入命令FIFO
        if (access(COMMAND_FIFO, F_OK) == 0) {
            int fifo_fd = open(COMMAND_FIFO, O_WRONLY | O_NONBLOCK);
            if (fifo_fd >= 0) {
                write(fifo_fd, message, strlen(message));
                write(fifo_fd, "\n", 1);
                close(fifo_fd);
                printfLog(EN_LOG_LEVEL_INFO, "Client_C: 已将命令写入FIFO: %s\n", message);
            }
        }
        
        // 调用命令回调函数
        if (client_config.command_callback) {
            client_config.command_callback(cmd_type);
        }
        return;
    }
    
    // 检查是否是天气数据（包含中文关键词）
    if (strstr(message, "城市") || strstr(message, "天气") || 
        strstr(message, "温度") || strstr(message, "湿度")) {
        
        // 解析文本格式的天气数据
        char city[64] = "Unknown";
        char weather[64] = "Unknown";
        char temperature[32] = "25.0";
        char humidity[32] = "60";
        
        // 按行分割消息
        char *message_copy = strdup(message);
        char *line = strtok(message_copy, "\n");
        
        while (line != NULL) {
            // 去除首尾空白字符
            char *trimmed = line;
            while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
            
            // 查找冒号分隔符
            char *colon = strstr(trimmed, ":");
            if (colon) {
                *colon = '\0';
                char *key = trimmed;
                char *value = colon + 1;
                
                // 去除value的首尾空白
                while (*value == ' ' || *value == '\t') value++;
                char *end = value + strlen(value) - 1;
                while (end > value && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
                    *end = '\0';
                    end--;
                }
                
                // 根据key存储对应的value
                if (strstr(key, "城市")) {
                    strncpy(city, value, sizeof(city) - 1);
                } else if (strstr(key, "天气")) {
                    strncpy(weather, value, sizeof(weather) - 1);
                } else if (strstr(key, "温度")) {
                    // 提取温度数字部分（去除单位）
                    char temp_num[32] = {0};
                    int i = 0, j = 0;
                    for (i = 0; value[i] != '\0' && j < 30; i++) {
                        if ((value[i] >= '0' && value[i] <= '9') || 
                            value[i] == '.' || value[i] == '-') {
                            temp_num[j++] = value[i];
                        }
                    }
                    temp_num[j] = '\0';
                    if (strlen(temp_num) > 0) {
                        strncpy(temperature, temp_num, sizeof(temperature) - 1);
                    }
                } else if (strstr(key, "湿度")) {
                    // 提取湿度数字部分
                    char hum_num[32] = {0};
                    int i = 0, j = 0;
                    for (i = 0; value[i] != '\0' && j < 30; i++) {
                        if ((value[i] >= '0' && value[i] <= '9') || 
                            value[i] == '.' || value[i] == '-') {
                            hum_num[j++] = value[i];
                        }
                    }
                    hum_num[j] = '\0';
                    if (strlen(hum_num) > 0) {
                        strncpy(humidity, hum_num, sizeof(humidity) - 1);
                        strcat(humidity, "%");
                    } else if (strstr(value, "N/A")) {
                        strcpy(humidity, "N/A");
                    }
                }
            }
            line = strtok(NULL, "\n");
        }
        
        free(message_copy);
        
        // 构建JSON格式的天气数据
        char json_buffer[1024];
        snprintf(json_buffer, sizeof(json_buffer),
                "{\"city\":\"%s\",\"weather\":\"%s\",\"temperature\":\"%s\",\"humidity\":\"%s\"}",
                city, weather, temperature, humidity);
        
        printfLog(EN_LOG_LEVEL_INFO, "Client_C: 解析后天气数据: %s\n", json_buffer);
        
        // 写入天气FIFO
        if (access(WEATHER_FIFO, F_OK) == 0) {
            int fifo_fd = open(WEATHER_FIFO, O_WRONLY | O_NONBLOCK);
            if (fifo_fd >= 0) {
                write(fifo_fd, json_buffer, strlen(json_buffer));
                write(fifo_fd, "\n", 1);
                close(fifo_fd);
                printfLog(EN_LOG_LEVEL_INFO, "Client_C: 已将天气数据写入FIFO\n");
            }
        }
        
        // 调用天气回调函数
        if (client_config.weather_callback) {
            client_config.weather_callback(json_buffer);
        }
        return;
    }
    
    // 其他类型消息
    if (debug_mode) {
        printfLog(EN_LOG_LEVEL_DEBUG, "Client_C: 无法识别的消息: %s\n", message);
    }
}

// 发送命令到服务器
bool client_c_send_command(const char* command) {
    if (!client_c_is_connected() || command == NULL) {
        printfLog(EN_LOG_LEVEL_WARNING, "Client_C: 无法发送命令，未连接或命令为空\n");
        return false;
    }
    
    pthread_mutex_lock(&tcp_mutex);
    int bytes_sent = send(tcp_socket, command, strlen(command), 0);
    pthread_mutex_unlock(&tcp_mutex);
    
    if (bytes_sent < 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "Client_C: 发送命令失败: %s\n", strerror(errno));
        return false;
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "Client_C: 已发送命令: %s\n", command);
    return true;
}

// 发送消息到服务器（带类型）
bool client_c_send_message(msg_type_t type, const char* data) {
    if (!client_c_is_connected() || data == NULL) {
        return false;
    }
    
    // 构建带类型的消息
    char buffer[2048];
    const char* type_str = "";
    
    switch (type) {
        case MSG_TYPE_WEATHER: type_str = "WEATHER:"; break;
        case MSG_TYPE_COMMAND: type_str = "COMMAND:"; break;
        case MSG_TYPE_STATUS: type_str = "STATUS:"; break;
        case MSG_TYPE_IDENTITY: type_str = "IDENTITY:"; break;
        default: type_str = "UNKNOWN:"; break;
    }
    
    snprintf(buffer, sizeof(buffer), "%s%s", type_str, data);
    return client_c_send_command(buffer);
}

// 解析天气JSON函数定义
bool parse_weather_json(const char* json_str, char* city, char* weather, char* temperature, char* humidity) {
    if (!json_str || !city || !weather || !temperature || !humidity) {
        return false;
    }
    
    // 直接使用cJSON解析
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        return false;
    }
    
    // 获取城市
    cJSON *city_obj = cJSON_GetObjectItem(root, "city");
    if (city_obj && cJSON_IsString(city_obj)) {
        strncpy(city, city_obj->valuestring, 63);
        city[63] = '\0';
    } else {
        strcpy(city, "Unknown");
    }
    
    // 获取天气
    cJSON *weather_obj = cJSON_GetObjectItem(root, "weather");
    if (weather_obj && cJSON_IsString(weather_obj)) {
        strncpy(weather, weather_obj->valuestring, 63);
        weather[63] = '\0';
    } else {
        strcpy(weather, "Unknown");
    }
    
    // 获取温度
    cJSON *temp_obj = cJSON_GetObjectItem(root, "temperature");
    if (temp_obj) {
        if (cJSON_IsString(temp_obj)) {
            strncpy(temperature, temp_obj->valuestring, 31);
            temperature[31] = '\0';
        } else if (cJSON_IsNumber(temp_obj)) {
            snprintf(temperature, 31, "%.1f", temp_obj->valuedouble);
        } else {
            strcpy(temperature, "25.0");
        }
    } else {
        strcpy(temperature, "25.0");
    }
    
    // 获取湿度
    cJSON *hum_obj = cJSON_GetObjectItem(root, "humidity");
    if (hum_obj) {
        if (cJSON_IsString(hum_obj)) {
            strncpy(humidity, hum_obj->valuestring, 31);
            humidity[31] = '\0';
        } else if (cJSON_IsNumber(hum_obj)) {
            snprintf(humidity, 31, "%.0f%%", hum_obj->valuedouble);
        } else {
            strcpy(humidity, "60%");
        }
    } else {
        strcpy(humidity, "60%");
    }
    
    cJSON_Delete(root);
    return true;
}


// 获取客户端C当前状态
client_c_state_t client_c_get_state(void) {
    return client_state;
}

// 检查是否连接
bool client_c_is_connected(void) {
    return (client_state == CLIENT_C_CONNECTED && tcp_socket >= 0);
}

// 设置调试模式
void client_c_set_debug(bool enable) {
    debug_mode = enable;
}

// 更新状态并回调
static void update_state(client_c_state_t new_state) {
    client_state = new_state;
    
    if (client_config.status_callback) {
        client_config.status_callback(new_state);
    }
    
    if (debug_mode) {
        const char* state_str = "";
        switch (new_state) {
            case CLIENT_C_DISCONNECTED: state_str = "断开连接"; break;
            case CLIENT_C_CONNECTING: state_str = "连接中"; break;
            case CLIENT_C_CONNECTED: state_str = "已连接"; break;
            case CLIENT_C_ERROR: state_str = "错误"; break;
        }
        printfLog(EN_LOG_LEVEL_DEBUG, "Client_C: 状态更新: %s\n", state_str);
    }
}