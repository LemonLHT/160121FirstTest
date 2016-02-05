#ifndef __CQUEUE_H__
#define __CQUEUE_H__

#include "cobject.h"

COMM_BEGIN_NAMESPACE

#define QUEUE_IDLECOUNT  0
#define QUEUE_MAXCOUNT   4096

template <class T>
class CQueue
{
public:
	CQueue(int size = QUEUE_MAXCOUNT);
	~CQueue();
	bool Peek(T& data);
	bool Put(T& data);
	bool TryPut(T& data);
	bool PutHead(T& data);
	bool Get(T& data);
	bool TryGet(T& data, int waittime = INFINITE);
	int  GetCount();
	bool Clear();
	
private:
	T*   Alloc();
	void Dealloc(T*& data);

private:
	CLock m_MulLock;
	CSemaphore m_MinLock;
	int m_MaxQueueSize;
	int m_Head;
	int m_Tail;
	int m_Count;
	int m_IdleCount;
	T** m_Table;
	T** m_IdleTable;
};

template <class T>
CQueue<T>::CQueue(int size)
: m_MaxQueueSize(size),m_MinLock(0, size),m_Head(0),m_Tail(0)
{
	m_Table = new T*[m_MaxQueueSize+1];
	m_IdleTable  = new T*[QUEUE_IDLECOUNT+1];
	assert(m_Table && m_IdleTable);
	memset(m_Table, 0, sizeof(T*)*m_MaxQueueSize);
	memset(m_IdleTable, 0, sizeof(T*)*(QUEUE_IDLECOUNT+1));
	m_Count = 0;
	m_IdleCount = 0;
}

template <class T>
bool CQueue<T>::Peek(T& data)
{
	CScopeLock lock(m_MulLock);
	if (m_Table[m_Head] == (T*)0)
		return false;
	data = *m_Table[m_Head];
	return true;
}

template <class T>
bool CQueue<T>::Put(T& data)
{
	CScopeLock lock(m_MulLock);

	if (m_Count >= m_MaxQueueSize)
		return false;

	if (m_Table[m_Tail] != (T*)0)
		return false;

	m_Table[m_Tail] = Alloc();//new T();
	assert(m_Table[m_Tail]);
	*m_Table[m_Tail] = data;
	m_Tail = (m_Tail+1)%m_MaxQueueSize;
	m_Count++;

	m_MinLock.Post();
	return true;
}


template <class T>
bool CQueue<T>::TryPut(T& data)
{
	return Put(data);
}

template <class T>
bool CQueue<T>::PutHead(T& data)
{
	CScopeLock lock(m_MulLock);

	if (m_Count >= m_MaxQueueSize)
		return false;

	m_Head = (m_Head == 0 ? m_MaxQueueSize-1 : m_Head-1);

	if (m_Table[m_Head] !=(T*)0 )
		return false;

	m_Table[m_Head] = Alloc();//new T();
	assert(m_Table[m_Head]);
	*m_Table[m_Head] = data;
	m_Count++;

	m_MinLock.Post();
	return true;
}

template <class T>
bool CQueue<T>::Get(T& data)
{
	return TryGet(data, INFINITE);
}

template <class T>
bool CQueue<T>::TryGet(T& data, int waittime/* = INFINITE*/)
{
	//¥À¥¶µ»10∫¡√Î
	if (!m_MinLock.Wait(waittime))
		return false;

	CScopeLock lock(m_MulLock);

	if (m_Table[m_Head] == (T*)0)
		return false;

	data = *m_Table[m_Head];
	//delete m_Table[m_Head];
	Dealloc(m_Table[m_Head]);
	m_Table[m_Head] = (T*)0;
	m_Head = (m_Head+1)%m_MaxQueueSize;
	m_Count--;

	return true;
}

template <class T>
int CQueue<T>::GetCount()
{
	CScopeLock lock(m_MulLock);
	return m_Count;
}

template <class T>
bool CQueue<T>::Clear()
{
	CScopeLock lock(m_MulLock);
	
	T data;
	while (TryGet(data, 0))
		NULL;

	assert(m_Count == 0);
	assert(m_Head == m_Tail);
	
	return 0;
}

template <class T>
CQueue<T>::~CQueue()
{
	CScopeLock lock(m_MulLock);
	int i = 0;
	for (i=0; i<m_MaxQueueSize;i++)
	{
		if (m_Table[i])
			delete m_Table[i];
	}
	for (i=0; i<QUEUE_IDLECOUNT; i++)
	{
		if (m_IdleTable[i])
			delete m_IdleTable[i];
	}
	delete[] m_Table;
	delete[] m_IdleTable;
}

template <class T>
T* CQueue<T>::Alloc()
{
	T* data = NULL;
	if (m_IdleCount == 0)
		data = new T();
	else
	{
		m_IdleCount--;
		data = m_IdleTable[m_IdleCount];
		m_IdleTable[m_IdleCount] = 0;
	}
	assert(data);
	return data;
}

template <class T>
void CQueue<T>::Dealloc(T*& data)
{
	if (m_IdleCount >= QUEUE_IDLECOUNT)
		delete data;
	else
		m_IdleTable[m_IdleCount++] = data;
	data = NULL;
}

COMM_END_NAMESPACE

#endif