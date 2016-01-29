#include "stdafx.h"
#include "convertprocess.h"
#include "cacti/util/IniFile.h"
#include "main/convertService.h"


std::vector<std::string> spiltChar(std::string str,std::string pattern);

ConverProcess::ConverProcess()
:m_logger(cacti::Logger::getInstance("billupload"))
{
}

ConverProcess::~ConverProcess()
{
	uninit();

}

bool ConverProcess::init()
{
	time_t now;
	time_t settime;
	time_t sec;
	struct tm * settimeinfo;
	m_logger.info("Starting the db process...\n");
	
	if (!m_dbProcess.init())
	{
		m_logger.info("Start the dbprocess failed.\n");
		return false;
	}

	m_logger.info("Starting the  conversion (convertprocess)...\n");

	//get the table  columns_name and the clomuns_type  and make sql
	m_dbProcess.makesql();

	return true;
}

void ConverProcess::uninit()
{
	m_logger.info("Uninit ConverProcess Process.\n");

	m_dbProcess.uninit();

}
void ConverProcess::stop()
{
	uninit();
}


bool ConverProcess::run()
{
	if(!init())
	{
		m_logger.info("Init the ConverProcess class failed.\n");
		return  false;
	}

	Service::printConsole("conversion Process running OK\n");
	if(!m_dbProcess.conversion_start())
	{
		Service::printConsole("conversion Process quit  failed \n");
		return false;
	}
	
	Service::printConsole("conversion Process quit  sucessed\n");
	return true;
	
}

std::vector<std::string> spiltChar(std::string str,std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str+=pattern;//扩展字符串以方便操作
	int size=str.size();
	
	for(int i=0; i<size; i++)
	{
		pos=str.find(pattern,i);
		if(pos<size)
		 {
			std::string s=str.substr(i,pos-i);
			result.push_back(s);
			i=pos+pattern.size()-1;
		}
	 }
	 return result;
}