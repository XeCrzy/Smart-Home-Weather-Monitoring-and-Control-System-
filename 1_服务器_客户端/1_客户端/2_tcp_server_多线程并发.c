#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 60000
#define BUFFER_SIZE 1024

// 全局变量，需要在多线程间共享
int client_a_fd = -1;
int client_b_fd = -1;
int client_c_fd = -1;  // 新增客户端C
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    
    char buffer[BUFFER_SIZE];
    
    // 接收客户端身份标识
    memset(buffer, 0, BUFFER_SIZE);
    int ret = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (ret <= 0) {
        printf("接收客户端标识失败\n");
        close(client_fd);
        return NULL;
    }
    
    printf("客户端连接，标识: %s\n", buffer);
    
    // 根据标识设置客户端类型
    pthread_mutex_lock(&lock);
    if (strstr(buffer, "CLIENT_A") != NULL) {
        client_a_fd = client_fd;
        printf("设置为客户端A\n");
        
        // 如果客户端B已连接，通知客户端A
        if (client_b_fd != -1) {
            const char *msg = "CLIENT_B_CONNECTED";
            send(client_a_fd, msg, strlen(msg), 0);
            printf("已通知客户端A：客户端B已连接\n");
        }
        
        // 如果客户端C已连接，通知客户端A
        if (client_c_fd != -1) {
            const char *msg = "CLIENT_C_CONNECTED";
            send(client_a_fd, msg, strlen(msg), 0);
            printf("已通知客户端A：客户端C已连接\n");
        }
    } else if (strstr(buffer, "CLIENT_B") != NULL) {
        client_b_fd = client_fd;
        printf("设置为客户端B\n");
        
        // 如果客户端A已连接，通知客户端A
        if (client_a_fd != -1) {
            const char *msg = "CLIENT_B_CONNECTED";
            send(client_a_fd, msg, strlen(msg), 0);
            printf("已通知客户端A：客户端B已连接\n");
        }
    } else if (strstr(buffer, "CLIENT_C") != NULL) {
        client_c_fd = client_fd;
        printf("设置为客户端C\n");
        
        // 如果客户端A已连接，通知客户端A
        if (client_a_fd != -1) {
            const char *msg = "CLIENT_C_CONNECTED";
            send(client_a_fd, msg, strlen(msg), 0);
            printf("已通知客户端A：客户端C已连接\n");
        }
    } else {
        printf("未知的客户端标识: %s\n", buffer);
        close(client_fd);
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    pthread_mutex_unlock(&lock);
    
    // 发送连接确认
    const char *confirm_msg = "CONNECTED";
    send(client_fd, confirm_msg, strlen(confirm_msg), 0);
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ret = recv(client_fd, buffer, BUFFER_SIZE, 0);
        
        if (ret <= 0) {
            printf("客户端断开连接\n");
            pthread_mutex_lock(&lock);
            if (client_fd == client_a_fd) {
                client_a_fd = -1;
                printf("客户端A已断开\n");
            } else if (client_fd == client_b_fd) {
                client_b_fd = -1;
                printf("客户端B已断开\n");
            } else if (client_fd == client_c_fd) {
                client_c_fd = -1;
                printf("客户端C已断开\n");
            }
            pthread_mutex_unlock(&lock);
            close(client_fd);
            break;
        }
        
        // 打印接收到的消息
        printf("收到消息: %s\n", buffer);
        
        pthread_mutex_lock(&lock);
        if (client_fd == client_a_fd) {
            // 客户端A发送的是天气信息，转发给客户端B和客户端C
            if (client_b_fd != -1) {
                send(client_b_fd, buffer, strlen(buffer), 0);
                printf("转发天气信息给客户端B\n");
            }
            if (client_c_fd != -1) {
                send(client_c_fd, buffer, strlen(buffer), 0);
                printf("转发天气信息给客户端C\n");
            }
        } else if (client_fd == client_b_fd) {
            // 客户端B发送的是城市名，转发给客户端A
            if (client_a_fd != -1) {
                send(client_a_fd, buffer, strlen(buffer), 0);
                printf("转发城市名给客户端A\n");
            }
        } else if (client_fd == client_c_fd) {
            // 客户端C发送的是命令信息，转发给客户端B
            if (client_b_fd != -1) {
                send(client_b_fd, buffer, strlen(buffer), 0);
                printf("转发命令信息给客户端B: %s\n", buffer);
            }
        }
        pthread_mutex_unlock(&lock);
    }
    
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("创建socket失败");
        return -1;
    }
    
    // 允许地址复用
    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("绑定端口失败");
        close(server_fd);
        return -1;
    }
    
    if (listen(server_fd, 5) < 0) {
        perror("监听失败");
        close(server_fd);
        return -1;
    }
    
    printf("服务器启动，端口: %d\n", PORT);
    printf("等待客户端连接...\n\n");
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        
        if (client_fd < 0) {
            perror("接受连接失败");
            continue;
        }
        
        char *ip = inet_ntoa(client_addr.sin_addr);
        printf("新客户端连接: %s\n", ip);
        
        // 创建线程处理客户端
        pthread_t tid;
        int *client_ptr = malloc(sizeof(int));
        *client_ptr = client_fd;
        
        if (pthread_create(&tid, NULL, handle_client, client_ptr) != 0) {
            perror("创建线程失败");
            close(client_fd);
            free(client_ptr);
            continue;
        }
        pthread_detach(tid);
    }
    
    close(server_fd);
    return 0;
}