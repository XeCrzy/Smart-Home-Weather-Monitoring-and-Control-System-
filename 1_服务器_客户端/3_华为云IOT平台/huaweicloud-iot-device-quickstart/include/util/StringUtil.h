#ifndef INCLUDE_UTIL_STRINGUTIL_H_
#define INCLUDE_UTIL_STRINGUTIL_H_

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
#undef size_t
#ifdef _WIN64
typedef unsigned long long size_t;
#else
typedef unsigned int size_t;
#endif /* _WIN64 */
#endif /* _SIZE_T_DEFINED */


int stringlength(char *str);
char* strInStr(const char *_Str, const char *_SubStr);
int string2int(char *value);
void * strMemSet(void *_Dst, int _Val, size_t _Size);
void memFree(char **str);
char * combineStrings(int strAmount, char * str1, ...);
void stringMalloc(char **str, int length);
void copyStrValue(char **dst, char *src, int length);
char* getClientTimeStamp(void);
size_t constStringlength(const char *_Str);
char* getEventTimeStamp(void);

#endif /* INCLUDE_UTIL_STRINGUTIL_H_ */
