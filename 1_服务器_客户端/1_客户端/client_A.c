#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.16.181"
#define SERVER_PORT 60000
#define BUFFER_SIZE 1024

// 声明 forecast.c 中的函数
void set_current_city(const char *city);
const char* get_current_city();
char* get_weather_data();

int client_fd;
int running = 1;
int client_b_connected = 0;
int client_c_connected = 0;  // 新增客户端C连接状态

// 信号处理函数
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n收到中断信号，正在退出...\n");
        running = 0;
    }
}

// 发送天气数据给服务器
void send_weather_to_server() {
    printf("正在查询 %s 的天气...\n", get_current_city());
    
    // 尝试多次查询，增加成功率
    char *weather_info = NULL;
    int max_retries = 3;
    for (int i = 0; i < max_retries; i++) {
        weather_info = get_weather_data();
        if (weather_info) {
            break;
        }
        printf("第%d次查询失败，1秒后重试...\n", i+1);
        sleep(1);
    }
    
    if (weather_info) {
        printf("查询结果:\n%s", weather_info);
        
        // 发送天气信息给服务器（转发给客户端B和客户端C）
        send(client_fd, weather_info, strlen(weather_info), 0);
        printf("已发送天气信息给服务器（转发给客户端B和客户端C）\n");
        free(weather_info);
    } else {
        printf("获取天气信息失败，发送错误消息\n");
        // 发送错误消息给客户端B和客户端C
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                "无法获取 %s 的天气信息，请检查城市名是否正确", get_current_city());
        send(client_fd, error_msg, strlen(error_msg), 0);
    }
}

int main() {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    
    // 创建socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("创建socket失败");
        return 1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // 转换IP地址
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("IP地址转换失败");
        close(client_fd);
        return 1;
    }
    
    printf("正在连接到服务器 %s:%d\n", SERVER_IP, SERVER_PORT);
    
    // 尝试连接
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("无法连接到服务器");
        printf("可能的原因：\n");
        printf("1. 服务器未运行\n");
        printf("2. 服务器IP地址错误\n");
        printf("3. 端口被占用\n");
        close(client_fd);
        return 1;
    }
    
    printf("连接成功！\n");
    printf("已连接到服务器 %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("我是客户端A\n");
    printf("当前查询城市: %s\n", get_current_city());
    printf("等待客户端B连接...\n\n");
    
    // 发送身份标识
    send(client_fd, "CLIENT_A", 8, 0);
    
    // 接收连接确认
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_fd, buffer, BUFFER_SIZE, 0);
    printf("服务器确认: %s\n", buffer);
    
    // 设置接收超时
    struct timeval tv;
    tv.tv_sec = 2;  // 2秒超时
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // 等待客户端B连接的通知（最多等待5秒）
    printf("等待客户端B连接...\n");
    int wait_count = 0;
    while (wait_count < 5 && running) {
        memset(buffer, 0, BUFFER_SIZE);
        int ret = recv(client_fd, buffer, BUFFER_SIZE, 0);
        
        if (ret > 0) {
            if (strstr(buffer, "CLIENT_B_CONNECTED") != NULL) {
                client_b_connected = 1;
                printf("客户端B已连接！\n");
                break;
            } else if (strstr(buffer, "CLIENT_C_CONNECTED") != NULL) {
                client_c_connected = 1;
                printf("客户端C已连接！\n");
            }
        }
        
        wait_count++;
        sleep(1);
    }
    
    // 移除接收超时，恢复阻塞接收
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // 如果客户端B已连接，发送初始天气数据
    if (client_b_connected) {
        // 获取初始天气数据
        printf("正在查询初始天气数据...\n");
        char *weather_info = get_weather_data();
        if (weather_info) {
            printf("初始天气信息:\n%s", weather_info);
            
            // 发送天气信息给服务器（转发给客户端B和客户端C）
            send(client_fd, weather_info, strlen(weather_info), 0);
            printf("已发送初始天气信息给客户端B和客户端C\n");
            free(weather_info);
        } else {
            printf("获取天气信息失败\n");
        }
        
        // 循环等待客户端B发送城市名
        while (running) {
            printf("\n等待客户端B发送城市名...\n");
            memset(buffer, 0, BUFFER_SIZE);
            int ret = recv(client_fd, buffer, BUFFER_SIZE, 0);
            
            if (ret <= 0) {
                printf("连接断开\n");
                running = 0;
                break;
            }
            
            // 检查是否是客户端C连接通知
            if (strstr(buffer, "CLIENT_C_CONNECTED") != NULL) {
                client_c_connected = 1;
                printf("客户端C已连接！\n");
                // 发送当前天气给新连接的客户端C
                send_weather_to_server();
                continue;
            }
            
            // 更新城市并查询天气
            printf("收到客户端B的城市更新: %s\n", buffer);
            set_current_city(buffer);
            
            // 查询并发送天气数据
            send_weather_to_server();
        }
    } else {
        printf("客户端B未连接，无法继续工作\n");
    }
    
    close(client_fd);
    return 0;
}