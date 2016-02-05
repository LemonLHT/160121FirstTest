#ifndef _DB_PROCESS_H_
#define _DB_PROCESS_H_

#include "dbconnect/dbconnect.h"
#include "cacti/message/TypeDef.h"
#include "dbconnect/base/baseQuery.h"
#include "cacti/util/IniFile.h"
#include <string>
#include <vector>
#include "cqueue.h"
#include "dbprocesssession.h"
#include "cthread.h"
#include "typedef.h"
#include "main/convertprocess.h"



#define MAXCOLUMS  1000  // 
struct Tabtype
{
	string col_name;
	string col_type;
	Tabtype()
	{
		col_name="";
		col_type="";
	}
	void clear() //clear the dbparam
	{
		col_name.clear();
		col_type.clear();
	}
};

typedef vector<Tabtype> vec_ttype;
typedef Comm::CQueue<dbprocessession *> cq_dbsession;


class ConverProcess;

class DBProcess
{
public:
	DBProcess();
	DBProcess(vector<string> & orcl, vector<string> sy, int id,dbprocessession * cp);
	DBProcess(vector<string> & orcl, vector<string> sy,vector<string> col_orcl,vector<string> col_sy, char ch,dbprocessession * cp);
	~DBProcess();
public:
	bool init(); //init the class.
	void uninit(); //uninit the class.
	void get_col_type(string tabname);// get table name and type.............
	void get_col_type_sy(string tabname);// get table name and type.............
	void makesql(string tabname);   //  MAKE  SQL
	bool makesql_sec(string tabname,string col);   //  MAKE  SQL
	inline void getsql(char *sql){strncpy(sql,m_str,sizeof(sql));}
	bool executeUpdate(const char* sql); //update the table.
	bool conversion_start(unsigned long timeout); //  PTHREAD FUNCTION
	bool conversion_start_another(unsigned long timeout);
	bool droptable(string &name);//   DROP TAB_BAK   AND CREATE   TAB_BAK
	inline bool getflag(){return m_flag;}//   process running flag
private:
	void Run() {	/*线程运行函数*/
		while (m_pThread && m_pThread->Running())
		{
			if(!conversion_start(20))
			{	
				Service::printConsole("[%d] Thread running Failed, going to be end!!!\n",processId);
			}
			delete m_pThread;
			m_pThread = NULL;
			m_flag = true;
			break;
		}
		return;
	}
	static uint32_t __stdcall _sRun(void *p) {	/*线程函数*/
		if (p == NULL)
			return 0;
		((DBProcess*)p)->Run();
		return 0;
	}
private:
	void RUN() {	/*线程运行函数*/
		while (m_pThread && m_pThread->Running())
		{
			if(!conversion_start_another(20))
			{	
				Service::printConsole("[%c] Thread running Failed, going to be end!!!\n",process_id);
			}
			delete m_pThread;
			m_pThread = NULL;
			m_flag = true;
			break;
		}
		return;
	}
	static uint32_t __stdcall __sRun(void *p) {	/*线程函数*/ // ------ 太过于繁琐。由于开始思路问题遗留
		if (p == NULL)
			return 0;
		((DBProcess*)p)->RUN();
		return 0;
	}
private:
	cacti::Logger& m_logger;
	char m_str[1024];
	vec_ttype v_type;
	vec_ttype v_type_sy;
	dbprocessession  *m_dbmanager;//  GET SOURCE
	//cq_dbsession  m_dbsession;
	Comm::CThread * m_pThread;	
	vector<string>  m_orcl;//TABNAME
	vector<string> m_sy;
	//
	vector<string>				m_an_orcl;
	vector<string>				m_an_sy;
	vector<string>				m_col_orcl;
	vector<string>				m_col_sy;
	//
	int processId;//  pthread ID
	char process_id;
	bool  m_flag;//process running flag
};
#endif