#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#if defined(WIN32) || defined(WIN64)
#include "windows.h"
#else
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "include/agentlite/hw_type.h"
#include "include/agentlite/iota_init.h"
#include "include/agentlite/iota_cfg.h"
#include "include/agentlite/iota_login.h"
#include "include/agentlite/iota_datatrans.h"
#include "include/agentlite/iota_error_type.h"

#include "include/util/LogUtil.h"
#include "include/util/StringUtil.h"
#include "include/third_party/cjson/cJSON.h"

// 包含客户端C头文件
#include "client_c.h"

// 全局变量
char* workPath = ".";
int alarmValue = 0;
int led_status = 0;
int buzzer_status = 0;

char* serverIp_ = "";
int port_ = 8883;  // 使用SSL端口
char* username_ = NULL;
char* password_ = NULL;

int disconnected_ = 0;
char *subDeviceId = "f6cd4bbb1a8ab53acbb595efd0e90199_ABC123456789";
int sleepTime = 5000;

// 客户端C相关变量
static bool client_c_initialized = false;
static pthread_t command_handler_thread = 0;
static volatile bool command_handler_running = false;
static pthread_t fifo_listener_thread_id = 0;
static volatile bool fifo_listener_running = false;
static int weather_fifo_fd = -1;

// 函数声明
void timeSleep(int ms);
void setConnectConfig(void);
void setAuthConfig(void);
void setMyCallbacks(void);
void myPrintLog(int level, char* format, va_list args);

// 测试函数
void Test_messageReport(void);
void Test_propertiesReport(void);
void Test_batchPropertiesReport(void);
void Test_commandResponse(char *requestId);
void Test_propSetResponse(char *requestId);
void Test_propGetResponse(char *requestId);

// 回调函数
void handleAuthSuccess(void* context, int messageId, int code, char *message);
void handleAuthFailure(void* context, int messageId, int code, char *message);
void handleConnectionLost(void* context, int messageId, int code, char *message);
void handleDisAuthSuccess(void* context, int messageId, int code, char *message);
void handleDisAuthFailure(void* context, int messageId, int code, char *message);
void handleSubscribesuccess(void* context, int messageId, int code, char *message);
void handleSubscribeFailure(void* context, int messageId, int code, char *message);
void handlePublishSuccess(void* context, int messageId, int code, char *message);
void handlePublishFailure(void* context, int messageId, int code, char *message);
void handleMessageDown(void* context, int messageId, int code, char *message);
void handleCommandRequest(void* context, int messageId, int code, char *message, char *requestId);
void handlePropertiesSet(void* context, int messageId, int code, char *message, char *requestId);
void handlePropertiesGet(void* context, int messageId, int code, char *message, char *requestId);
void handleEventsDown(void* context, int messageId, int code, char *message);

// 新增函数声明
static void* fifo_listener_thread(void* arg);
static void* command_handler_thread_func(void* arg);
static void client_c_status_callback(client_c_state_t state);
static bool write_command_to_fifo(const char* command);
static void start_fifo_listener(void);
static void start_command_handler_thread(void);
static void stop_command_handler_thread(void);
static void stop_fifo_listener(void);
char* read_weather_from_fifo(void);
void deleteSubStr(char *str, const char *substr);

// 时间睡眠函数
void timeSleep(int ms)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

// 删除子字符串
void deleteSubStr(char *str, const char *substr)
{
    if (!str || !substr) return;
    
    char *p = strstr(str, substr);
    if (p) {
        size_t len = strlen(substr);
        memmove(p, p + len, strlen(p + len) + 1);
    }
}

