#ifndef __CTHREAD_H__
#define __CTHREAD_H__

#ifdef WIN32
#include <windows.h>
#endif

#include "comm.h"

COMM_BEGIN_NAMESPACE

typedef uint32_t (__stdcall * COMM_THREADFUNC)(void *);
//线程定义
class CThread
{
public:
	CThread(COMM_THREADFUNC startaddress, void* arglist = NULL, bool initflag = false);
	~CThread();
	
	uint32_t	GetId();
	HANDLE		GetHandle();
	uint32_t	Suspend();
	uint32_t	Resume();
	bool		Kill(uint32_t exitCode);
	bool		Exit(uint32_t exitCode);
	bool		IsValid();
	bool		SetPriority(int pri);
	int			GetPriority();
	bool		Running();
	void		Stop();
	
	void        TryStop();  //通知退出
	
	bool		m_StopFlag;   //线程停止标识

	
private:

	HANDLE		m_Handle;
	uint32_t	m_ThreadId;

	bool		m_Running;
};

COMM_END_NAMESPACE

#endif