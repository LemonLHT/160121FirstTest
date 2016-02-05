#ifndef _CONVERT_PROCESS_H_
#define _CONVERT_PROCESS_H_

#include <string>
#include <vector>
#include "cacti/logging/Logger.h"
#include "cacti/kernel/Thread.h"
#include "cacti/util/BoundedQueue.h"
#include "cacti/message/TransferMessage.h"
#include "cacti/mtl/ServiceSkeleton.h"
#include "db/dbprocess.h"
#include "db/cqueue.h"
#include "db/dbprocesssession.h"
#include "db/cthread.h"
#include "db/typedef.h"
#include "db/cobject.h"

using namespace std;
using namespace cacti;
class DBProcess;

#define PERCOUNT m_count
#define MAXCONN  16
//The billExport the process of class.
typedef Comm::CQueue<dbprocessession *> cq_dbsession;

class ConverProcess
{
public:
	ConverProcess();
	~ConverProcess();
public:
	bool init(); //Initialization.
	void uninit();//Reverse initialization. 

	void stop();//Stop the thread.
	virtual bool run();
	bool get_tabname();
	bool start_dbprocess();
	inline cq_dbsession Get_session()
	{
		Comm::CScopeLock lock(m_MulLock);
		return m_dbsession;
	}
private:
	Comm::CLock			m_MulLock;
private:
	vector<string>			m_tabname_oracl;
	vector<string>			m_tabname_sy;
	// for peer database
	vector<string>			m_tab_orcl;
	vector<string>			m_tab_sy;
	vector<string>          m_col_orcl;
	vector<string>			 m_col_sy;

	Logger&					m_logger; //Log Print object.
	vector<DBProcess*>  m_dbProcess;
	cq_dbsession				m_dbsession;
	cq_dbsession			m_m_dbsession_tmp;
	int								m_count;
};
#endif