// 从FIFO读取天气数据
char* read_weather_from_fifo()
{
    static char weather_json[1024] = {0};
    static char last_weather_json[1024] = {0};
    
    memset(weather_json, 0, sizeof(weather_json));
    
    // 检查FIFO是否存在
    if (access(WEATHER_FIFO, F_OK) != 0) {
        if (strlen(last_weather_json) > 0) {
            return last_weather_json;
        }
        return NULL;
    }
    
    // 使用阻塞模式打开，确保能读到数据
    int fd = open(WEATHER_FIFO, O_RDONLY);
    if (fd < 0) {
        if (strlen(last_weather_json) > 0) {
            return last_weather_json;
        }
        return NULL;
    }
    
    // 读取数据
    int bytes = read(fd, weather_json, sizeof(weather_json) - 1);
    close(fd);
    
    if (bytes > 0) {
        weather_json[bytes] = '\0';
        // 去除可能的换行符
        char* newline = strchr(weather_json, '\n');
        if (newline) {
            *newline = '\0';
        }
        
        printfLog(EN_LOG_LEVEL_INFO, "从FIFO读取到天气数据: %s\n", weather_json);
        
        // 保存为上次数据
        strcpy(last_weather_json, weather_json);
        return weather_json;
    } else if (bytes == 0) {
        // 没有新数据，返回上次的数据
        if (strlen(last_weather_json) > 0) {
            return last_weather_json;
        }
    }
    
    return NULL;
}

// FIFO监听线程函数
static void* fifo_listener_thread(void* arg)
{
    printfLog(EN_LOG_LEVEL_INFO, "FIFO监听线程启动\n");
    
    // 确保FIFO目录存在
    struct stat st;
    if (stat(FIFO_BASE_PATH, &st) != 0) {
        mkdir(FIFO_BASE_PATH, 0777);
    }
    
    // 确保FIFO存在
    if (access(WEATHER_FIFO, F_OK) != 0) {
        mkfifo(WEATHER_FIFO, 0666);
    }
    
    weather_fifo_fd = open(WEATHER_FIFO, O_RDONLY);
    if (weather_fifo_fd < 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "打开天气FIFO失败: %s\n", strerror(errno));
        return NULL;
    }
    
    char buffer[1024];
    
    while (fifo_listener_running) {
        memset(buffer, 0, sizeof(buffer));
        
        int bytes = read(weather_fifo_fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            if (buffer[strlen(buffer)-1] == '\n') {
                buffer[strlen(buffer)-1] = '\0';
            }
            
            printfLog(EN_LOG_LEVEL_INFO, "FIFO监听线程收到天气数据: %s\n", buffer);
        } else if (bytes == 0) {
            // FIFO写入端关闭，短暂等待后继续
            timeSleep(100);
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printfLog(EN_LOG_LEVEL_ERROR, "读取FIFO失败: %s\n", strerror(errno));
                break;
            }
        }
    }
    
    if (weather_fifo_fd >= 0) {
        close(weather_fifo_fd);
        weather_fifo_fd = -1;
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "FIFO监听线程退出\n");
    return NULL;
}

// 命令处理线程函数
static void* command_handler_thread_func(void* arg)
{
    printfLog(EN_LOG_LEVEL_INFO, "命令处理线程启动\n");
    
    // 确保FIFO目录存在
    struct stat st;
    if (stat(FIFO_BASE_PATH, &st) != 0) {
        mkdir(FIFO_BASE_PATH, 0777);
    }
    
    // 确保command_fifo存在
    if (access(COMMAND_FIFO, F_OK) != 0) {
        mkfifo(COMMAND_FIFO, 0666);
    }
    
    int command_fifo_fd = open(COMMAND_FIFO, O_RDONLY);
    if (command_fifo_fd < 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "打开command_fifo失败: %s\n", strerror(errno));
        return NULL;
    }
    
    char buffer[1024];
    
    while (command_handler_running) {
        memset(buffer, 0, sizeof(buffer));
        
        int bytes = read(command_fifo_fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            if (buffer[strlen(buffer)-1] == '\n') {
                buffer[strlen(buffer)-1] = '\0';
            }
            
            printfLog(EN_LOG_LEVEL_INFO, "从command_fifo收到命令: %s\n", buffer);
            
            // 通过TCP发送命令到服务器
            if (client_c_initialized && client_c_is_connected()) {
                if (client_c_send_command(buffer)) {
                    printfLog(EN_LOG_LEVEL_INFO, "命令已发送到服务器\n");
                } else {
                    printfLog(EN_LOG_LEVEL_ERROR, "发送命令到服务器失败\n");
                }
            } else {
                printfLog(EN_LOG_LEVEL_WARNING, "客户端C未连接，无法发送命令\n");
            }
        } else if (bytes == 0) {
            timeSleep(100);
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printfLog(EN_LOG_LEVEL_ERROR, "读取command_fifo失败: %s\n", strerror(errno));
                break;
            }
        }
    }
    
    if (command_fifo_fd >= 0) {
        close(command_fifo_fd);
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "命令处理线程退出\n");
    return NULL;
}

