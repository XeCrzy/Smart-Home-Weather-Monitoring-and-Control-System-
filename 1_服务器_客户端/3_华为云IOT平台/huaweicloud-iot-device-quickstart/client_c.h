#ifndef CLIENT_C_H
#define CLIENT_C_H

#include <stdbool.h>
#include <pthread.h>

// ==================== FIFO路径定义 ====================
#define FIFO_BASE_PATH "/home/gec/fifofile"
#define WEATHER_FIFO FIFO_BASE_PATH "/weather_fifo"
#define COMMAND_FIFO FIFO_BASE_PATH "/command_fifo"

// ==================== 客户端C配置 ====================
#define SERVER_IP "192.168.16.181"
#define SERVER_PORT 60000
#define CLIENT_C_ID "CLIENT_C"

// 客户端C状态
typedef enum {
    CLIENT_C_DISCONNECTED,
    CLIENT_C_CONNECTING,
    CLIENT_C_CONNECTED,
    CLIENT_C_ERROR
} client_c_state_t;

// 消息类型
typedef enum {
    MSG_TYPE_WEATHER,
    MSG_TYPE_COMMAND,
    MSG_TYPE_STATUS,
    MSG_TYPE_IDENTITY
} msg_type_t;

// 命令类型
typedef enum {
    CMD_LED_ON,
    CMD_LED_OFF,
    CMD_BUZZER_ON,
    CMD_BUZZER_OFF,
    CMD_UNKNOWN
} command_type_t;

// 客户端C回调函数类型
typedef void (*client_c_weather_callback_t)(const char* weather_data);
typedef void (*client_c_command_callback_t)(command_type_t command);
typedef void (*client_c_status_callback_t)(client_c_state_t state);

// 客户端C初始化配置
typedef struct {
    const char* server_ip;
    int server_port;
    const char* client_id;
    client_c_weather_callback_t weather_callback;
    client_c_command_callback_t command_callback;
    client_c_status_callback_t status_callback;
} client_c_config_t;

// ==================== 函数声明 ====================

// 初始化客户端C
bool client_c_init(client_c_config_t* config);

// 启动客户端C
bool client_c_start(void);

// 停止客户端C
void client_c_stop(void);

// 发送命令到服务器
bool client_c_send_command(const char* command);

// 发送消息到服务器（带类型）
bool client_c_send_message(msg_type_t type, const char* data);

// 获取客户端C当前状态
client_c_state_t client_c_get_state(void);

// 检查是否连接
bool client_c_is_connected(void);

// 设置调试模式
void client_c_set_debug(bool enable);

// 解析天气JSON函数声明
bool parse_weather_json(const char* json_str, char* city, char* weather, char* temperature, char* humidity);
#endif // CLIENT_C_H