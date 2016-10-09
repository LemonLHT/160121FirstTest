
#include "stdafx.h"
#include "dagw.h"
#include "dagwservice.h"
#include "reqmsg.h"

boost::shared_ptr<Service> createService()
{
	return boost::shared_ptr<Service> (new DAGW());
}
	
bool DAGW::start()
{
	m_pMessageDispatcher = new cacti::MessageDispatcher();

	if( !m_pMessageDispatcher )
	{
		return false;
	}
	if( !m_pMessageDispatcher->start(DAGW_CFG_FILE))
	{
		Service::printConsole("MTL running...Failed\n");
		return false;
	}
		
	Service::printConsole("MTL running....OK\n");
	m_pTDBGWService = new TDBGWService(m_pMessageDispatcher);
	if( !m_pTDBGWService )
	{
		return false;
	}
	if(!m_pTDBGWService->start())
	{
		delete m_pTDBGWService;
		m_pTDBGWService = NULL;

		m_pMessageDispatcher->stop();
		delete m_pMessageDispatcher;
		m_pMessageDispatcher = NULL;

		return false;
	}

	return true;
}

void DAGW::stop()
{
	Service::printConsole("system stop...\n");

	if( m_pTDBGWService )
	{
		m_pTDBGWService->stop();
		delete m_pTDBGWService;
	}

	if( m_pMessageDispatcher)
	{
		m_pMessageDispatcher->stop();
		delete m_pMessageDispatcher;
	}

}

void DAGW::snapshot()
{

}

void DAGW::handleUICommand(std::vector<std::string> & vec)
{
	if (vec[0] == "1")
	{
		UserTransferMessagePtr utm(new UserTransferMessage);

		// 设置请求需要的相关参数
		utm->setMessageId(_ReqStartFlowEx);
		utm->setReq(ServiceIdentifier(3, AppPort::_apSlee, 0));
		utm->setRes(ServiceIdentifier(4, AppPort::_apDagw, 0));
		utm->setReturn(0);
		//char sql[1024]= {0};

		//sprintf_s(sql, "Begin P_SP_QUERY ('01066666666','1169918711','1169918711',:P1); end;");

		//(*utm)[_TagSql] = sql;
		char* buf[]= {"1000","1","15200083761","1169918711","1169918711"};
		strarray str(buf,buf+5);
		(*utm)[_TagParameter] = str;
		(*utm)[_TagSourceName] = "xy";
		m_pTDBGWService->postSelfMessage(utm);
	}
}