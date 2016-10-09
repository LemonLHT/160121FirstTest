#include "stdafx.h"
#include <ctype.h>
#include "dagwservice.h"
#include "cacti/logging/LazyLog.h"
#include "dbconnect/dbconnect.h"
#include "cacti/util/IniFile.h"
#include "Service.h"
#include "cacti/util/DailyFile.h"
#include "tag.h"
#include "reqmsg.h"
#include "resmsg.h"
#include "sxcode.h"

cacti::LazyLog g_dagwlog("dagw");
CDAGWCfgData g_cfgData;

MTL_BEGIN_MESSAGE_MAP(TDBGWService)
MTL_ON_MESSAGE(_ReqExecSQL,HandleExecSQL)
MTL_ON_MESSAGE(_ResRegisterService,HandleRespRegister)
MTL_END_MESSAGE_MAP

//	-----------------------------------------------------------------

int     dbfindex( const char* src )     {
        int     ret     = 0;
        const char*     pc= strstr(src, ".dbf");
        if ( pc == NULL )
                pc      = strstr(src, ".DBF");
        if ( pc == NULL)
                pc      = strstr(src, ".Dbf");
        if ( pc == NULL ) return 0;
        while( *pc != ' ' && *pc != '\t' && pc != src ){
                ret     += toupper(*pc);
                pc--;
        };
        return  ret;
};


//	-----------------------------------------------------------------

TDBGWService::TDBGWService(MessageDispatcher * dispatcher )
:ServiceSkeleton( AppPort::_apDagw, dispatcher)
{
	g_dagwlog.open();
	g_cfgData.m_cfglog.open();
	g_cfgData.Load(DAGW_CFG_FILE);	
 }

TDBGWService::~TDBGWService()
{
	uninit();
	Service::printConsole("DAGW Service exit\n");
	g_dagwlog.print("DAGW Service exit\n");
}

bool TDBGWService::init()
{
	try 
	{
		int index = 0;
		for ( u32 i = 0; i < g_cfgData.m_vctConnection.size() ; i++)
		{
			map<string,CDBProcInfo*>::iterator it = 
				m_mapProcess.find(g_cfgData.m_vctConnection[i].m_SourceName);
			
			if( it != m_mapProcess.end() )
				continue;

			
			CDBProcInfo *pDBProcInfo = new CDBProcInfo;
			if( !pDBProcInfo )
				return false;
			pDBProcInfo->m_pDBConnection = new DbConnection( g_cfgData.m_DatabaseType );

			if( !pDBProcInfo->m_pDBConnection)
				return false;

			pDBProcInfo->m_pDBConnection->connect(
				g_cfgData.m_vctConnection[i].m_DB_User,	
				g_cfgData.m_vctConnection[i].m_DB_Password,
				g_cfgData.m_vctConnection[i].m_DB_Database,
				g_cfgData.m_vctConnection[i].m_DB_Server,
				g_cfgData.m_vctConnection[i].m_ThreadCount,
				1, g_cfgData.m_vctConnection[i].m_charset,this->getServiceName() );

			pDBProcInfo->m_connected=true;
			Service::printConsole("%s: Connect to Server(%s) DB(%s) Number of Connection(%d) OK\n",
				g_cfgData.m_vctConnection[i].m_SourceName.c_str(),
				g_cfgData.m_vctConnection[i].m_DB_Server.c_str(),
				g_cfgData.m_vctConnection[i].m_DB_Database.c_str(), 
				g_cfgData.m_vctConnection[i].m_ThreadCount);
		
			g_dagwlog.print("%s: Connect to Server(%s) DB(%s) Number of Connection(%d) OK\n",
				g_cfgData.m_vctConnection[i].m_SourceName.c_str(),
				g_cfgData.m_vctConnection[i].m_DB_Server.c_str(),
				g_cfgData.m_vctConnection[i].m_DB_Database.c_str(), 
				g_cfgData.m_vctConnection[i].m_ThreadCount);
			TDBProcess *pTDBProcess;
			for( int j = 0; j < g_cfgData.m_vctConnection[i].m_ThreadCount;j++,index++)
			{
				BaseQuery* pQuery = pDBProcInfo->m_pDBConnection->requestQueryConnection();
				if(!pQuery)
					return false;

				pTDBProcess = new TDBProcess (this,pQuery,index+1);
				if( !pTDBProcess)
					return false;
				pTDBProcess->start();
				pDBProcInfo->m_vctProcess.push_back( pTDBProcess);
			}
			m_mapProcess[g_cfgData.m_vctConnection[i].m_SourceName]
				=	pDBProcInfo;
		}
	}
	catch( BaseException &err )
	{
		g_dagwlog.print("%s %s\n",err.name.c_str(),err.description.c_str());
		Service::printConsole(cacti::LogLevel::FATAL, "%s %s\n",err.name.c_str(),err.description.c_str());
	}
	Service::printConsole("DAGW Service init OK \n");
	return true;
}