// 客户端C状态回调函数
static void client_c_status_callback(client_c_state_t state)
{
    const char* state_str = "";
    switch (state) {
        case CLIENT_C_DISCONNECTED: state_str = "断开连接"; break;
        case CLIENT_C_CONNECTING: state_str = "连接中"; break;
        case CLIENT_C_CONNECTED: state_str = "已连接"; break;
        case CLIENT_C_ERROR: state_str = "错误"; break;
    }
    printfLog(EN_LOG_LEVEL_INFO, "客户端C状态: %s\n", state_str);
}

// 写入命令到FIFO
static bool write_command_to_fifo(const char* command)
{
    if (command == NULL) return false;
    
    // 确保FIFO目录存在
    struct stat st;
    if (stat(FIFO_BASE_PATH, &st) != 0) {
        mkdir(FIFO_BASE_PATH, 0777);
    }
    
    // 确保command_fifo存在
    if (access(COMMAND_FIFO, F_OK) != 0) {
        mkfifo(COMMAND_FIFO, 0666);
    }
    
    int fifo_fd = open(COMMAND_FIFO, O_WRONLY | O_NONBLOCK);
    if (fifo_fd < 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "打开command_fifo失败: %s\n", strerror(errno));
        return false;
    }
    
    int bytes_written = write(fifo_fd, command, strlen(command));
    if (bytes_written > 0) {
        write(fifo_fd, "\n", 1);
    }
    close(fifo_fd);
    
    if (bytes_written < 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "写入command_fifo失败: %s\n", strerror(errno));
        return false;
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "命令已写入command_fifo: %s\n", command);
    return true;
}

// 启动FIFO监听线程
static void start_fifo_listener(void)
{
    if (fifo_listener_running) return;
    
    fifo_listener_running = true;
    pthread_create(&fifo_listener_thread_id, NULL, fifo_listener_thread, NULL);
    printfLog(EN_LOG_LEVEL_INFO, "FIFO监听线程已启动\n");
}

// 启动命令处理线程
static void start_command_handler_thread(void)
{
    if (command_handler_running) return;
    
    command_handler_running = true;
    pthread_create(&command_handler_thread, NULL, command_handler_thread_func, NULL);
    printfLog(EN_LOG_LEVEL_INFO, "命令处理线程已启动\n");
}

// 停止命令处理线程
static void stop_command_handler_thread(void)
{
    if (!command_handler_running) return;
    
    printfLog(EN_LOG_LEVEL_INFO, "正在停止命令处理线程...\n");
    command_handler_running = false;
    if (command_handler_thread) {
        pthread_join(command_handler_thread, NULL);
        command_handler_thread = 0;
    }
    printfLog(EN_LOG_LEVEL_INFO, "命令处理线程已停止\n");
}

// 停止FIFO监听线程
static void stop_fifo_listener(void)
{
    if (!fifo_listener_running) return;
    
    printfLog(EN_LOG_LEVEL_INFO, "正在停止FIFO监听线程...\n");
    fifo_listener_running = false;
    if (weather_fifo_fd >= 0) {
        close(weather_fifo_fd);
        weather_fifo_fd = -1;
    }
    if (fifo_listener_thread_id) {
        pthread_join(fifo_listener_thread_id, NULL);
        fifo_listener_thread_id = 0;
    }
    printfLog(EN_LOG_LEVEL_INFO, "FIFO监听线程已停止\n");
}

// ==================== 华为云IoT功能函数 ====================

