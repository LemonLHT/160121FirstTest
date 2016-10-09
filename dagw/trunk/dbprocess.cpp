#include "stdafx.h"
#include "dbprocess.h"
#include "dagwservice.h"
#include "Service.h"
#include "cacti/util/Timestamp.h"
#include "cacti/logging/LazyLog.h"
#include "tag.h"
#include "reqmsg.h"
#include "resmsg.h"
#include "sxcode.h"

#ifdef _DB_ORACLE_ 
#include "dbconnect\driver_oracle\oracleconnection.h"
#include "dbconnect\driver_oracle\oraclequery.h"
#endif

#ifdef _DB_SYBASE_ 
#include "dbconnect\driver_sybase\sybaseConnection.h"
#include "dbconnect\driver_sybase\sybasequery.h"
#endif

#ifdef FOR_ODBC 
#	include "dbconnect\driver_odbc\odbcconnection.h"
#	include "dbconnect\driver_odbc\odbcquery.h"
#endif

extern cacti::LazyLog g_dagwlog;
extern CDAGWCfgData g_cfgData;

MTL_BEGIN_MESSAGE_MAP(TDBProcess)
MTL_ON_MESSAGE(_ReqExecSQL,OnExecSQL)
MTL_END_MESSAGE_MAP


TDBProcess::TDBProcess(TDBGWService *owner,BaseQuery *pDbQuery, int iThreadNo)
:m_queue(DAGW_PROCESS_QUEUE_LEN)
,m_owner(owner)
,m_pDbQuery(pDbQuery)
,m_iThreadNo(iThreadNo)
,m_bStop(false)
,m_rowCount(0)
,count_msg(0)
{

}

TDBProcess::~TDBProcess()
{
	Stop();
}
void  TDBProcess::onDefaultMessage(const ServiceIdentifier& sender, UserTransferMessage& utm)
{
}

void TDBProcess::run()
{
	Service::printConsole("[%04d] DBGW Process running OK\n",m_iThreadNo);
	//m_thread.start(this,&TDBProcess::ReconnectProcess);
	UserTransferMessage utm;
	while( !m_bStop )
	{
		g_dagwlog.print("[%04d] try to get msg\n", m_iThreadNo );
		//m_queue.get(utm);
		if (false == m_queue.tryGet(utm))
		{
			sleep(50);
			continue;
		}
		g_dagwlog.print("[%04d] get msg ok\n", m_iThreadNo );
		onMessage( utm.getReq(), utm);
	}
	Service::printConsole("[%04d] DBGW Process quit\n",m_iThreadNo);
}

void TDBProcess::Stop()
{
	g_dagwlog.print("[%04d] Stop DBProcess\n", m_iThreadNo);
	if( !m_bStop)
	{
		m_bStop = true;
		UserTransferMessage utm;
		m_queue.put(utm);
		join();
	}
}

bool TDBProcess::Process(UserTransferMessage &utm)
{
	return m_queue.tryPut(utm);
}

void TDBProcess::ReconnectProcess()
{
	UserTransferMessage utm;
	string sql="select * from dual";
	utm.setMessageId(_ReqExecSQL);
	utm[_TagSql]=sql;
	while( !m_bStop )
	{
		g_dagwlog.print("keep connect!\n");
		Process(utm);
		Sleep(60*1000);
	}
}

