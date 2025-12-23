#ifndef _IOTA_DATATRANS_H_
#define _IOTA_DATATRANS_H_

#include "cJSON.h"

#define MaxServiceReportNum 10
typedef struct stru_ST_IOTA_SERVICE_DATA_INFO
{
	HW_CHAR *service_id;
	HW_CHAR *event_time;
	HW_CHAR *properties;
}ST_IOTA_SERVICE_DATA_INFO;


typedef struct stru_ST_IOTA_DEVICE_DATA_INFO
{
	HW_CHAR *device_id;
    ST_IOTA_SERVICE_DATA_INFO services[MaxServiceReportNum];   //暂时设置每个消息里最多有10个service同时上报
}ST_IOTA_DEVICE_DATA_INFO;

HW_API_FUNC HW_INT IOTA_ServiceDataReport(HW_CHAR *pcDeviceId, HW_CHAR *pcServiceId, HW_CHAR *pcServiceProperties);
HW_API_FUNC HW_INT IOTA_ServiceBatchDataReport(char *pcPayload);
HW_API_FUNC HW_INT IOTA_ServiceCommandRespense(HW_UINT uiMid, HW_UINT uiResultCode, HW_CHAR *pcCommandRespense);
HW_API_FUNC HW_INT IOTA_MessageReport(HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content);
HW_API_FUNC HW_INT IOTA_PropertiesReport(ST_IOTA_SERVICE_DATA_INFO pServiceData[], int serviceNum);
HW_API_FUNC HW_INT IOTA_BatchPropertiesReport(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], int deviceNum, int serviceLenList[]);
HW_API_FUNC HW_INT IOTA_CommandResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandRespense);
HW_API_FUNC HW_INT IOTA_PropertiesSetResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *result_desc);
HW_API_FUNC HW_INT IOTA_PropertiesGetResponse(HW_CHAR *requestId, ST_IOTA_SERVICE_DATA_INFO serviceProp[], HW_INT serviceNum);
HW_API_FUNC HW_INT IOTA_DeviceGetPropReq(HW_CHAR *requestId, HW_CHAR *deviceId, cJSON *serviceIdList);
HW_API_FUNC HW_INT IOTA_SubDeviceInfoReport(HW_CHAR *pcPayload);
HW_API_FUNC HW_INT IOTA_SubDeviceVersionReport(HW_CHAR *version);
HW_API_FUNC HW_INT IOTA_SubDeviceProductGetReport(cJSON *product_id_list);
HW_API_FUNC HW_INT IOTA_SubDeviceScanReport(cJSON *device_list);

#define SUB_DEVICE_VERSION_REPORT    "sub_device_list_version_report"
#define GET_PRODUCTS    			 "get_products"
#define SCAN_SUB_DEVICE_RESULT       "scan_result"
#define MESSAGE_NAME       			 "message_name"
#define VERSION                      "version"
#define PRODUCTID_LIST       		 "product_id_list"
#define DEVICE_LIST       			 "device_list"
#define SERVICES      			 	 "services"
#define DEVICES      			 	 "devices"
#define RESULT_CODE      			 "result_code"
#define RESULT      			 	 "result"
#define TARGET_DEVICE_ID      		 "target_device_id"
#define DEVICE_ID       			 "device_id"
#define SERVICE_ID       			 "service_id"
#define EVENT_TIME      			 "event_time"
#define PROPERTIES      			 "properties"
#define RESPONSE_NAME      			 "response_name"
#define PARAS  	    			 	 "paras"
#define RESULT_DESC      			 "result_desc"
#define CONTENT      				 "content"
#define ID      					 "id"
#define NAME	      				 "name"
#define OBJECT_DEVICE_ID      		 "object_device_id"




/**
 * ----------------------------deprecated below------------------------------------->
 */
#define IOTA_TOPIC_SERVICE_DATA_REPORT_RET    "IOTA_TOPIC_SERVICE_DATA_REPORT_RET"
#define IOTA_TOPIC_SERVICE_COMMAND_RECEIVE    "IOTA_TOPIC_SERVICE_COMMAND_RECEIVE"
#define IOTA_TOPIC_DATATRANS_REPORT_RSP       "IOTA_TOPIC_DATATRANS_REPORT_RSP"

typedef enum enum_EN_IOTA_DATATRANS_IE_TYPE
{
    EN_IOTA_DATATRANS_IE_RESULT     = 0,//nsigned int  命令执行返回结果
    EN_IOTA_DATATRANS_IE_DEVICEID   = 1,//String        设备ID
    EN_IOTA_DATATRANS_IE_REQUESTID  = 2,//String        请求ID
    EN_IOTA_DATATRANS_IE_SERVICEID  = 3,//String        服务ID
    EN_IOTA_DATATRANS_IE_METHOD     = 4,//String        服务方法
    EN_IOTA_DATATRANS_IE_CMDCONTENT = 5,//String        命令内容
}EN_IOTA_DATATRANS_IE_TYPE;

#endif
