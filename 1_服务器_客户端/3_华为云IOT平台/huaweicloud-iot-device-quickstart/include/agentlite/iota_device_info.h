#ifndef _IOTA_DEVICE_INFO_H_
#define _IOTA_DEVICE_INFO_H_

/**
 * <b>Description:</b> It is used to notify caller login successed. 
 * <br><b>Purpose:</b> After the caller login, When the UI receives this notification, it means the caller 
 * login successed, and the UI must be updated. The UI can not receive this notification if caller login 
 * failed. The UI is recommended to perform interception on this notification during the entire running 
 * of the application.
 */
typedef struct stru_ST_IOTA_DEVICE_INFO
{
    HW_CHAR *pcNodeId;
    HW_CHAR *pcName;
    HW_CHAR *pcDescription;
    HW_CHAR *pcProductId;
    HW_CHAR *pcManufacturerId;
    HW_CHAR *pcModel;
    HW_CHAR *pcSwVersion;
    HW_CHAR *pcFwVersion;
    HW_CHAR *ONLINE;
}ST_IOTA_DEVICE_INFO;

#endif

