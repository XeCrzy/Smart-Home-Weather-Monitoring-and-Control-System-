#ifndef INCLUDE_DATATRANS_H_
#define INCLUDE_DATATRANS_H_

_DLLEXPORT int batchDeviceDataReport(char *payload);
_DLLEXPORT int commandResultReport(char *payload);
_DLLEXPORT int deviceDataReport(char *payload);
_DLLEXPORT int devicePropertiesReport(char *payload);
_DLLEXPORT int batchDevicePropertiesReport(char *payload);

_DLLEXPORT int commandReponseReport(char *requestId, char *pcCommandRespense);
_DLLEXPORT int propSetReponseReport(char *requestId, char *pcCommandRespense);
_DLLEXPORT int propGetReponseReport(char *requestId, char *pcCommandRespense);
_DLLEXPORT int getPropertiesRequest(char *requestId, char *pcCommandRespense);
_DLLEXPORT int subDeviceInfoReport(char *payload);

#endif /* INCLUDE_DATATRANS_H_ */