// 从配置文件读取配置
void setConnectConfig(void)
{
    FILE *file = fopen("./ClientConf.json", "rb");
    if (!file) {
        printfLog(EN_LOG_LEVEL_ERROR, "无法打开配置文件 ClientConf.json\n");
        return;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = (char*)malloc(length + 1);
    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);
    
    // 使用cJSON_Parse替代JSON_Parse
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        printfLog(EN_LOG_LEVEL_ERROR, "解析配置文件失败\n");
        free(content);
        return;
    }
    
    // 读取设备ID
    cJSON *deviceId = cJSON_GetObjectItem(json, "deviceId");
    if (deviceId && cJSON_IsString(deviceId)) {
        username_ = strdup(deviceId->valuestring);
        printfLog(EN_LOG_LEVEL_INFO, "设备ID: %s\n", username_);
    }
    
    // 读取设备密钥
    cJSON *secret = cJSON_GetObjectItem(json, "secret");
    if (secret && cJSON_IsString(secret)) {
        password_ = strdup(secret->valuestring);
        printfLog(EN_LOG_LEVEL_INFO, "设备密钥: %s\n", password_);
    }
    
    // 读取服务器地址
    cJSON *serverUri = cJSON_GetObjectItem(json, "serverUri");
    if (serverUri && cJSON_IsString(serverUri)) {
        char *uri = strdup(serverUri->valuestring);
        // 去除 ssl:// 前缀和 :8883 端口
        if (strstr(uri, "ssl://") == uri) {
            serverIp_ = uri + 6; // 跳过 "ssl://"
        } else {
            serverIp_ = uri;
        }
        
        // 查找并移除端口
        char *port_ptr = strrchr(serverIp_, ':');
        if (port_ptr) {
            *port_ptr = '\0';
        }
        
        printfLog(EN_LOG_LEVEL_INFO, "服务器地址: %s\n", serverIp_);
        // 注意：这里我们没有释放uri，因为serverIp_指向它的一部分
    }
    
    cJSON_Delete(json);
    free(content);
}


// 设置认证配置
void setAuthConfig(void)
{
    if (!serverIp_ || !username_ || !password_) {
        printfLog(EN_LOG_LEVEL_ERROR, "认证配置参数为空\n");
        return;
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "设置MQTT配置: 地址=%s, 端口=%d, 设备ID=%s\n", 
              serverIp_, port_, username_);
    
    IOTA_ConfigSetStr(EN_IOTA_CFG_MQTT_ADDR, serverIp_);
    IOTA_ConfigSetUint(EN_IOTA_CFG_MQTT_PORT, port_);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICEID, username_);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICESECRET, password_);
    
    // 设置连接参数
    IOTA_ConfigSetUint(EN_IOTA_CFG_KEEP_ALIVE_TIME, 120);
    IOTA_ConfigSetUint(EN_IOTA_CFG_CONNECT_TIMEOUT, 30);
    IOTA_ConfigSetUint(EN_IOTA_CFG_RETRY_INTERVAL, 10);
    IOTA_ConfigSetUint(EN_IOTA_CFG_QOS, 1);
}

// 设置回调函数
void setMyCallbacks(void)
{
    IOTA_SetCallback(EN_IOTA_CALLBACK_CONNECT_SUCCESS, handleAuthSuccess);
    IOTA_SetCallback(EN_IOTA_CALLBACK_CONNECT_FAILURE, handleAuthFailure);
    IOTA_SetCallback(EN_IOTA_CALLBACK_CONNECTION_LOST, handleConnectionLost);
    IOTA_SetCallback(EN_IOTA_CALLBACK_DISCONNECT_SUCCESS, handleDisAuthSuccess);
    IOTA_SetCallback(EN_IOTA_CALLBACK_DISCONNECT_FAILURE, handleDisAuthFailure);
    IOTA_SetCallback(EN_IOTA_CALLBACK_SUBSCRIBE_SUCCESS, handleSubscribesuccess);
    IOTA_SetCallback(EN_IOTA_CALLBACK_SUBSCRIBE_FAILURE, handleSubscribeFailure);
    IOTA_SetCallback(EN_IOTA_CALLBACK_PUBLISH_SUCCESS, handlePublishSuccess);
    IOTA_SetCallback(EN_IOTA_CALLBACK_PUBLISH_FAILURE, handlePublishFailure);
    IOTA_SetCallback(EN_IOTA_CALLBACK_MESSAGE_DOWN, handleMessageDown);
    IOTA_SetCallbackWithTopic(EN_IOTA_CALLBACK_COMMAND_REQUEST, handleCommandRequest);
    IOTA_SetCallbackWithTopic(EN_IOTA_CALLBACK_PROPERTIES_SET, handlePropertiesSet);
    IOTA_SetCallbackWithTopic(EN_IOTA_CALLBACK_PROPERTIES_GET, handlePropertiesGet);
    IOTA_SetCallback(EN_IOTA_CALLBACK_SUB_DEVICE_MESSAGE_DOWN, handleEventsDown);
}

