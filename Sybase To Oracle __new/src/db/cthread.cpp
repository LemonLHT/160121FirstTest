#include "cThread.h"

COMM_BEGIN_NAMESPACE

CThread::CThread(COMM_THREADFUNC start_address, void* arglist, bool initflag)
{
	m_StopFlag = false;
	m_Handle = ::CreateThread(0,0,start_address,arglist,initflag?0:CREATE_SUSPENDED,&m_ThreadId);
	m_Running = true;
}

CThread::~CThread()
{
	Stop();
	CloseHandle(m_Handle);
}

uint32_t CThread::GetId()
{ 
	return m_ThreadId;
}

HANDLE CThread::GetHandle()
{	
	return m_Handle;
}

uint32_t CThread::Suspend()
{
	return ::SuspendThread(m_Handle);
}

uint32_t CThread::Resume()
{
	return ::ResumeThread(m_Handle);
}

/*
 TerminateThread() is a dangerous function that should only be used in the most extreme cases. 
 You should call TerminateThread only if you know exactly what the target thread is doing, 
 and you control all of the code that the target thread could possibly be running at the time
 of the termination. For example, TerminateThread can result in the following problems: 
     * If the target thread owns a critical section, the critical section will not be released. 
     * If the target thread is executing certain kernel32 calls when it is terminated,
	   the kernel32 state for the thread's process could be inconsistent. 
     * If the target thread is manipulating the global state of a shared DLL,
	   the state of the DLL could be destroyed, affecting other users of the DLL. 
*/
bool CThread::Kill(uint32_t exitCode)
{
#ifdef __AFX_H__
	TRACE("!!!! <RTCS WARNING> thread(0x%04x) is killed !!!!\n",m_ThreadId);
#endif
	return (TRUE == ::TerminateThread(m_Handle,exitCode));
}

bool CThread::Exit(uint32_t exitCode)
{
	if (GetCurrentThreadId() == m_ThreadId)	
	{
		::ExitThread(exitCode);
		//_endthreadex(exitCode);

		return true;
	}
	return false;
}

bool CThread::IsValid()
{
	if(m_Handle)
		return true;
	else
		return false;

}

bool CThread::SetPriority(int pri)
{
	return (TRUE == ::SetThreadPriority(m_Handle,pri));
}

int CThread::GetPriority()
{
	return ::GetThreadPriority(m_Handle);

}

bool CThread::Running()
{
	return (m_Running && !m_StopFlag);
}

void CThread::TryStop()
{
	if (Running())
	{
		Resume();
	}
	m_StopFlag = true;
}

void CThread::Stop()
{
	if (Running())
	{
		Resume();
	}
	if( m_Running != false)
	{
		m_Running = false;

		if (GetCurrentThreadId() == m_ThreadId) return; // 不要自己等自己
		if (::WaitForSingleObject(m_Handle, 5000) != WAIT_OBJECT_0)
			Kill(0); //5秒还不停下来就把它KILL掉
		return;
	}
}

COMM_END_NAMESPACE