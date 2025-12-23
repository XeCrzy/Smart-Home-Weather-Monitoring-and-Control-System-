#ifndef LOGIN_H_
#define LOGIN_H_

_DLLEXPORT void onConnectionLost(void *context, char *cause);
_DLLEXPORT int createConnection(void);
_DLLEXPORT int releaseConnection(void);

#endif /*LOGIN_H_*/