// 日志回调函数
void myPrintLog(int level, char* format, va_list args)
{
    vprintf(format, args);
    fflush(stdout);
}

// ==================== 测试函数实现 ====================

void Test_messageReport()
{
    int messageId = IOTA_MessageReport(NULL, "data123", "123", "hello");
    if(messageId != 0)
    {
        printfLog(EN_LOG_LEVEL_ERROR, "Test_messageReport() failed, messageId %d\n", messageId);
    }
}

void Test_propertiesReport()
{
    // 创建3个服务
    int serviceNum = 3;
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];

    char city[64] = "Unknown";
    char weather[64] = "Unknown";
    char temperature_str[32] = "25.5";
    char humidity_str[32] = "60";
    // 1. 烟感检测服务 - 使用cJSON
    cJSON *smokeRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(smokeRoot, "alarm", alarmValue);
    cJSON_AddNumberToObject(smokeRoot, "smokeConcentration", (double)(rand()%900+100)/10);
    cJSON_AddStringToObject(smokeRoot, "temperature", temperature_str);
    cJSON_AddNumberToObject(smokeRoot, "humidity", rand() % 100);
    
    char *smokePayload = cJSON_Print(smokeRoot);
    cJSON_Delete(smokeRoot);
    
    services[0].event_time = getEventTimeStamp();
    services[0].service_id = "smokeDetector";
    services[0].properties = smokePayload;
    
    // 2. 天气服务
    cJSON *weatherRoot = cJSON_CreateObject();
    

    
    // 尝试从FIFO读取天气数据
    char* weather_json = read_weather_from_fifo();
    if (weather_json) {
        // 使用parse_weather_json函数（已在client_c.c中定义）
        if (parse_weather_json(weather_json, city, weather, temperature_str, humidity_str)) {
            printfLog(EN_LOG_LEVEL_INFO, "使用FIFO天气数据: 城市=%s, 天气=%s, 温度=%s, 湿度=%s\n", 
                   city, weather, temperature_str, humidity_str);
        }
    }
    
    cJSON_AddStringToObject(weatherRoot, "city", city);
    cJSON_AddStringToObject(weatherRoot, "weather", weather);
    cJSON_AddStringToObject(weatherRoot, "temperature", temperature_str);
    cJSON_AddStringToObject(weatherRoot, "humidity", humidity_str);
    cJSON_AddStringToObject(weatherRoot, "wind_direction", "North");
    
    char *weatherPayload = cJSON_Print(weatherRoot);
    cJSON_Delete(weatherRoot);
    
    services[1].event_time = getEventTimeStamp();
    services[1].service_id = "Weather";
    services[1].properties = weatherPayload;
    
    // 3. 控制服务
    cJSON *controlRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(controlRoot, "led_status", led_status);
    cJSON_AddNumberToObject(controlRoot, "buzzer_status", buzzer_status);
    
    char *controlPayload = cJSON_Print(controlRoot);
    cJSON_Delete(controlRoot);
    
    services[2].event_time = getEventTimeStamp();
    services[2].service_id = "Control";
    services[2].properties = controlPayload;
    
    // 上报所有服务
    int messageId = IOTA_PropertiesReport(services, serviceNum);
    if(messageId != 0)
    {
        printfLog(EN_LOG_LEVEL_ERROR, "Test_propertiesReport() failed, messageId %d\n", messageId);
    }
    else
    {
        printfLog(EN_LOG_LEVEL_INFO, "所有服务上报成功\n");
    }
    
    // 释放内存
    // 注意：cJSON_Print返回的字符串需要释放
    if (smokePayload) free(smokePayload);
    if (weatherPayload) free(weatherPayload);
    if (controlPayload) free(controlPayload);
}

void Test_batchPropertiesReport()
{
    // 简化的批量属性上报
    printfLog(EN_LOG_LEVEL_INFO, "Test_batchPropertiesReport()\n");
}

void Test_commandResponse(char *requestId)
{
    char *pcCommandRespense = "{\"result\":\"success\"}";
    int result_code = 0;
    char *response_name = "command_response";
    
    int messageId = IOTA_CommandResponse(requestId, result_code, response_name, pcCommandRespense);
    if(messageId != 0)
    {
        printfLog(EN_LOG_LEVEL_ERROR, "Test_commandResponse() failed, messageId %d\n", messageId);
    }
}