void TDBGWService::uninit()
{
	g_dagwlog.print("DAGW Service uninit");
	BaseQuery* pQuery;
	map<string,CDBProcInfo*>::iterator  it = m_mapProcess.begin();
	for(  ; it !=m_mapProcess.end() ; ++it )
	{
		CDBProcInfo *pDBProcInfo = (*it).second;
		for(u32 i=0 ; i< pDBProcInfo->m_vctProcess.size() ;i ++)
		{
			pQuery=pDBProcInfo->m_vctProcess[i]->GetBaseQuery();
			pDBProcInfo->m_pDBConnection->releaseQueryConnection(pQuery);
			pDBProcInfo->m_vctProcess[i]->Stop();
			delete pDBProcInfo->m_vctProcess[i];
		}

		try 
		{
			pDBProcInfo->m_pDBConnection->disconnect(1);
			delete pDBProcInfo->m_pDBConnection;
			pDBProcInfo->m_pDBConnection = NULL;
		}
		catch( BaseException &err )
		{
			g_dagwlog.print("%s %s\n",err.name.c_str(),err.description.c_str());
			Service::printConsole(cacti::LogLevel::FATAL, "%s %s\n",err.name.c_str(),err.description.c_str());
		}
	}
	return;
}

void TDBGWService::HandleExecSQL(const ServiceIdentifier& sender, UserTransferMessage& utm)
{
	
	string sourcename = utm[_TagSourceName];

	map<string,CDBProcInfo*>::iterator it = m_mapProcess.find( sourcename);
	if( it == m_mapProcess.end() )
	{
		g_dagwlog.print("Can not find any process for source (%s) \n",
											sourcename.c_str());
		utm.setMessageId(_ResExecSQL);
		utm.setRes(utm.getReq());
		utm.setReturn(-1);
		postMessage(sender,utm);
		return;
	}
	//
	//	
	//
	string		sql			= utm[_TagSql];
	TDBProcess *pTDBProcess = 0;
	int			dbfidx		= dbfindex( sql.c_str() );
	if ( dbfidx == 0 )	{
		pTDBProcess	=	(*it).second->GetProcess();
	}
	else{
		char*		sql2	= strdup( sql.c_str() );
		char*		ps		= sql2;
		while( *ps == ' ' || *ps == '\t' ) ps++;
		if ( *ps >= '0' && *ps <= '9' ){
			while( *ps != ';' && *ps != '\0')	ps++;
			if ( *ps == ';' ){
				ps++;
				utm[ _TagSql ]	= ps;
			};
		};
		pTDBProcess	=	(*it).second->GetProcess( dbfidx );
		free( sql2 );
	}
	//
	//
	//
	if(! pTDBProcess)
	{
		utm.setMessageId(_ResExecSQL);
		utm.setRes(utm.getReq());
		utm.setReturn(-1);
		postMessage(sender,utm);
		g_dagwlog.print("Process count is 0 for sourcename (%s) \n",
					sourcename.c_str());
		return;
	}
	else{
		g_dagwlog.print("[%04d] select this process \n", pTDBProcess->GetIndex());
	};
	
	if (!(*it).second->m_connected)
	{
		g_dagwlog.print("this dbconnection is not connect,must reconnect!\n");
		utm.setMessageId(_ResExecSQL);
		utm.setRes(utm.getReq());
		utm.setReturn(-1);
		postMessage(sender,utm);
		reconnect(sourcename);
		return;
	}
	int ret = pTDBProcess->Process( utm );
	if( !ret)
	{
		utm.setMessageId(_ResExecSQL);
		utm.setRes(utm.getReq());
		utm.setReturn(-1);
		postMessage(sender,utm);
		g_dagwlog.print("Queue full for thread %d\n",pTDBProcess->GetIndex());
		return;
	}
	return ;
}

