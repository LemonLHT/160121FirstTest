#ifndef __COBJECT_H__
#define __COBJECT_H__

#include <windows.h>
#include "comm.h"

COMM_BEGIN_NAMESPACE
#include <winbase.h>

/*�ٽ���ʵ�ֵ���*/
class CLock
{
public:
	CLock()  { ::InitializeCriticalSection(&m_cs); }
	~CLock() { ::DeleteCriticalSection(&m_cs); }
	
	void Lock()   { ::EnterCriticalSection(&m_cs); }
	void Unlock() { ::LeaveCriticalSection(&m_cs); }
	bool TryLock() { 
#if(_WIN32_WINNT >= 0x0400)
		return ::TryEnterCriticalSection(&m_cs); 
#else
		return false;
#endif
	}
	bool IsLocked() {
		if (!TryLock())
			return false;
		Unlock();
		return true;
	}
	
private:
	CRITICAL_SECTION m_cs;
};

/*��������Ч��*/
class CScopeLock
{
public:
	CScopeLock(CLock& lock) : m_lock(lock) { m_lock.Lock(); }
	~CScopeLock( ) { m_lock.Unlock(); }
private:
	CLock& m_lock;
};

/*�ں˻���*/
class CObject
{
public:
	CObject() : m_handle(INVALID_HANDLE_VALUE) {}
	virtual ~CObject() { if (m_handle != INVALID_HANDLE_VALUE) ::CloseHandle(m_handle); }

protected:
	HANDLE m_handle;
};

/*������*/
class CMutex : public CObject
{
public:
	CMutex(bool initOwner = false, const char *mxName = NULL) {
		m_handle = ::CreateMutex(NULL, initOwner, mxName);
	}
	
	bool Lock(uint32_t timeout = INFINITE ) { 
		return (WAIT_TIMEOUT != ::WaitForSingleObject(m_handle, timeout)); 
	}	
	bool Unlock() { return (TRUE == ::ReleaseMutex(m_handle)); }
	bool TryLock() { return Lock(0); }
	
	bool IsLocked() { 
		if (!TryLock())
			return true; 
		Unlock();
		return false;
	}
};

/*�ź���*/
class CSemaphore : public CObject
{
public:	
    CSemaphore(int initial = 0, int maxsps = 0x7fffffff, const char* spName = NULL) { 
		m_handle = ::CreateSemaphore(NULL, initial, maxsps, spName); 
	}

    bool Wait(int timeout = INFINITE) {
		return (WAIT_TIMEOUT != ::WaitForSingleObject(m_handle, timeout)); 
	}
    void Post(int count = 1) { ::ReleaseSemaphore(m_handle, count, 0); }
};

/*�¼�*/
class CEvent : public CObject
{
public:
	CEvent(bool manualReset = false, bool initState = true, const char *evName = NULL) {
		m_handle = ::CreateEvent( NULL, manualReset, initState, evName);
	}
	
	bool Set()	 { return ::SetEvent(m_handle)   == TRUE;	} /*����Ϊ����*/
	bool Reset() { return ::ResetEvent(m_handle) == TRUE;	} /*���÷�����״̬*/
	
	bool Wait( uint32_t timeout = INFINITE ) { 
		return WAIT_TIMEOUT != ::WaitForSingleObject(m_handle, timeout); 
	}
};

COMM_END_NAMESPACE

#endif