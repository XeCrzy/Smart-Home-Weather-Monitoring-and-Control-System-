#ifndef _IOTA_CFG_H_
#define _IOTA_CFG_H_

/**
 * enumeration values for item in IOTA_ConfigSetStr(HW_INT iItem, HW_CHAR *pValue)
 */
typedef enum enum_IOTA_CFG_TYPE
{
    EN_IOTA_CFG_DEVICEID     = 0,  //the value for this item is deviceId
    EN_IOTA_CFG_DEVICESECRET = 1,

    EN_IOTA_CFG_MQTT_ADDR    = 2,
    EN_IOTA_CFG_MQTT_PORT    = 3,

    EN_IOTA_CFG_AUTH_MODE    = 4, //deprecated item.
    
    EN_IOTA_CFG_LOG_LOCAL_NUMBER = 5, //take effect only when syslog is available
    EN_IOTA_CFG_LOG_LEVEL        = 6, //take effect only when syslog is available

    EN_IOTA_CFG_KEEP_ALIVE_TIME  = 7,
    EN_IOTA_CFG_CONNECT_TIMEOUT  = 8,
    EN_IOTA_CFG_RETRY_INTERVAL   = 9,

    EN_IOTA_CFG_RESET_SECRET_IN_PROGRESS   = 10,

    EN_IOTA_CFG_QOS   = 11

}EN_IOTA_CFG_TYPE;

typedef enum enum_IOTA_CFG_AUTH_MODE
{
    EN_IOTA_CFG_AUTH_MODE_DEVICE_ID = 0,
    EN_IOTA_CFG_AUTH_MODE_NODE_ID = 2,
}ENUM_IOTA_CFG_AUTH_MODE;      //deprecated enum.

HW_API_FUNC HW_INT IOTA_ConfigSetStr(HW_INT iItem, HW_CHAR *pValue);
HW_API_FUNC HW_INT IOTA_ConfigSetUint(HW_INT iItem, HW_UINT uiValue);

#endif

