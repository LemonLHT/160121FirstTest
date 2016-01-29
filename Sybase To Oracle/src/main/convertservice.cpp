#include "stdafx.h"
#include "convertService.h"
#include "convertProcess.h"
#include "cacti/logging/BALog.h"

boost::shared_ptr<Service> createService()
{
	return boost::shared_ptr<Service> (new ConverService());
}

bool ConverService::start()
{
	createLogger("sybase conversion to oracle");

	Logger::getInstance("sybase_oracle_conversion").fatal("Starting convering...\n");

	m_ExportProcess = new ConverProcess();
	if(!m_ExportProcess)
	{
		Service::printConsole("Distribution m_ExportProcess pointer failed.\n");
	}

	if(!m_ExportProcess->run())
	{
		return false;
	}

	Service::printConsole("Start database conversion Success.\n");
	return true;
}

void ConverService::stop()
{
	if(m_ExportProcess)
	{
		m_ExportProcess->stop();
		delete m_ExportProcess;
	}
}

void ConverService::snapshot()
{

}

void ConverService::createLogger(const char* name)
{
	char path[100];
	sprintf_s(path, "./log/%s.log", name);
	LogHandlerPtr tasHandler = LogHandlerPtr(new StarFileHandler(path));
	StarFormatter* ttic2 = new StarFormatter;
	ttic2->logIndex(false);
	ttic2->autoNewLine(false);
	FormatterPtr ttic2Ptr(ttic2);
	tasHandler->setFormatter(ttic2Ptr);
	Logger::getInstance(name).addHandler(tasHandler);
	Logger::getInstance(name).setLevel(LogLevel::DBG);
}
