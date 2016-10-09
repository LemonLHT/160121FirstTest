
#include "stdafx.h"
#include "dagw.h"
#include "dagwservice.h"


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
