#ifndef _CDMA_SERVICE_H_
#define _CDMA_SERVICE_H_

#include <map>
#include "startup/Service.h"
#include "cacti/mtl/MessageDispatcher.h"
#include "cacti/message/TypeDef.h"
#include "cacti/util/IniFile.h"
//#include "billprocess.h"

#define CFG_CONF "./common-groupexport.conf"
using namespace cacti;
using namespace boost;
using namespace std;

#define EvtDealCDR   0x40000001

// 处理话单的类型 1:详单 2:月汇总话单
typedef enum
{
	eBillType_Detail = 1,
	eBillType_Month
} TBillType;

class BillProcess;
class CDMAService : public Service
{
public:
	CDMAService()
	:Service("CExport", "CTI_GroupBillExport_V1.0.0_20160229","电信集团话单出账-通用版本")
	,m_scanExit(FALSE)
	{};
	virtual bool start(); //Start the service.
	virtual void stop();  //Stop the service.
	virtual void snapshot(); 
private:
	void createLogger(const char* name); //Create the log.
	bool startCDMAProc();  //开始程控话单CDR处理线程
	void post2CDMAProc();	 //投递消息到程控话单CDR处理线程
	void startScan(); //开始扫描

	void loadConfig();

	TBillType GetBillType() { return m_billType; }

	friend class BillProcess;

private:
	map<unsigned int,BillProcess*> m_mapCDMAProc;
	RecursiveMutex			m_lock; //Thread lock.
	cacti::Thread m_scanThread; //扫描线程
	bool m_scanExit; //扫描线程工作标志 
	cacti::Event m_scanEvent; //扫描处理事件
	float m_scanInterval;//扫描间隔分钟

	TBillType m_billType;		// 处理的话单类型
};

#endif
