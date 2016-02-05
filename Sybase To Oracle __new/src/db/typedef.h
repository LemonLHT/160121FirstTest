#ifndef __TYPEDEFINE_H__
#define __TYPEDEFINE_H__

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef long int32_t;
typedef unsigned long uint32_t;

#ifdef _MSC_VER
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef void *HANDLE;
#endif

#include <assert.h>

#if (defined(_MSC_VER) && defined(_DEBUG))
#pragma warning(disable: 4786)	//warning C4786: identifier was truncated to '255' characters in the browser information
#endif

#if (defined(_MSC_VER))
#pragma warning(disable:4355) //warning C4355: 'this' : used in base member initializer list
#endif

#endif 