void TDBProcess::OnExecSQL(const ServiceIdentifier&sender,UserTransferMessage& utm)
{
	string sql = utm[_TagSql];
	string sourceName=utm[_TagSourceName];
	strarray result ;
	strarray fieldname;

	BaseFieldDescription *pField;
	int rowcount = 0;
	u32 fieldcount = 0;
	bool	tryconnect	= false;

	utm.setRes(utm.getReq());
	utm.setMessageId(_ResExecSQL);
	PrintLog("RECV ReqExecSQL  sql = %s \n",sql.c_str() );

	if(!PreExecSQL(sender,utm, sql ) )
	{
		PrintLog("Check SQL Statement Error\n");
		return;	
	}
	try 
	{
		m_pDbQuery->command(sql);
		for(u32 i= 1 ; i <= m_pDbQuery->outputCount() ; i++)
		{
			m_pDbQuery->registerOutParam(i,FT_STRING);
		}
		PrintLog("Debug DO ExecSQL\n");
		m_pDbQuery->execute();
		PrintLog("Debug Finish ExecSQL\n");
		fieldcount = (u32) m_pDbQuery->fieldCount();
		utm[_TagFieldCount] = fieldcount;
		while ( !m_pDbQuery->eof())
		{
			rowcount++;
			m_pDbQuery->fetchNext();
			for(u32 i= 0 ; i < fieldcount ; i++)
				result.push_back( m_pDbQuery->getFieldByColumn(i)->asString());
		}
		if( 0 == rowcount )
		{
			for(u32 i=1; i <= m_pDbQuery->outputCount() ; i++ )
				result.push_back(m_pDbQuery->getOutParamByIndex(i)->asString());
			utm[_TagFieldCount] = (u32) m_pDbQuery->outputCount();
		}
		utm[_TagResult] = result;
		if( fieldcount )
		{
			for(u32 i = 0; i< fieldcount ; i++)
			{
				pField = m_pDbQuery->getFieldInfoByColumn(i);
				if( pField)
					fieldname.push_back(pField->name());
			}
		}
		utm[_TagFieldName] = fieldname;
	}
#ifdef _DB_ORACLE_ 
		catch( BaseException &err )
	{		
		Service::printConsole(cacti::LogLevel::FATAL, "%s %s\n",err.name.c_str(),err.description.c_str());
		utm.setReturn(ERR_DBS_ERROR);
		PrintLog("SEND ResExecSQL  failed, Exec SQL %s %s\n",
			err.name.c_str(),err.description.c_str());
		errProcess(err,sourceName);
		m_rowCount = 0;
		m_owner->postMessage(sender, utm);
		return ;
	}
#endif
#ifdef _DB_SYBASE_ 
	catch( BaseException &err )
	{
		if ( utm.getReturn() == 0 ){
			//	check the error information.
			//	if the communication is lost, reconnect it.
			if ( err.state == "08S01" || err.state == "HY000"){
				tryconnect	= true;
			};
		};		
		if ( ! tryconnect ){
			Service::printConsole(cacti::LogLevel::FATAL, "%s %s\n",err.name.c_str(),err.description.c_str());
			utm.setReturn(ERR_DBS_ERROR);
			PrintLog("SEND ResExecSQL  failed, Exec SQL %s %s\n",
				err.name.c_str(),err.description.c_str());
			errProcess(err,sourceName);
			m_owner->postMessage(sender, utm);
			return ;
		}
	}
#endif
#ifdef FOR_ODBC 
		catch( BaseException &err )
		{
			if ( utm.getReturn() == 0 ){
				//	check the error information.
				//	if the communication is lost, reconnect it.
				if ( err.state == "08S01" || err.state == "HY000"){
					tryconnect	= true;
				};
			};		
			if ( ! tryconnect ){
				Service::printConsole(cacti::LogLevel::FATAL, "%s %s\n",err.name.c_str(),err.description.c_str());
				utm.setReturn(ERR_DBS_ERROR);
				PrintLog("SEND ResExecSQL  failed, Exec SQL %s %s\n",
					err.name.c_str(),err.description.c_str());
				errProcess(err,sourceName);
				m_owner->postMessage(sender, utm);
				return ;
			}
		}
	if ( tryconnect ){
		ODBCQuery*	oq	= (ODBCQuery*)m_pDbQuery;
		try{
			oq->getParentConnection()->_odbcReconnect( oq->getIndex() );
		}
		catch( BaseException& err ){
			Service::printConsole(cacti::LogLevel::FATAL, "%s %s\n",err.name.c_str(),err.description.c_str());
			utm.setReturn(ERR_DBS_ERROR);
			PrintLog("SEND ResExecSQL  failed, Exec SQL %s %s\n",
				err.name.c_str(),err.description.c_str());
			errProcess(err,sourceName);
			m_owner->postMessage(sender, utm);
			return ;
		};
		utm.setMessageId( _ReqExecSQL );
		utm.setReturn( utm.getReturn() + 1 );
		Process( utm );
		return;
	};
	utm.setReturn( 0 );
#endif

	PrintLog("SEND ResExecSQL  fieldcount =%d, rowcount=%d\n",fieldcount,rowcount);
	m_owner->postMessage(sender, utm);

	return;
}
bool TDBProcess::PreExecSQL(const ServiceIdentifier&sender,UserTransferMessage& utm,string &sql)
{
	bool	ret	= true;
	//empty string will cause sybase api crash
	if(sql == "")
	{
		utm.setReturn(ERR_DBS_ERROR);
		PrintLog("SEND ResExecSQL  failed,sql is null\n");
		m_owner->postMessage(sender, utm);
		return false;
	}	
	//just for sybase, need change for all database
#ifdef  _DB_SYBASE_  
	if( sql.find("rowcount")!= string::npos  &&
		sql.find("set") !=string::npos )
	{
		utm.setReturn(ERR_DBS_ERROR);
		PrintLog("SEND ResExecSQL  failed,sql can not set row count\n");
		m_owner->postMessage(sender, utm);
		return false;
	}
	u32 rowCount = utm[_TagMaxRowCount];
	if (rowCount == 0 || rowCount > g_cfgData.m_MaxRowCount )
		rowCount = g_cfgData.m_MaxRowCount;

	if ( rowCount != m_rowCount || m_pDbQuery->effectCount() > m_rowCount ) 
	{
		m_rowCount = rowCount;
		try
		{
			char buf[256];
			sprintf(buf,"set rowcount %d", m_rowCount);
			m_pDbQuery->command(buf);
			m_pDbQuery->execute();
			PrintLog("%s\n", buf);
		}
		catch(BaseException& excp)
		{
			utm.setReturn( ERR_DBS_ERROR );
			PrintLog("SEND ResExecSQL failed, %s:%s\n",
				excp.name.c_str(),
				excp.description.c_str() );
			m_owner->postMessage(sender, utm);
			ret	=false;
		};

	}

#endif
	return ret;
}

