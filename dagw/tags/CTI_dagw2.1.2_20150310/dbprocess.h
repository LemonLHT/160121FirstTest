#ifndef __DB_PROCESS_H
#define __DB_PROCESS_H

#include "cacti/kernel/Thread.h"
#include "cacti/util/BoundedQueue.h"
#include "cacti/message/TransferMessage.h"
#include "cacti/mtl/ServiceSkeleton.h"
#include "dbconnect/dbconnect.h"


const int DAGW_PROCESS_QUEUE_LEN = 512;

using namespace cacti;

class TDBGWService;

class  TDBProcess : public Thread 
{
public:
	TDBProcess(TDBGWService *owner, BaseQuery *pDbQuery, int iThreadNo);
	~TDBProcess();

	bool Process(UserTransferMessage &utm);
	int	 GetIndex() { return m_iThreadNo; };
	BaseQuery* GetBaseQuery(){return m_pDbQuery;};
	void SetBaseQuery(BaseQuery* baseQuery){m_pDbQuery=baseQuery;};
	void Stop();
	void ReconnectProcess();//增加一个线程用于重连

	void errProcess( BaseException & err,string sourceName);
private:
	virtual void onMessage(const ServiceIdentifier& sender, UserTransferMessage& utm);
	virtual void onDefaultMessage(const ServiceIdentifier& sender, UserTransferMessage& utm);
	virtual void run();

	void OnExecSQL(const ServiceIdentifier&sender,UserTransferMessage& utm);
	bool PreExecSQL(const ServiceIdentifier&sender,UserTransferMessage& utm, string &sql);

	void PrintLog(const char* format,...);

	BoundedQueue<UserTransferMessage> m_queue;
	TDBGWService *m_owner;
	BaseQuery *m_pDbQuery;
	int  m_iThreadNo;
	bool m_bStop;
	u32 m_rowCount;
	cacti::Thread m_thread;
	TimerID m_TimerID;  

	int count_msg;  //计数器,如果大于最大次数异常后才开始重连
};


#endif //__DB_PROCESS_H