void Test_propSetResponse(char *requestId)
{
    int messageId = IOTA_PropertiesSetResponse(requestId, 0, "success");
    if(messageId != 0)
    {
        printfLog(EN_LOG_LEVEL_ERROR, "Test_propSetResponse() failed, messageId %d\n", messageId);
    }
}

void Test_propGetResponse(char *requestId)
{
    // 简化的属性获取响应
    printfLog(EN_LOG_LEVEL_INFO, "Test_propGetResponse() for requestId: %s\n", requestId);
}

// ==================== 回调函数实现 ====================

void handleAuthSuccess(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_INFO, "登录成功\n");
    disconnected_ = 0;
}

void handleAuthFailure(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_ERROR, "登录失败: code=%d, message=%s\n", code, message);
    
    // 3秒后重试
    printfLog(EN_LOG_LEVEL_INFO, "3秒后尝试重新登录...\n");
    timeSleep(3000);
    int ret = IOTA_Connect();
    if (ret != 0)
    {
        printfLog(EN_LOG_LEVEL_ERROR, "重新登录失败, result %d\n", ret);
    }
}

void handleConnectionLost(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_WARNING, "连接丢失: code=%d, message=%s\n", code, message);
    
    // 5秒后重连
    printfLog(EN_LOG_LEVEL_INFO, "5秒后尝试重新连接...\n");
    timeSleep(5000);
    int ret = IOTA_Connect();
    if (ret != 0)
    {
        printfLog(EN_LOG_LEVEL_ERROR, "重新连接失败, result %d\n", ret);
    }
}

void handleDisAuthSuccess(void* context, int messageId, int code, char *message)
{
    disconnected_ = 1;
    printfLog(EN_LOG_LEVEL_INFO, "登出成功\n");
}

void handleDisAuthFailure(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_ERROR, "登出失败\n");
}

void handleSubscribesuccess(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_INFO, "订阅成功\n");
}

void handleSubscribeFailure(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_WARNING, "订阅失败\n");
}

void handlePublishSuccess(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_INFO, "发布成功\n");
}

void handlePublishFailure(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_WARNING, "发布失败\n");
}

void handleMessageDown(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_INFO, "收到下行消息: %s\n", message);
}

void handleCommandRequest(void* context, int messageId, int code, char *message, char *requestId)
{
    printfLog(EN_LOG_LEVEL_INFO, "收到命令请求: %s, requestId: %s\n", message, requestId);
    
    // 解析JSON
    cJSON *root = cJSON_Parse(message);
    if (!root) {
        printfLog(EN_LOG_LEVEL_ERROR, "解析命令请求失败\n");
        return;
    }
    
    char* service_id = NULL;
    char* command_name = NULL;
    cJSON* paras = NULL;
    
    cJSON *service_id_obj = cJSON_GetObjectItem(root, "service_id");
    if (service_id_obj && cJSON_IsString(service_id_obj)) {
        service_id = service_id_obj->valuestring;
    }
    
    cJSON *command_name_obj = cJSON_GetObjectItem(root, "command_name");
    if (command_name_obj && cJSON_IsString(command_name_obj)) {
        command_name = command_name_obj->valuestring;
    }
    
    paras = cJSON_GetObjectItem(root, "paras");
    
    // 处理控制命令
    if (service_id && command_name && paras) {
        if (strcmp(service_id, "Control") == 0) {
            if (strcmp(command_name, "control_led") == 0) {
                cJSON *action_obj = cJSON_GetObjectItem(paras, "action");
                if (action_obj && cJSON_IsNumber(action_obj)) {
                    int action = action_obj->valueint;
                    led_status = action;
                    printfLog(EN_LOG_LEVEL_INFO, "控制LED命令: action=%d\n", action);
                    
                    // 写入FIFO
                    char cmd[32];
                    snprintf(cmd, sizeof(cmd), "LED_%s", action ? "ON" : "OFF");
                    write_command_to_fifo(cmd);
                    
                    // 通过TCP发送
                    if (client_c_initialized && client_c_is_connected()) {
                        client_c_send_command(cmd);
                    }
                }
            }
            else if (strcmp(command_name, "control_buzzer") == 0) {
                cJSON *action_obj = cJSON_GetObjectItem(paras, "action");
                if (action_obj && cJSON_IsNumber(action_obj)) {
                    int action = action_obj->valueint;
                    buzzer_status = action;
                    printfLog(EN_LOG_LEVEL_INFO, "控制蜂鸣器命令: action=%d\n", action);
                    
                    // 写入FIFO
                    char cmd[32];
                    snprintf(cmd, sizeof(cmd), "BUZZER_%s", action ? "ON" : "OFF");
                    write_command_to_fifo(cmd);
                    
                    // 通过TCP发送
                    if (client_c_initialized && client_c_is_connected()) {
                        client_c_send_command(cmd);
                    }
                }
            }
        }
    }
    
    cJSON_Delete(root);
    
    // 发送响应
    Test_commandResponse(requestId);
}

