#ifndef INCLUDE_SERVICE_CALLBACK_H_
#define INCLUDE_SERVICE_CALLBACK_H_

typedef enum enum_CALLBACK_SETTING
{
    EN_CALLBACK_CONNECT_SUCCESS     = 0,
    EN_CALLBACK_CONNECT_FAILURE     = 1,
    EN_CALLBACK_DISCONNECT_SUCCESS  = 2,
    EN_CALLBACK_DISCONNECT_FAILURE  = 3,
    EN_CALLBACK_CONNECTION_LOST     = 4,

    EN_CALLBACK_PUBLISH_SUCCESS     = 5,
    EN_CALLBACK_PUBLISH_FAILURE     = 6,

    EN_CALLBACK_SUBSCRIBE_SUCCESS   = 7,
    EN_CALLBACK_SUBSCRIBE_FAILURE   = 8,

	EN_CALLBACK_MESSAGE_DOWN = 9,
	EN_CALLBACK_COMMAND_REQUEST = 10,
	EN_CALLBACK_PROPERTIES_SET = 11,
	EN_CALLBACK_PROPERTIES_GET = 12,
	EN_CALLBACK_SUB_DEVICE_MESSAGE_DOWN = 14,

    EN_CALLBACK_COMMAND_ARRIVED     = 15

}EN_CALLBACK_SETTING;

/**
 * @param context the application-self defined parameter, current is NULL
 * @param token the inner messageId(1-65535) of the uplink message in this MQTT SDK
 * @param code the reason code in the failure callback
 * @param message the message can be from the IoT platform (e.g. command content) or from the MQTT SDK (e.g. failure explanation message)
 */
typedef void (*CALLBACK_HANDLER)(void* context, int token, int code, char *message);
typedef void (*CALLBACK_HANDLER_WITH_TOPIC)(void* context, int token, int code, char *message, char *requestId);
typedef void (*LOG_CALLBACK_HANDLER)(int level, char* format, va_list args);

void onLoginSuccess(void* context, int token, int code, char *message);
void onMessageArrived(void* context, int token, int code, char *topic, char *message);
_DLLEXPORT int setCallback(int item, CALLBACK_HANDLER callbackHandler);
_DLLEXPORT void setLogCallback(LOG_CALLBACK_HANDLER logCallbackHandler);
_DLLEXPORT int setCallbackWithTopic(int item, CALLBACK_HANDLER_WITH_TOPIC callbackHandler);

#endif /* INCLUDE_SERVICE_CALLBACK_H_ */