void TDBProcess::PrintLog(const char *format,...)
{
	va_list va;
	va_start(va, format);
	string logstring  = StringUtil::vform(format, va);
	g_dagwlog.print("[%04d] %s",m_iThreadNo, logstring.c_str());
	va_end(va);
}
void TDBProcess::errProcess( BaseException & err,string sourceName)
{
	g_dagwlog.print("[%04d]errProcess: errcode(%d),%s %s,sourceName:%s\n",m_iThreadNo,err.code,err.name.c_str(),err.description.c_str(),sourceName.c_str());
	switch(err.code)
	{
	case DBERROR:
		{
			if (strstr(err.description.c_str(),"ORA-00942")==NULL)
			{
				g_dagwlog.print("[%04d]errProcess: found dbError, reconnect start...\n",m_iThreadNo);
				m_owner->reconnect(sourceName);
			}
		}
		break;
	case NOT_CONNECTED:
	case ERROR_CONNECTING:
	case QUERY_CONNECTION_TIMEOUT:
	case ERROR_PINGING_CONNECTION:
		{
			g_dagwlog.print("[%04d]errProcess: found dbError, reconnect start...\n",m_iThreadNo);
			m_owner->reconnect(sourceName);
		}	
		break;
	case ERROR_QUERYING:
		{
			if (31!=strlen(err.description.c_str()))
			{
				g_dagwlog.print("[%04d]errProcess: found dbError, reconnect start...\n",m_iThreadNo);
				m_owner->reconnect(sourceName);
			}
		}
		break;
	default:
		break;
	}
}