#ifndef INCLUDE_MQTT_BASE_H_
#define INCLUDE_MQTT_BASE_H_

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#include "MQTTAsync.h"

typedef enum enum_MQTT_BASE_CALLBACK_SETTING
{
    EN_MQTT_BASE_CALLBACK_CONNECT_SUCCESS     = 0,
    EN_MQTT_BASE_CALLBACK_CONNECT_FAILURE     = 1,
    EN_MQTT_BASE_CALLBACK_DISCONNECT_SUCCESS  = 2,
    EN_MQTT_BASE_CALLBACK_DISCONNECT_FAILURE  = 3,
    EN_MQTT_BASE_CALLBACK_CONNECTION_LOST     = 4,

    EN_MQTT_BASE_CALLBACK_PUBLISH_SUCCESS     = 5,
    EN_MQTT_BASE_CALLBACK_PUBLISH_FAILURE     = 6,

    EN_MQTT_BASE_CALLBACK_SUBSCRIBE_SUCCESS   = 7,
    EN_MQTT_BASE_CALLBACK_SUBSCRIBE_FAILURE   = 8,

    EN_MQTT_BASE_CALLBACK_MESSAGE_ARRIVED     = 15 //the same value as EN_CALLBACK_COMMAND_ARRIVED in Callback.h

}EN_MQTT_BASE_CALLBACK_SETTING;

//see also enum_BASE_CONFIG_SETTING in Base.h
typedef enum enum_MQTT_BASE_CONFIG
{
    EN_MQTT_BASE_CONFIG_USERNAME    = 0,
    EN_MQTT_BASE_CONFIG_PASSWORD    = 1,

    EN_MQTT_BASE_CONFIG_SERVER_IP   = 2,
    EN_MQTT_BASE_CONFIG_SERVER_PORT = 3,

    EN_MQTT_BASE_CONFIG_AUTH_MODE    = 4,

    EN_MQTT_BASE_CONFIG_LOG_LOCAL_NUMBER = 5,
    EN_MQTT_BASE_CONFIG_LOG_LEVEL        = 6,

    EN_MQTT_BASE_CONFIG_KEEP_ALIVE_TIME  = 7,
    EN_MQTT_BASE_CONFIG_CONNECT_TIMEOUT  = 8,
    EN_MQTT_BASE_CONFIG_RETRY_INTERVAL   = 9,

    EN_MQTT_BASE_CONFIG_RESET_SECRET_IN_PROGRESS = 10,

    EN_MQTT_BASE_CONFIG_QOS = 11

}ENUM_MQTT_BASE_CONFIG;

typedef void (*MQTT_BASE_CALLBACK_HANDLER)(void* context, int token, int code, char *message);
typedef void (*MQTT_BASE_CALLBACK_HANDLER_WITH_TOPIC)(void* context, int token, int code, char *topic, char *message);

typedef void MqttBase_connectionLost(void *context, char *cause);
typedef void MqttBase_deliveryComplete(void *context, int token);
typedef void MqttBase_messageArrived(void *context, char *topic, char *message);
typedef void MqttBase_commonCallback(void *context, int token, char *message);

int getEncryptedPassword(const char *timestamp, char **encryptedPwd);
void handleCallbackFailure(char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void* context, MQTTAsync_failureData* response);
void handleCallbackSuccess(char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void* context, MQTTAsync_successData* response);
void MqttBase_onConnectSuccess(void* context, MQTTAsync_successData* response);
void MqttBase_onConnectFailure(void* context, MQTTAsync_failureData* response);
void MqttBase_onDisconnectSuccess(void* context, MQTTAsync_successData* response);
void MqttBase_onDisconnectFailure(void* context,  MQTTAsync_failureData* response);
void MqttBase_onConnectionLost(void *context, char *cause);
void MqttBase_onSubscribeSuccess(void* context, MQTTAsync_successData* response);
void MqttBase_onSubscribeFailure(void* context, MQTTAsync_failureData* response);
void MqttBase_onPublishSuccess(void* context, MQTTAsync_successData* response);
void MqttBase_onPublishFailure(void* context,  MQTTAsync_failureData* response);
int MqttBase_onMessageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message);


int MqttBase_init(char *workPath);
int MqttBase_setConfig(int item, char *value);
char* MqttBase_getConfig(int item);
int MqttBase_setCallback(int item, MQTT_BASE_CALLBACK_HANDLER handler);
int MqttBase_setCallbackWithTopic(int item, MQTT_BASE_CALLBACK_HANDLER_WITH_TOPIC handler);
int MqttBase_createConnection(void);
int MqttBase_releaseConnection(void);
int MqttBase_subscribe(char *topic);
int MqttBase_publish(char *topic, char *payload);
int MqttBase_stringlength(char *str);
int MqttBase_destory(void);

#endif

