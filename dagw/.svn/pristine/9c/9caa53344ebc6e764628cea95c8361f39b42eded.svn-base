#ifndef __TAS_SERVICE_H
#define __TAS_SERVICE_H


#include <string>
#include <cacti/mtl/ServiceSkeleton.h>
#include "dbprocess.h"
#include "cacti/logging/LazyLog.h"

using namespace std;
using namespace cacti;

#define  DAGW_CFG_FILE "./dagw.conf"

struct CConnectionCfg
{
	int			m_ThreadCount;
	string		m_SourceName;
	string		m_DB_Server;
	string		m_DB_User;
	string		m_DB_Password;
	string		m_DB_Database;
	string		m_charset;

};

struct CDAGWCfgData
{
	u32			m_MaxRowCount;
	LazyLog		m_cfglog;
	DbConnection::Driver m_DatabaseType;
	vector<CConnectionCfg>	m_vctConnection;
	
	CDAGWCfgData() : m_cfglog("dagw_cfg") {};
	void		Load( const char*  filename );
};

struct CDBProcInfo
{
	u32	m_currIndex;
	bool m_connected;
	DbConnection*			m_pDBConnection;
	vector<TDBProcess *>	m_vctProcess;
	CDBProcInfo()
	{
		m_currIndex = 0;
		m_connected=false;
	}
	TDBProcess *GetProcess()
	{
		if( m_vctProcess.size() != 0 )
			return m_vctProcess[m_currIndex++%m_vctProcess.size()];
		else
			return NULL;
	}
	TDBProcess	*GetProcess( u32 idx ){
		if ( m_vctProcess.size() != 0 )
			return	m_vctProcess[ idx % m_vctProcess.size() ];
		else
			return	NULL;
	};
};


class TDBGWService : public ServiceSkeleton
{

public:
	TDBGWService( MessageDispatcher * dispatcher);
	~TDBGWService();

	void reconnect(string sourceName);
protected:
	virtual bool init();
	virtual void uninit();

private:
	virtual void onMessage(const ServiceIdentifier& sender, UserTransferMessage& utm);
	void	onDefaultMessage(const ServiceIdentifier& sender, UserTransferMessage& utm);
	void	registerToSLEE(const ServiceIdentifier& sender, UserTransferMessage& utm);
	void	HandleExecSQL(const ServiceIdentifier& sender, UserTransferMessage& utm);
	void	HandleRespRegister(const ServiceIdentifier& sender, UserTransferMessage& utm);
public:
	map<string,	CDBProcInfo *>	m_mapProcess;
};


#endif //__TAS_SERVICE_H