// 属性设置回调
void handlePropertiesSet(void* context, int messageId, int code, char *message, char *requestId)
{
    printfLog(EN_LOG_LEVEL_INFO, "收到属性设置请求: %s, requestId: %s\n", message, requestId);
    
    cJSON *root = cJSON_Parse(message);
    if (!root) {
        printfLog(EN_LOG_LEVEL_ERROR, "解析属性设置请求失败\n");
        return;
    }
    
    cJSON *services = cJSON_GetObjectItem(root, "services");
    if (services && cJSON_IsArray(services)) {
        int size = cJSON_GetArraySize(services);
        for (int i = 0; i < size; i++) {
            cJSON *service = cJSON_GetArrayItem(services, i);
            cJSON *service_id_obj = cJSON_GetObjectItem(service, "service_id");
            cJSON *properties = cJSON_GetObjectItem(service, "properties");
            
            if (service_id_obj && properties) {
                char *service_id = service_id_obj->valuestring;
                
                if (strcmp(service_id, "smokeDetector") == 0) {
                    cJSON *alarm_obj = cJSON_GetObjectItem(properties, "alarm");
                    if (alarm_obj && cJSON_IsNumber(alarm_obj)) {
                        alarmValue = alarm_obj->valueint;
                        printfLog(EN_LOG_LEVEL_INFO, "设置烟感报警值: %d\n", alarmValue);
                    }
                }
                else if (strcmp(service_id, "Control") == 0) {
                    cJSON *led_obj = cJSON_GetObjectItem(properties, "led_status");
                    cJSON *buzzer_obj = cJSON_GetObjectItem(properties, "buzzer_status");
                    
                    if (led_obj && cJSON_IsNumber(led_obj)) {
                        led_status = led_obj->valueint;
                        printfLog(EN_LOG_LEVEL_INFO, "设置LED状态: %d\n", led_status);
                        
                        char cmd[32];
                        snprintf(cmd, sizeof(cmd), "LED_%s", led_status ? "ON" : "OFF");
                        write_command_to_fifo(cmd);
                        
                        if (client_c_initialized && client_c_is_connected()) {
                            client_c_send_command(cmd);
                        }
                    }
                    
                    if (buzzer_obj && cJSON_IsNumber(buzzer_obj)) {
                        buzzer_status = buzzer_obj->valueint;
                        printfLog(EN_LOG_LEVEL_INFO, "设置蜂鸣器状态: %d\n", buzzer_status);
                        
                        char cmd[32];
                        snprintf(cmd, sizeof(cmd), "BUZZER_%s", buzzer_status ? "ON" : "OFF");
                        write_command_to_fifo(cmd);
                        
                        if (client_c_initialized && client_c_is_connected()) {
                            client_c_send_command(cmd);
                        }
                    }
                }
            }
        }
    }
    
    cJSON_Delete(root);
    Test_propSetResponse(requestId);
}

void handlePropertiesGet(void* context, int messageId, int code, char *message, char *requestId)
{
    printfLog(EN_LOG_LEVEL_INFO, "收到属性获取请求: %s, requestId: %s\n", message, requestId);
    Test_propGetResponse(requestId);
}

void handleEventsDown(void* context, int messageId, int code, char *message)
{
    printfLog(EN_LOG_LEVEL_INFO, "收到子设备消息: %s\n", message);
}

// ==================== 主函数 ====================

