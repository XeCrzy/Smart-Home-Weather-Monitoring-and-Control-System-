#ifndef _HW_TYPE_H_
#define _HW_TYPE_H_

#if defined(WIN32) || defined(WIN64) && defined(EXPORT_AGENTLITE)
#define HW_API_FUNC __declspec(dllexport)
#else
#define HW_API_FUNC
#endif

#define HW_TRUE      1 /**< Indicates true */
#define HW_FALSE     0 /**< Indicates false */

#define HW_SUCCESS   0 /**< Indicates success */
#define HW_FAILED    1 /**< Indicates failed */
#define HW_NULL     ((void *)0) /**< Indicates null ptr */  

typedef int            HW_INT;  /**< Indicates type of int. */
typedef unsigned int   HW_UINT; /**< Indicates type of unsigned int. */
typedef char           HW_CHAR; /**< Indicates type of char. */
typedef unsigned char  HW_UCHAR;/**< Indicates type of unsigned char. */
typedef int            HW_BOOL; /**< Indicates type of bool. */
typedef void           HW_VOID; /**< Indicates type of void. */

#define _HSTR(_conststr)        ((HW_CHAR *)_conststr)

/**< Indicates type of byte array. */
typedef struct stru_HW_BYTES
{
    HW_UINT   uiLen;     /**< Indicates length of byte array */
    HW_CHAR  *pcByte;    /**< Indicates byte value of byte array */
} HW_BYTES;

#endif