void TDBGWService::onDefaultMessage(const ServiceIdentifier &sender,UserTransferMessage& utm)
{
	switch(utm.getMessageId())
	{
	case _EvtModuleActive:

		if( AppPort::_apSlee == sender.m_appport)
			registerToSLEE(sender,utm);

		break;
	case _EvtModuleDeactive:
		break;
	default:
		g_dagwlog.print("RECV Message (0x%X) but no handler\n", utm.getMessageId());
		break;
	}

}

void TDBGWService::registerToSLEE(const cacti::ServiceIdentifier &sender, cacti::UserTransferMessage &utm)
{

	utm.setMessageId(_ReqRegisterService);
	utm.swapSid();

	for( int i= 0 ; i< g_cfgData.m_vctConnection.size() ; i++ )
	{
		utm[_TagGatewayName] = g_cfgData.m_vctConnection[i].m_SourceName;
		utm[_TagGatewayUUID] = g_cfgData.m_vctConnection[i].m_DB_Server;
		postMessage(sender,utm);
		g_dagwlog.print("[0000] Register Service  '%s' to %s\n",
			g_cfgData.m_vctConnection[i].m_SourceName.c_str(),  sender.toString().c_str());
	}
}

void TDBGWService::HandleRespRegister(const ServiceIdentifier& sender, UserTransferMessage& utm)
{
	string sourcename =  utm[_TagGatewayName];
	g_dagwlog.print("[0000] Register Service '%s' to %s  %s(%d)\n", sourcename.c_str(),sender.toString().c_str(),
					(utm.getReturn()== 0) ? "OK": "Fail", utm.getReturn());
}


