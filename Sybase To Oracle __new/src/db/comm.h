#ifndef __COMM_H__
#define __COMM_H__

#include "typedef.h"

#define COMM_NAMESPACE Comm
#define COMM_BEGIN_NAMESPACE namespace COMM_NAMESPACE {
#define COMM_END_NAMESPACE }

//编码转换句柄定义
typedef void * codec_hd;

//字节序强制转换
#define	SWAPLONG(y) \
	((((y)&0xff)<<24) | (((y)&0xff00)<<8) | (((y)&0xff0000)>>8) | (((y)>>24)&0xff))
#define	SWAPSHORT(y) \
	( (((y)&0xff)<<8) | ((uint16_t)((y)&0xff00)>>8) )

#endif