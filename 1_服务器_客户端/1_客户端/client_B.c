#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.16.181"
#define SERVER_PORT 60000
#define BUFFER_SIZE 1024

int client_fd;

int main() {
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    printf("已连接到服务器 %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("我是客户端B (192.168.16.182)\n");
    printf("输入城市名发送给客户端A，客户端A会自动查询该城市天气\n");
    printf("输入quit退出\n\n");
    
    // 发送身份标识
    send(client_fd, "CLIENT_B", 8, 0);
    
    // 接收连接确认
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_fd, buffer, BUFFER_SIZE, 0);
    printf("服务器确认: %s\n", buffer);
    
    // 接收初始天气信息
    printf("等待客户端A发送天气信息...\n");
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_fd, buffer, BUFFER_SIZE, 0);
    printf("收到天气信息:\n%s\n", buffer);
    
    // 发送消息
    while (1) {
        printf("输入城市名更新天气查询 (如: 北京/beijing, 上海/shanghai, 广州/guangzhou): ");
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        buffer[strcspn(buffer, "\n")] = 0;
        
        if (strcmp(buffer, "quit") == 0) {
            break;
        }
        
        send(client_fd, buffer, strlen(buffer), 0);
        printf("已发送城市名: %s\n", buffer);
        
    // 等待客户端A返回天气信息
    printf("等待客户端A返回天气信息...\n");
    memset(buffer, 0, BUFFER_SIZE);

    // 设置接收超时
    struct timeval tv;
    tv.tv_sec = 10;  // 10秒超时
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int ret = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (ret <= 0) {
        printf("等待超时或连接错误\n");
        strcpy(buffer, "等待响应超时");
    }

    printf("收到天气信息:\n%s\n", buffer);
    }
    
    close(client_fd);
    return 0;
}