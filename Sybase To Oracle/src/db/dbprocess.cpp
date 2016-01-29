#include "stdafx.h"
#include "dbprocess.h"
#include "cacti/util/IniFile.h"
#include "main/convertService.h"

DBProcess::DBProcess()
:m_logger(Logger::getInstance("billupload")), m_query_sy(NULL), m_query_orcl(NULL),m_dbConnection_orcl(NULL),m_dbConnection_sy(NULL),m_activate_sy(false),m_activate_orcl(false)
{
	memset(m_str,0,sizeof(m_str));

}

DBProcess::~DBProcess()
{

}

bool DBProcess::init()
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
		m_dbConnection_orcl->setPingInterval(m_dbParam.interval); // set ping interval.
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
		m_dbConnection_sy->setPingInterval(m_dbParam.interval); // set ping interval.
		m_query_sy = m_dbConnection_sy->requestQueryConnection();
		m_activate_sy = true;
	}
	catch( BaseException &err )
	{
		m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
		Service::printConsole("Connect Database failed\n");
		return false;
	}


	Service::printConsole("Connect database success\n");
	m_logger.info("NOTE:Connect database success\n");
	return true;
}

void DBProcess::uninit()
{
	if(m_query_sy){ 
		m_dbConnection_sy->releaseQueryConnection(m_query_sy);
		
	}
	if(m_query_orcl)
	{
		m_dbConnection_orcl->releaseQueryConnection(m_query_sy);
	}
	if(m_activate_sy){
		m_dbConnection_sy->disconnect(5);
		
	}
	if(m_activate_orcl)
	{
		m_dbConnection_orcl->disconnect(5);
	}
	delete m_dbConnection_sy;
	delete m_dbConnection_orcl;
}

void DBProcess::get_col_type()
{

	int colindex;
	char tmp[1024] = { 0 };
	Tabtype tmpttype;
	snprintf(tmp,1024,"select t.column_name,t.data_type,t.column_id  FROM user_tab_cols t,user_col_comments c WHERE lower(t.table_name) = lower('%s') and c.table_name = t.table_name and c.column_name = t.column_name and t.hidden_column = 'NO' order by t.column_id",m_dbParam.tabname_orcl.c_str());

	m_query_orcl->command(tmp);
	m_query_orcl->execute();

	if(m_query_orcl->eof())
		return;
	try
	{
		while(!m_query_orcl->eof())
		{
			colindex=0;
			tmpttype.clear();

			m_query_orcl->fetchNext();
			tmpttype.col_name = m_query_orcl->getFieldByColumn(colindex++)->asString();
			tmpttype.col_type = m_query_orcl->getFieldByColumn(colindex++)->asString();
			v_type.push_back(tmpttype);

		}
	}
	catch(BaseException& err)
	{
		m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
		return;
	}	
}
//get sql  test 
void DBProcess::makesql()
{
	get_col_type();
	char tmp[1024] = { 0 };
	char str[1024] = { 0 };
	memset(str,0,sizeof(tmp));
	strcpy(str,"select ");
	int i;
	for (i = 0;i < v_type.size();i++)
	{
		memset(tmp,0,sizeof(tmp));
		if( (strcmp(v_type[i].col_type.c_str(),"DATE") == 0) || (strcmp(v_type[i].col_type.c_str(),"TIMESTAMP(6)") == 0))
		{
			snprintf(tmp,1024," convert(varchar(20),%s,20) ",v_type[i].col_name.c_str());
			dateindex++;
		}
		else
		{
			if( (strcmp(v_type[i].col_type.c_str(),"CHAR") == 0) || (strcmp(v_type[i].col_type.c_str(),"VARCHAR2") == 0))
				strindex++;
			if( strcmp(v_type[i].col_type.c_str(),"NUMBER") == 0)
				numindex++;

			snprintf(tmp,1024," %s ",v_type[i].col_name.c_str());
		}
		
		strcat(str, tmp);
		if (i != (v_type.size()-1))
		{
			strcat(str," , ");
		}
			
	}
	snprintf(m_str,1024,"%s from %s",str,m_dbParam.tabname_sy.c_str());

}

bool  DBProcess::conversion_start()
{
	char sql[1024] = { 0 };
	char tmp[1024] = { 0 };
	char ttmp[1024] = { 0 };
	int i = 0;
	int index = 0;
	snprintf(ttmp,1024, "insert into %s values ( ",m_dbParam.tabname_orcl.c_str());

	
	try
	{
		

		m_query_sy->command(m_str);
		m_query_sy->execute();

		if(m_query_sy->eof())
			return false;			
		int colindex;
		while(!m_query_sy->eof())
		{
			colindex=0;



			m_query_sy->fetchNext();
			{
				for (i = 0;i < v_type.size();i++)
				{
					if( (strcmp(v_type[i].col_type.c_str(),"DATE") == 0) || (strcmp(v_type[i].col_type.c_str(),"TIMESTAMP(6)") == 0))
					{
						snprintf(tmp,1024," %s ",m_query_sy->getFieldByColumn(colindex++)->asString());
					}
					else
					{
						if( (strcmp(v_type[i].col_type.c_str(),"CHAR") == 0) || (strcmp(v_type[i].col_type.c_str(),"VARCHAR2") == 0))
							snprintf(tmp,1024," %s ",m_query_sy->getFieldByColumn(colindex++)->asString());
						
						if( strcmp(v_type[i].col_type.c_str(),"NUMBER") == 0)
							snprintf(tmp,1024," %f ",m_query_sy->getFieldByColumn(colindex++)->asFloat());
					}
					if(i != v_type.size())
					{
						snprintf(tmp, 1024," , ");
					}
					else
					{
						snprintf(tmp, 1024," ) ");
					}
					

				}
			}
			snprintf(sql,1024,"%s %s",ttmp,tmp);

			m_query_orcl->command(sql);
			m_query_orcl->execute();
			index ++;
			if(index == 100)
			{
				m_query_orcl->command("commit");
				m_query_orcl->execute();
				
			}

	
		}

		m_query_orcl->command("commit");
		m_query_orcl->execute();


	}

	catch(BaseException& err)
	{
		m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
		return false ;
	}


	return  true ;
}
/*
bool DBProcess::executeUpdate(const char* sql)
{	
	try
	{
		if(!m_activate)
		{
			if(m_query->reconnect())
				m_activate=true;
			else
				m_activate=false;

			return false;
		}

		m_query->command(sql);
		m_query->execute();

		unsigned int effectcount=m_query->effectCount();	

		m_logger.info("SQL:%s,effectcount=%d.\n",sql,effectcount);

		m_activate=true;
		return true;
	}
	catch(BaseException& err)
	{
		m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());

		switch(err.code)
		{
		case NOT_CONNECTED:
		case ERROR_CONNECTING:
		case QUERY_CONNECTION_TIMEOUT:
		case ERROR_PINGING_CONNECTION:
			{
				m_activate=false;
			}	
			break;

		default:
			break;
		}

		return false;
	}

}
*/