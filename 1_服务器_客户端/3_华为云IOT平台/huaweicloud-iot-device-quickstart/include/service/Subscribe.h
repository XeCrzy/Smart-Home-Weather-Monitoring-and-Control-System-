#ifndef INCLUDE_SUBSCRIBE_H_
#define INCLUDE_SUBSCRIBE_H_

_DLLEXPORT int subscribeCommand(void);
_DLLEXPORT int subscribeDeviceAdditionResult(void);
_DLLEXPORT int subscribeDeviceDeletionResult(void);
_DLLEXPORT int subscribeDeviceUpdateResult(void);
_DLLEXPORT int subscribeDeviceQueryResult(void);
_DLLEXPORT void subscribeAll(void);
_DLLEXPORT int subscribeMessageDown(void);
_DLLEXPORT int subscribePropSet(void);
_DLLEXPORT int subscribePropget(void);
_DLLEXPORT int subscribePropResp(void);
_DLLEXPORT int subscribeSubDeviceEvent(void);

#endif /* INCLUDE_SUBSCRIBE_H_ */
