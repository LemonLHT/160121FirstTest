#ifndef _CONVERT_SERVICE_H_
#define _CONVERT_SERVICE_H_

#include "startup/Service.h"
#include "cacti/mtl/MessageDispatcher.h"

#define CFG_CONF "./conversion_info.conf"
using namespace cacti;

//Service startup class
class ConverProcess;
class ConverService : public Service
{
public:
	ConverService()
	:Service("Sybase-Oracle_conversion", "V1.2.1_20160129","database_conversion(sybase_oracle)")
	,m_ExportProcess(NULL)
	{};
	virtual bool start(); //Start the service.
	virtual void stop();  //Stop the service.
	virtual void snapshot(); 
private:
	void createLogger(const char* name); //Create the log.
private:
	ConverProcess* m_ExportProcess; //the process of ConverProcess class object.
};

#endif
