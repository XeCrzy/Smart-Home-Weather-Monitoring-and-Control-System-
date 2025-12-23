#ifndef BASE_H_
#define BASE_H_

//see also enum_MQTT_BASE_CONFIG in MqttBase.h
typedef enum enum_BASE_CONFIG
{
    EN_BASE_CONFIG_USERNAME     = 0,
    EN_BASE_CONFIG_PASSWORD     = 1,

    EN_BASE_CONFIG_SERVER_ADDR  = 5,
    EN_BASE_CONFIG_SERVER_PORT  = 6,

    EN_BASE_CONFIG_AUTH_MODE    = 9,

    EN_BASE_CONFIG_LOG_LOCAL_NUMBER  = 10,
    EN_BASE_CONFIG_LOG_LEVEL         = 11,

    EN_BASE_CONFIG_KEEP_ALIVE_TIME   = 12,
    EN_BASE_CONFIG_CONNECT_TIMEOUT   = 13,
    EN_BASE_CONFIG_RETRY_INTERVAL    = 14,

    EN_BASE_CONFIG_RESET_SECRET_IN_PROGRESS = 15,

    EN_BASE_CONFIG_QOS = 16,
}ENUM_BASE_CONFIG;

#if defined(WIN32) || defined(WIN64) && defined(EXPORT_SERVICE)
#define _DLLEXPORT __declspec(dllexport)
#else
#define _DLLEXPORT
#endif

_DLLEXPORT int init(char* workPath);
_DLLEXPORT int setConfig(int item, char *value);
_DLLEXPORT int destory(void);


#endif /*BASE_H_*/

