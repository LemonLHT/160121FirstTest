#include "stdafx.h"
#include "groupbillservice.h"
#include "cacti/logging/BALog.h"
#include "TLuaExec.h"
#include "monthbillprocess.h"
#include "detailbillprocess.h"

boost::shared_ptr<Service> createService()
{
	return boost::shared_ptr<Service> (new CDMAService());
}

bool CDMAService::start()
{
	createLogger("CExport");

	Logger::getInstance("CExport").fatal("Starting CExport...\n");

	// 加载lua
	char luaFilePath[256];
	GetPrivateProfileString("system", "luafilepath", "./lua/gz-pstn-export.lua", luaFilePath, sizeof(luaFilePath), CFG_CONF);
	if (0 != TLuaExec::LoadAndExecLua(luaFilePath))
	{
		Logger::getInstance("CExport").info("Load lua failed, Start CExport Failed.\n");
		return false;
	}

	// 加载配置项
	loadConfig();

	if (!startCDMAProc())
	{
		Logger::getInstance("CExport").fatal("Starting CExport  FAILED.\n");
		return false;
	}

	m_scanThread.start(this,&CDMAService::startScan);

	Logger::getInstance("CExport").info("Start CExport Success.\n");
	return true;
}

void CDMAService::loadConfig()
{
	m_billType = (TBillType)GetPrivateProfileInt("system", "BillType", eBillType_Month, CFG_CONF);
	Logger::getInstance("CExport").info("处理话单类型: %s\n", m_billType==eBillType_Detail ? "准实时详单" : "月汇总话单");
}

void CDMAService::stop()
{
	for (map<unsigned int,BillProcess*>::iterator  it =m_mapCDMAProc.begin();it!=m_mapCDMAProc.end();it++)
	{
		BillProcess* pCdma=(*it).second;

		pCdma->stop();
		if (pCdma !=NULL)
		{
			delete pCdma;
		}

	}

	if (!m_scanExit)
	{
		m_scanExit=true;
		m_scanThread.join();
	}
}

void CDMAService::snapshot()
{
	FILE * fp = NULL;
	fp = fopen("./log/snapshot.log","w+");
	if (fp == NULL)
	{
		return ;
	}
	fprintf(fp,"SyncGWProcess{\n");

	for (map<unsigned int,BillProcess*>::iterator  it =m_mapCDMAProc.begin();it!=m_mapCDMAProc.end();++it)
	{
		BillProcess* cdr=(*it).second;
		fprintf(fp,"thread id=%d.\n",cdr->currentId());
	}
	fprintf(fp,"}\n");
	fclose(fp);

}

void CDMAService::createLogger(const char* name)
{
	char path[100]={0};
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

bool CDMAService::startCDMAProc()
{
	char buffer[512]={0};
	try
	{		
		IniFile cfgFile;
		if(!cfgFile.open(CFG_CONF))
			return false;

		if (eBillType_Detail == m_billType)
		{
			int i=1;
			for (;i<512;i++)
			{
				sprintf_s(buffer,"CityCode.%d",i);
				int code = cfgFile.readInt("CityConf",buffer,0);
				if (code == 0)
					break;

			// 创建c网处理线程
			BillProcess* pCDMAProcess = new DetailBillProcess(code, "CDMA", this);
			if(pCDMAProcess == NULL)
			{
				Logger::getInstance("CExport").fatal("Distribution CDMAProcess pointer failed.\n");
				return false;
			}
			m_mapCDMAProc.insert(pair<unsigned int,BillProcess*>(i*2-1,pCDMAProcess));
			pCDMAProcess->start();

			// 创建固网处理线程
			BillProcess* pPSTNProcess = new DetailBillProcess(code, "PSTN", this);
			if(pPSTNProcess == NULL)
			{
				Logger::getInstance("CExport").fatal("Distribution PSTNProcess pointer failed.\n");
				return false;
			}
			m_mapCDMAProc.insert(pair<unsigned int,BillProcess*>(i*2,pPSTNProcess));
			pPSTNProcess->start();
			}
		}
		else
		{
			// 处理固网月话单
			BillProcess* pMonthProcess = new MonthBillProcess(0, "PSTN", this);
			if(pMonthProcess == NULL)
			{
				Logger::getInstance("CExport").fatal("Distribution MonthBillProcess pointer failed.\n");
				return false;
			}
			m_mapCDMAProc.insert(pair<unsigned int,BillProcess*>(0,pMonthProcess));
			pMonthProcess->start();
		}
	}
	catch( ... )
	{
		return false;
	}
	return true;
}

void CDMAService::post2CDMAProc()
{
	Logger::getInstance("CExport").info("-------- begin to process ftp\n");
	RecursiveMutex::ScopedLock lock(m_lock);
	lock.lock();
	UserTransferMessage utm;
	utm.setMessageId(EvtDealCDR);
	try
	{
		for (map<unsigned int,BillProcess*>::iterator it=m_mapCDMAProc.begin();it!=m_mapCDMAProc.end();++it)
		{
			BillProcess* pCdrData=(*it).second;
			pCdrData->Process(utm);
		}
	}
	catch(...)
	{
		Logger::getInstance("CExport").error("error code = %d\r\n",GetLastError());
	}
	lock.unlock();
}

void CDMAService::startScan()
{
	IniFile cfgFile;
	if(!cfgFile.open(CFG_CONF))
		return ;
	m_scanInterval = cfgFile.readDouble("System", "ScanInterval", 1);
	cfgFile.clear();

	if (eBillType_Month == m_billType)	// 月汇总话单只需要启动时执行一次
	{
		//通知ftp开始下载处理联通局方话单
		post2CDMAProc();

		// 等待结束
		while(!m_scanExit)
		{
			Timestamp starttime(1);
			m_scanEvent.wait(starttime);
			printf("处理完成\n");
			Logger::getInstance("CExport").info("处理完成\n");
			break;
		}
	}
	else	// 详单需要周期执行
	{
		while(!m_scanExit)
		{
			Timestamp starttime(m_scanInterval*60);
			m_scanEvent.wait(starttime);
			//通知ftp开始下载处理联通局方话单
			post2CDMAProc();
			while(!BillProcess::isComplete())
			{
				m_scanEvent.wait(1); // 等待1秒，轮询是否完成
			}
			BillProcess::resetAction();		
		}
	}
}

