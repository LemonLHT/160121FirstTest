#ifndef _DBPROCESS_SESSION_H_

#define _DBPROCESS_SESSION_H_

#include "dbconnect/dbconnect.h"
#include "cacti/message/TypeDef.h"
#include "dbconnect/base/baseQuery.h"
#include "cacti/util/IniFile.h"
#include <string>
#include <vector>
#include "dbprocess.h"
#include "main/convertService.h"

using namespace std;
using namespace cacti;

struct DBParam //database parameter.
{
	string source;
	string name;
	string user;
	string password;
	time_t interval;
	int maxCon;
	string tabname_sy;
	string tabname_orcl;

	DBParam()
	{
		source="";
		name="";
		user="";
		password="";
		interval=0;
		tabname_sy="";
		tabname_orcl="";
		maxCon=0;
	}

	void clear() //clear the dbparam
	{
		source.clear();
		name.clear();
		user.clear();
		password.clear();
		tabname_sy.clear();
		tabname_orcl.clear();
		interval=0;
	}

	bool load_sy(char* filename) //load the dbparam.
	{
		IniFile cfgFile;
		if(!cfgFile.open(filename))
			return false;

		source = cfgFile.readString("Database_sy", "Source", "");
		name = cfgFile.readString("Database_sy", "Name", "");
		user = cfgFile.readString("Database_sy", "User", "");
		password = cfgFile.readString("Database_sy", "Password", "");
		interval     = cfgFile.readInt("Database_sy","Interval",10);
		maxCon   =cfgFile.readInt("Database_sy","maxCon",1);
		cfgFile.clear();

		if(user.empty()) //--------------here should change...
			return false;

		return true;		
	}


	bool load_orcl(char* filename) //load the dbparam.
	{
		IniFile cfgFile;
		if(!cfgFile.open(filename))
			return false;

		source = cfgFile.readString("Database_orcl", "Source", "");
		name = cfgFile.readString("Database_orcl", "Name", "");
		user = cfgFile.readString("Database_orcl", "User", "");
		password = cfgFile.readString("Database_orcl", "Password", "");
		interval     = cfgFile.readInt("Database_orcl","Interval",10);
		maxCon   =cfgFile.readInt("Database_orcl","maxCon",1);
	
		cfgFile.clear();

		if(user.empty()||password.empty())
			return false;

		return true;		
	}
};

class dbprocessession
{
public:
	dbprocessession()
		:m_used(false),m_logger(Logger::getInstance("Conversion")), m_query_sy(NULL), m_query_orcl(NULL),m_dbConnection_orcl(NULL),m_dbConnection_sy(NULL),m_activate_sy(false),m_activate_orcl(false)
	{
		
	}
	bool init()
	{
		m_logger.info("Start DBProcess\n");
		//connect oracle
		if(!m_dbParam.load_orcl(CFG_CONF))
		{
			m_logger.info("Load dbParam failed.\n");
			return false;
		}

		try
		{
			m_dbConnection_orcl = new DbConnection(DbConnection::ODBC);
			m_dbConnection_orcl->connect(m_dbParam.user, m_dbParam.password, m_dbParam.name, m_dbParam.source,m_dbParam.maxCon+1);
			//m_dbConnection_orcl->setPingInterval(m_dbParam.interval); // set ping interval.
			m_query_orcl = m_dbConnection_orcl->requestQueryConnection();
			m_activate_orcl = true;
		}
		catch( BaseException &err )
		{
			m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
			printf("Connect Database failed\n");
			return false;
		}

		//conncet sybase

		if(!m_dbParam.load_sy(CFG_CONF))
		{
			m_logger.info("Load dbParam failed.\n");
			return false;
		}

		try
		{
			//m_dbConnection_sy = new DbConnection(DbConnection::SYBASE);
			m_dbConnection_sy = new DbConnection(DbConnection::ODBC);
			m_dbConnection_sy->connect(m_dbParam.user, m_dbParam.password, m_dbParam.name, m_dbParam.source//,m_dbParam.maxCon+1);
				);
			//m_dbConnection_sy->setPingInterval(m_dbParam.interval); // set ping interval.
			m_query_sy = m_dbConnection_sy->requestQueryConnection();
			m_activate_sy = true;
		}
		catch( BaseException &err )
		{
			m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
			Service::printConsole("Connect Database failed\n");
			return false;
		}


		Service::printConsole("Connect database success oracle: %p  sybase %p \n",m_query_orcl,m_query_sy);
		m_logger.info("NOTE:Connect database success\n");

		return true;
	}
	void unint()
	{
		if(m_query_sy){ 
			m_dbConnection_sy->releaseQueryConnection(m_query_sy);

		}
		if(m_query_orcl)
		{
			m_dbConnection_orcl->releaseQueryConnection(m_query_orcl);
		}
		if(m_activate_sy){
			m_dbConnection_sy->disconnect(5);
			delete m_dbConnection_sy;

		}
		if(m_activate_orcl)
		{
			m_dbConnection_orcl->disconnect(5);
			delete m_dbConnection_orcl;
		}
	}
	inline BaseQuery * getquery_orcl(){return m_query_orcl;}
	inline BaseQuery * getquery_sy(){return m_query_sy;}
private:
	DbConnection* m_dbConnection_sy; 
	DbConnection* m_dbConnection_orcl; 
	BaseQuery* m_query_sy;
	BaseQuery* m_query_orcl;	
	cacti::Logger& m_logger;

	bool m_activate_sy; //activate flag.
	bool m_activate_orcl;
	bool m_used;

	DBParam m_dbParam;
};



#endif 