void CDAGWCfgData::Load(const char* filename)
{
	IniFile dagwconf;
	dagwconf.open(filename);
	char key[128];
	u32 i = 0;

	CConnectionCfg connCfg;
	while(i++ < 100)
	{
		sprintf(key,"connection.%d.name",i);
		 connCfg.m_SourceName = dagwconf.readString("Database",key,"");
		if( "" == connCfg.m_SourceName )
			break;

		sprintf(key,"connection.%d.server",i);
		connCfg.m_DB_Server = dagwconf.readString("DataBase",key,"");

		sprintf(key,"connection.%d.user",i);
		connCfg.m_DB_User = dagwconf.readString("DataBase",key,"");

		sprintf(key,"connection.%d.password",i);
		connCfg.m_DB_Password = dagwconf.readString("DataBase",key,"");

		sprintf(key,"connection.%d.database",i);
		connCfg.m_DB_Database = dagwconf.readString("DataBase",key,"");
		
		sprintf(key,"connection.%d.charset",i);
		connCfg.m_charset = dagwconf.readString("DataBase",key,"iso_1");

		sprintf(key,"connection.%d.threadcount",i);
		connCfg.m_ThreadCount = dagwconf.readInt("DataBase",key,1);
		if(connCfg.m_ThreadCount <= 0)
			connCfg.m_ThreadCount = 1;

		m_vctConnection.push_back(connCfg);		
		m_cfglog.print("connect.%d.	name	= %s\n"
						"			server	= %s\n"
						"			user	= %s\n"
						"			password= %s\n"
						"			database= %s\n"
						"			thcount	= %d\n"
						"			charset = %s\n",
						i, connCfg.m_SourceName.c_str(),
						connCfg.m_DB_Server.c_str(), connCfg.m_DB_User.c_str(),
						connCfg.m_DB_Password.c_str(), connCfg.m_DB_Database.c_str(),
						connCfg.m_ThreadCount,connCfg.m_charset.c_str() );
	}

	m_cfglog.print("Loading config from %s \n", filename );
#ifdef  _DB_ORACLE_ 
	m_DatabaseType =(DbConnection::Driver)dagwconf.readInt(
		"Database","DatabaseType",DbConnection::ORACLE10);
#endif

#ifdef  _DB_SYBASE_ 
	m_DatabaseType =(DbConnection::Driver)dagwconf.readInt(
				"Database","DatabaseType",DbConnection::SYBASE);
#endif

#ifdef  FOR_ODBC 
	m_DatabaseType =(DbConnection::Driver)dagwconf.readInt(
				"Database","DatabaseType",DbConnection::ODBC);
#endif

	m_MaxRowCount = dagwconf.readInt("Database","MaxRowCount",200);
	if(m_MaxRowCount <= 0)
		m_MaxRowCount = 200;
}
void TDBGWService::reconnect(string sourceName)
{
	g_dagwlog.print("[%s]Reconnect DBProcess\n",sourceName.c_str());
	try
	{ 
		for ( int i = 0; i < g_cfgData.m_vctConnection.size() ; i++)
		{	
			if(sourceName==g_cfgData.m_vctConnection[i].m_SourceName)
			{
				map<string,CDBProcInfo*>::iterator it =this->m_mapProcess.find(sourceName);
				if (it!=this->m_mapProcess.end())
				{
					CDBProcInfo *pDBProcInfo = (*it).second; 
					pDBProcInfo->m_countMsg++; 
					g_dagwlog.print("[%s]m_countMsg=%d\n",sourceName.c_str(),pDBProcInfo->m_countMsg); 

					if(0!=pDBProcInfo->m_countMsg%DAGW_MAX_COUNT_MSG)
					{
						g_dagwlog.print("[%d] Reconnect(%s) return\n", pDBProcInfo->m_countMsg,sourceName.c_str()); 
						pDBProcInfo->m_connected=false;
						return;
					}
					else
					{
						g_dagwlog.print("[%s] Release query connection.\n", sourceName.c_str());
						pDBProcInfo->m_connected=false;
						BaseQuery* baseQuery=NULL;
						std::vector<TDBProcess *>::iterator iter ;
						for (iter=pDBProcInfo->m_vctProcess.begin();iter!=pDBProcInfo->m_vctProcess.end();iter++)
						{
							baseQuery=(*iter)->GetBaseQuery();
							if (baseQuery)
							{
								pDBProcInfo->m_pDBConnection->releaseQueryConnection(baseQuery);
								(*iter)->SetBaseQuery(NULL);
							}
						}

						try 
						{
							pDBProcInfo->m_pDBConnection->disconnect(5);
							g_dagwlog.print("[%s] The connection is disconnected successfully! \n", sourceName.c_str());
						}
						catch( BaseException &err )
						{
							g_dagwlog.print("[%s]error code is %d,%s %s\n",sourceName.c_str(),err.code,err.name.c_str(),err.description.c_str());
						}
					}
					g_dagwlog.print("[%s] Reconnect database with source:%s,name:%s,user:%s,password:%s\n",
						sourceName.c_str(),g_cfgData.m_vctConnection[i].m_DB_Server.c_str(), g_cfgData.m_vctConnection[i].m_DB_Database.c_str(), g_cfgData.m_vctConnection[i].m_DB_User.c_str(), g_cfgData.m_vctConnection[i].m_DB_Password.c_str());

					try
					{
						pDBProcInfo->m_pDBConnection->connect(
							g_cfgData.m_vctConnection[i].m_DB_User,	
							g_cfgData.m_vctConnection[i].m_DB_Password,
							g_cfgData.m_vctConnection[i].m_DB_Database,
							g_cfgData.m_vctConnection[i].m_DB_Server,
							g_cfgData.m_vctConnection[i].m_ThreadCount,
							1, g_cfgData.m_vctConnection[i].m_charset,this->getServiceName() );
					}
					catch (BaseException& errConnect)
					{
						g_dagwlog.print("[%s]error code is %d,%s %s\n",sourceName.c_str(),errConnect.code,errConnect.name.c_str(),errConnect.description.c_str());
						return;
					}

					Service::printConsole("[%s] Reconnect database with source:%s,name:%s,user:%s\n",sourceName.c_str(), g_cfgData.m_vctConnection[i].m_DB_Server.c_str(), g_cfgData.m_vctConnection[i].m_DB_Database.c_str(), g_cfgData.m_vctConnection[i].m_DB_User.c_str());

					std::vector<TDBProcess *>::iterator iter;
					for (iter=pDBProcInfo->m_vctProcess.begin();iter!=pDBProcInfo->m_vctProcess.end();iter++)
					{
						BaseQuery* pQuery = pDBProcInfo->m_pDBConnection->requestQueryConnection();
						(*iter)->SetBaseQuery(pQuery);
					}

					pDBProcInfo->m_connected=true;
					g_dagwlog.print("[%s] Reconnect successfully! \n", sourceName.c_str()); 
				}
			}
		}	
	}
	catch (BaseException& err)
	{ 
		g_dagwlog.print("[%s] Reconnect failed,code=%d\n", sourceName.c_str(),err.code);
		Service::printConsole("[%s] Open database failed\n", sourceName.c_str());
		int error_code = err.code; 
	}
}