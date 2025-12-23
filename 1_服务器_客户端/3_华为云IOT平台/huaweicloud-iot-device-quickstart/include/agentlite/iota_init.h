#ifndef _IOTA_INIT_H_
#define _IOTA_INIT_H_

#include "stdarg.h"
/**
 * @param context the application-self defined parameter, current is NULL
 * @param messageId the inner messageId(1-65535) of the uplink message in this MQTT SDK
 * @param code the reason code for the failure callback
 * @param message the message can be from the IoT platform (e.g. command content) or from the MQTT SDK (e.g. failure explanation message)
 */
typedef HW_VOID (*PFN_CALLBACK_HANDLER)(HW_VOID* context, HW_INT messageId, HW_INT code, HW_CHAR *message);
typedef HW_VOID (*PFN_LOG_CALLBACK_HANDLER)(int level, char* format, va_list args);

typedef HW_VOID (*PFN_CALLBACK_HANDLER_WITH_TOPIC)(HW_VOID* context, HW_INT messageId, HW_INT code, HW_CHAR *message, HW_CHAR *requestId);

HW_API_FUNC HW_INT IOTA_Init(HW_CHAR *pcWorkPath);
HW_API_FUNC HW_INT IOTA_Destroy(void);

HW_API_FUNC HW_VOID IOTA_SetCallback(HW_INT iItem, PFN_CALLBACK_HANDLER pfnCallbackHandler);
HW_API_FUNC void IOTA_SetCallbackWithTopic(HW_INT iItem, PFN_CALLBACK_HANDLER_WITH_TOPIC pfnCallbackHandler);

HW_API_FUNC HW_VOID IOTA_SetPrintLogCallback(PFN_LOG_CALLBACK_HANDLER pfnLogCallbackHandler);

typedef enum enum_IOTA_CALLBACK_SETTING
{
    EN_IOTA_CALLBACK_CONNECT_SUCCESS     = 0,
    EN_IOTA_CALLBACK_CONNECT_FAILURE     = 1,
    EN_IOTA_CALLBACK_DISCONNECT_SUCCESS  = 2,
    EN_IOTA_CALLBACK_DISCONNECT_FAILURE  = 3,
    EN_IOTA_CALLBACK_CONNECTION_LOST     = 4,

    EN_IOTA_CALLBACK_PUBLISH_SUCCESS     = 5,
    EN_IOTA_CALLBACK_PUBLISH_FAILURE     = 6,

    EN_IOTA_CALLBACK_SUBSCRIBE_SUCCESS   = 7,
    EN_IOTA_CALLBACK_SUBSCRIBE_FAILURE   = 8,

	EN_IOTA_CALLBACK_MESSAGE_DOWN = 9,
	EN_IOTA_CALLBACK_COMMAND_REQUEST = 10,
	EN_IOTA_CALLBACK_PROPERTIES_SET = 11,
	EN_IOTA_CALLBACK_PROPERTIES_GET = 12,
	EN_IOTA_CALLBACK_SUB_DEVICE_MESSAGE_DOWN = 14

}EN_IOTA_CALLBACK_SETTING;


#endif

