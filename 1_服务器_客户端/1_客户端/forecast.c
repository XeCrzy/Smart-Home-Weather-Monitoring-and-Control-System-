#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/fb.h>
#include <linux/un.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include "common.h"
#include "cJSON.h"

// 全局变量，存储当前要查询的城市
char current_city[50] = "广州";

// 设置当前城市
void set_current_city(const char *city) {
    if (city != NULL && strlen(city) > 0) {
        strncpy(current_city, city, sizeof(current_city) - 1);
        current_city[sizeof(current_city) - 1] = '\0';
        printf("已更新查询城市为: %s\n", current_city);
    }
}

// 获取当前城市
const char* get_current_city() {
    return current_city;
}

// 获取天气数据（返回格式化字符串，需要调用者释放）
char* get_weather_data() {
    printf("查询城市: %s (长度: %zu)\n", current_city, strlen(current_city));
    
    struct hostent *he = gethostbyname("api.seniverse.com");
    if(he == NULL) {
        perror("DNS查询失败");
        return NULL;
    }

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, len);

    addr.sin_family = AF_INET;
    addr.sin_addr   = *(struct in_addr*)((he->h_addr_list)[0]);
    addr.sin_port   = htons(80);

    printf("正在连接到天气服务器 %s:%d...\n", 
           inet_ntoa(*(struct in_addr*)((he->h_addr_list)[0])), 80);
    
    // 创建TCP套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(connect(fd, (struct sockaddr *)&addr, len) != 0) {
        perror("连接天气服务器失败");
        close(fd);
        return NULL;
    }
    
    printf("已连接到天气服务器\n");
    
    // 准备HTTP请求
    char request[1024];
    snprintf(request, 1024, 
             "GET /v3/weather/now.json?key=SK4cNZ6Q9wXmiwJ0r&location=%s&language=zh-Hans&unit=c HTTP/1.1\r\n"
             "Host: api.seniverse.com\r\n"
             "User-Agent: WeatherClient/1.0\r\n"
             "Connection: close\r\n\r\n", 
             current_city);
    
    printf("发送的HTTP请求:\n%s\n", request);
    
    // 发送请求
    if(write(fd, request, strlen(request)) <= 0) {
        perror("发送请求失败");
        close(fd);
        return NULL;
    }
    
    printf("请求已发送，等待响应...\n");
    
    // 接收完整响应
    char *response = NULL;
    size_t response_size = 0;
    size_t total_read = 0;
    char buffer[4096];
    
    while(1) {
        int n = read(fd, buffer, sizeof(buffer) - 1);
        if(n <= 0) {
            break;
        }
        
        // 扩展响应缓冲区
        char *new_response = realloc(response, response_size + n + 1);
        if(new_response == NULL) {
            free(response);
            close(fd);
            return NULL;
        }
        
        response = new_response;
        memcpy(response + response_size, buffer, n);
        response_size += n;
        response[response_size] = '\0';
        total_read += n;
    }
    
    close(fd);
    
    if(response == NULL) {
        printf("没有收到响应\n");
        return NULL;
    }
    
    printf("收到响应，总大小: %zu字节\n", response_size);
    
    // 查找JSON开始位置（跳过HTTP头）
    char *json_start = strstr(response, "\r\n\r\n");
    if(json_start == NULL) {
        json_start = strstr(response, "\n\n");
    }
    
    if(json_start != NULL) {
        json_start += 4;  // 跳过 \r\n\r\n
    } else {
        json_start = response;
    }
    
    printf("JSON数据开始位置: %ld\n", json_start - response);
    
    // 跳过可能的空白字符
    while(*json_start && isspace((unsigned char)*json_start)) {
        json_start++;
    }
    
    // 解析JSON - 使用json_start而不是之前的json变量
    cJSON *root = cJSON_Parse(json_start);
    if(root == NULL) {
        printf("JSON解析失败\n");
        free(response);
        return NULL;
    }
    
    cJSON *results = cJSON_GetObjectItem(root, "results");
    if(!cJSON_IsArray(results) || cJSON_GetArraySize(results) == 0) {
        printf("没有找到results数组或数组为空\n");
        cJSON_Delete(root);
        free(response);
        return NULL;
    }
    
    cJSON *city_data = cJSON_GetArrayItem(results, 0);
    if(city_data == NULL) {
        printf("city_data为空\n");
        cJSON_Delete(root);
        free(response);
        return NULL;
    }
    
    cJSON *location = cJSON_GetObjectItem(city_data, "location");
    cJSON *now = cJSON_GetObjectItem(city_data, "now");
    
    if(location == NULL || now == NULL) {
        printf("location或now为空\n");
        cJSON_Delete(root);
        free(response);
        return NULL;
    }
    
    // 安全地提取天气信息
    const char *city_name = NULL;
    const char *weather = NULL;
    const char *temperature = NULL;
    const char *humidity = NULL;
    const char *wind_direction = NULL;
    const char *wind_speed = NULL;
    const char *wind_scale = NULL;
    
    cJSON *city_name_item = cJSON_GetObjectItem(location, "name");
    cJSON *weather_item = cJSON_GetObjectItem(now, "text");
    cJSON *temperature_item = cJSON_GetObjectItem(now, "temperature");
    cJSON *humidity_item = cJSON_GetObjectItem(now, "humidity");
    cJSON *wind_direction_item = cJSON_GetObjectItem(now, "wind_direction");
    cJSON *wind_speed_item = cJSON_GetObjectItem(now, "wind_speed");
    cJSON *wind_scale_item = cJSON_GetObjectItem(now, "wind_scale");
    
    if(city_name_item != NULL && cJSON_IsString(city_name_item)) {
        city_name = city_name_item->valuestring;
    }
    
    if(weather_item != NULL && cJSON_IsString(weather_item)) {
        weather = weather_item->valuestring;
    }
    
    if(temperature_item != NULL && cJSON_IsString(temperature_item)) {
        temperature = temperature_item->valuestring;
    }
    
    if(humidity_item != NULL) {
        if(cJSON_IsString(humidity_item)) {
            humidity = humidity_item->valuestring;
        } else if(cJSON_IsNumber(humidity_item)) {
            static char humidity_str[10];
            snprintf(humidity_str, sizeof(humidity_str), "%d", (int)humidity_item->valuedouble);
            humidity = humidity_str;
        }
    }
    
    if(wind_direction_item != NULL && cJSON_IsString(wind_direction_item)) {
        wind_direction = wind_direction_item->valuestring;
    }
    
    if(wind_speed_item != NULL && cJSON_IsString(wind_speed_item)) {
        wind_speed = wind_speed_item->valuestring;
    }
    
    if(wind_scale_item != NULL && cJSON_IsString(wind_scale_item)) {
        wind_scale = wind_scale_item->valuestring;
    }
    
    // 检查所有必需字段是否都有值
    if(city_name == NULL || weather == NULL || temperature == NULL) {
        printf("缺少必需字段: city_name=%p, weather=%p, temperature=%p\n", 
               city_name, weather, temperature);
        cJSON_Delete(root);
        free(response);
        return NULL;
    }
    
    // 如果其他字段为空，使用默认值
    if(humidity == NULL) {
        humidity = "N/A";
    }
    if(wind_direction == NULL) {
        wind_direction = "N/A";
    }
    if(wind_speed == NULL) {
        wind_speed = "N/A";
    }
    if(wind_scale == NULL) {
        wind_scale = "N/A";
    }
    
    printf("提取到的数据: 城市=%s, 天气=%s, 温度=%s, 湿度=%s, 风向=%s, 风速=%s, 风力=%s\n", 
           city_name, weather, temperature, humidity, wind_direction, wind_speed, wind_scale);
    
    // 创建格式化字符串
    char *weather_str = malloc(400);
    if (weather_str == NULL) {
        cJSON_Delete(root);
        free(response);
        return NULL;
    }
    
    // 格式化输出，包含所有天气信息
    snprintf(weather_str, 400, 
             " 城市: %s\n"
             " 天气: %s\n"
             " 温度: %s°C\n"
             " 湿度: %s%%\n"
             " 风向: %s\n"
             " 风速: %s\n"
             " 风力: %s\n",
             city_name, weather, temperature, humidity, 
             wind_direction, wind_speed, wind_scale);
    
    // 清理
    cJSON_Delete(root);
    free(response);
    
    return weather_str;
}