int main(int argc, char **argv)
{
#if defined(_DEBUG)
    setvbuf(stdout, NULL, _IONBF, 0);
#endif
    
    printfLog(EN_LOG_LEVEL_INFO, "============== 智能家居网关程序启动 ==============\n");
    
    // 1. 设置日志回调
    IOTA_SetPrintLogCallback(myPrintLog);
    
    // 2. 从配置文件读取华为云IoT配置
    printfLog(EN_LOG_LEVEL_INFO, "读取华为云IoT配置...\n");
    setConnectConfig();
    
    if (!serverIp_ || !username_ || !password_) {
        printfLog(EN_LOG_LEVEL_ERROR, "华为云IoT配置不完整，程序退出\n");
        return 1;
    }
    
    // 3. 初始化客户端C
    printfLog(EN_LOG_LEVEL_INFO, "初始化客户端C...\n");
    client_c_config_t client_c_config = {
        .server_ip = SERVER_IP,
        .server_port = SERVER_PORT,
        .client_id = CLIENT_C_ID,
        .weather_callback = NULL,
        .command_callback = NULL,
        .status_callback = client_c_status_callback
    };
    
    if (!client_c_init(&client_c_config)) {
        printfLog(EN_LOG_LEVEL_ERROR, "客户端C初始化失败\n");
        return 1;
    }
    
    client_c_initialized = true;
    client_c_set_debug(true);
    
    if (!client_c_start()) {
        printfLog(EN_LOG_LEVEL_ERROR, "客户端C启动失败\n");
        return 1;
    }
    
    // 4. 启动FIFO监听线程
    printfLog(EN_LOG_LEVEL_INFO, "启动FIFO监听线程...\n");
    start_fifo_listener();
    
    // 5. 启动命令处理线程
    printfLog(EN_LOG_LEVEL_INFO, "启动命令处理线程...\n");
    start_command_handler_thread();
    
    // 6. 初始化华为云IoT SDK
    printfLog(EN_LOG_LEVEL_INFO, "初始化华为云IoT SDK...\n");
    if (IOTA_Init(workPath) > 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "华为云IoT SDK初始化失败\n");
        goto cleanup;
    }
    
    // 7. 设置华为云IoT配置和回调
    setAuthConfig();
    setMyCallbacks();
    
    // 8. 连接华为云平台
    printfLog(EN_LOG_LEVEL_INFO, "连接华为云平台...\n");
    int ret = IOTA_Connect();
    if (ret != 0) {
        printfLog(EN_LOG_LEVEL_ERROR, "华为云平台连接失败, 错误码: %d\n", ret);
        goto cleanup;
    }
    
    printfLog(EN_LOG_LEVEL_INFO, "华为云平台连接成功\n");
    timeSleep(2000);
    
    // 9. 主循环
    int count = 0;
    printfLog(EN_LOG_LEVEL_INFO, "进入主循环...\n");
    
    while(count < 10000) {
        // 检查客户端C连接状态
        if (!client_c_is_connected()) {
            printfLog(EN_LOG_LEVEL_WARNING, "客户端C未连接\n");
            timeSleep(2000);
            continue;
        }
        
        // 属性上报
        Test_propertiesReport();
        
        // 休眠
        timeSleep(sleepTime);
        
        count++;
        
        // 每10次输出一次状态
        if (count % 10 == 0) {
            printfLog(EN_LOG_LEVEL_INFO, "运行中... 已循环 %d 次\n", count);
        }
    }
    
cleanup:
    // 10. 清理资源
    printfLog(EN_LOG_LEVEL_INFO, "============== 清理资源 ==============\n");
    
    // 停止命令处理线程
    stop_command_handler_thread();
    
    // 停止FIFO监听线程
    stop_fifo_listener();
    
    // 停止客户端C
    if (client_c_initialized) {
        client_c_stop();
    }
    
    // 断开华为云连接
    printfLog(EN_LOG_LEVEL_INFO, "断开华为云连接...\n");
    IOTA_DisConnect();
    
    // 销毁华为云SDK
    printfLog(EN_LOG_LEVEL_INFO, "销毁华为云SDK...\n");
    IOTA_Destroy();
    
    // 释放配置内存
    if (username_) free(username_);
    if (password_) free(password_);
    
    printfLog(EN_LOG_LEVEL_INFO, "============== 程序退出 ==============\n");
    return 0;
}