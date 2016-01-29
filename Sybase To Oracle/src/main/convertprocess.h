#ifndef _EXPORT_PROCESS_H_
#define _EXPORT_PROCESS_H_

#include <string>
#include <vector>
#include "cacti/logging/Logger.h"
#include "cacti/kernel/Thread.h"
#include "cacti/util/BoundedQueue.h"
#include "cacti/message/TransferMessage.h"
#include "cacti/mtl/ServiceSkeleton.h"
#include "timer/easytimer.h"
#include "db/dbprocess.h"
#include "filemanager/filemanager.h"
#include "timer/timerprocess.h"

using namespace std;
using namespace cacti;

#define EvtOnTimer   0x40000001
#define EvtActInfoTimer  0x40000002

#define SetTime    0x50000001




const int EXPORT_PROCESS_QUEUE_LEN = 4096;


class TimerPro;
class EasyTimer;

//The billExport the process of class.

class ConverProcess
{
public:
	ConverProcess();
	~ConverProcess();
public:
	bool init(); //Initialization.
	void uninit();//Reverse initialization. 

	void stop();//Stop the thread.
	virtual bool run();

private:

	


private:
	Logger& m_logger; //Log Print object.

	
	DBProcess m_dbProcess; //the object of the database process class.

	
	
};
#endif