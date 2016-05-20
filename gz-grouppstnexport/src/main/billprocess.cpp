#include "stdafx.h"
#include "billprocess.h"
#include "cacti/util/IniFile.h"
#include <fstream>
//#include <boost/regex.hpp>
#include "TLuaExec.h"
#include "cacti/util/FileSystem.h"

int BillProcess::m_doaction = 0;
int BillProcess::m_threadcount = 0;
RecursiveMutex			BillProcess::m_lock;

using namespace std;
using namespace boost;

BillProcess::BillProcess(unsigned int threadno, const string& nettype, CDMAService* owner)
:m_areaCode(threadno)
,m_logger(cacti::Logger::getInstance("CExport"))
,m_syncTimer(NULL)
,m_queue(EXPORT_PROCESS_QUEUE_LEN)
,m_bStop(false)
,m_netTypeName(nettype)
,m_pOwner(owner) 
,m_immediateExtName(".tmp")
{
	++m_threadcount;
}

BillProcess::~BillProcess()
{

}

void BillProcess::resetAction()
{
	RecursiveMutex::ScopedLock lock(m_lock);
	m_doaction = 0;
	cout<<"resetAction():"<<m_doaction<<endl;
}

void BillProcess::addAction()
{
	RecursiveMutex::ScopedLock lock(m_lock);
	m_doaction++;
	cout<<"addAction():"<<m_doaction<<endl;
}

bool BillProcess::isComplete()
{
	RecursiveMutex::ScopedLock lock(m_lock);
	cout<<"isComplete():"<<m_doaction<<endl;
	if (m_doaction == m_threadcount)
	{
		return true;
	}
	else
		return false;
}

bool BillProcess::init()
{
	m_logger.info("[%04d]Starting the Process timer...\n",m_areaCode);

	// 加载配置项，其中包括创建目录
	if (false == LoadConfig())
	{
		return false;
	}

	m_syncTimer=new EasyTimer(this);

	if(!m_syncTimer)
	{
		m_logger.info("[%04d]Distribution m_exportTimer pointer failed.\n",m_areaCode);
		return false;
	}

	if(!m_syncTimer->start())
	{
		m_logger.info("[%04d]Started the sync timer failed.\n",m_areaCode);
		return false;
	}

	return true;
}

void BillProcess::uninit()
{
	m_logger.info("Uninit CDMAProcess Process.\n");

	if(m_syncTimer)
	{
		m_syncTimer->stop();

		delete m_syncTimer;
	}	

	m_logger.info("CDMAProcess Process uninited.\n");
}

void BillProcess::onMessage(const ServiceIdentifier& sender, UserTransferMessage& utm)
{
	switch (utm.getMessageId())
	{
	case EvtDealCDR: //只处理C网话单
		{
			onCDMABill();
		}
		break;
	default:
		onDefaultMessage(sender,utm);
		break;
	}
}

void BillProcess::stop()
{
	if( !m_bStop)
	{
		uninit();
		m_bStop = true;
		UserTransferMessage utm;
		m_queue.put(utm);
		join();
	}
}

void BillProcess::run()
{
	if(!init())
	{
		m_logger.info("Init the ExportProcess class failed.\n");
		return ;
	}
	UserTransferMessage utm;
	while( !m_bStop )
	{
		m_queue.get(utm);
		if( m_queue.size() > 1024)
			m_logger.info("[%04d]QueueLen is %d ",m_areaCode,m_queue.size());
		onMessage( utm.getReq(), utm);
	}
}

void  BillProcess::onDefaultMessage(const ServiceIdentifier& sender, UserTransferMessage& utm)
{
	m_logger.info("[%04d]RECV Message (0x%08X) from (%s) but no handler.\n",
		m_areaCode,
		utm.getMessageId(),
		utm.getReq().toString().c_str());
}

bool BillProcess::Process(UserTransferMessage &utm)
{
	return m_queue.tryPut(utm);
}

TimerID BillProcess::setTimer(u32 expires,UserTransferMessage utm)
{
	return m_syncTimer->set(expires,utm);
}

void BillProcess::killTimer(TimerID id)
{
	m_syncTimer->cancel(id);
}

ServiceIdentifier BillProcess::myIdentifier(u32 ref /*= 0*/)
{
	ServiceIdentifier tmp(0,0,m_areaCode);
	return tmp;
}

void BillProcess::response(UserTransferMessage utm)
{
	m_queue.tryPut(utm);
	return ;
}

void BillProcess::onCDMABill()
{
	// 测试调用lua
// 	int ret;
// 	TLuaExec::CallLuaFunction(0, "HandleOneFileOutput", "ss>i", "a", "1\n2\n", &ret);
// 	TLuaExec::CallLuaFunction(0, "HandleOneFileOutput", "ss>i", "", "a\nb\n", &ret);

	m_logger.info("[%s] begin to handle.\n", GetLogDesc().c_str());

	char buffer[512]={0};
	list<string> filelist;	// ftp服务端文件名列表，不包含路径

	// 获取需要处理的文件名列表
	GetNeedHandleFileList(filelist);

	if (filelist.empty())
	{	
		m_logger.info("[%s] end. no file\n", GetLogDesc().c_str());
		addAction();
		return;
	}

	// 输出待处理文件列表
	for (list<string>::iterator it = filelist.begin();it!=filelist.end();++it)
	{
		m_logger.info("[%s] need handle file: file=%s\n",GetLogDesc().c_str(),(*it).c_str());
	}

	string content;//保存转化话单内容

	// 处理所有源文件
	HandleAllBillFiles(filelist);

	// 清除源文件
	if (m_bNeedHandleLineByLine)	// 仅当此情况下需要清除源文件
	{
		char szFullSourceFileName[256];
		for (list<string>::iterator it=filelist.begin(); it!=filelist.end(); ++it)
		{
			sprintf_s(szFullSourceFileName, "%s\\%s\\%s", m_storedPath.c_str(), GetSubDirOfStoredPath().c_str(), (*it).c_str());
			remove(szFullSourceFileName);
			m_logger.info("remove original: %s\n", szFullSourceFileName);
		}
	}

	addAction();

	m_logger.info("[%s] end\n",GetLogDesc().c_str());
}

/************************************************************************/
/* 
功能: 获取月汇总话单的文件名 注: 文件名中的时间应为上月的1号至最后一天
参数:
返回:
*/
/************************************************************************/
string BillProcess::GetLastMonthBillFileName()
{
	time_t t1 = time(NULL);
	printf("t1: %d\n", t1);
	struct tm* tm1 = localtime(&t1);

	int year = tm1->tm_year + 1900;
	int month = tm1->tm_mon + 1;
	int maxDay = 31;
	// 月份减1
	if (month == 1)
	{
		month = 12;
		year--;
	}
	else
	{
		month--;
	}

	if (month==4 || month==6 || month==9 || month==11) // 小月
	{
		maxDay = 30;
	}
	else if (month == 2)
	{
		if ((year%4==0 && year%100!=0) || year%400==0)	// 润年
		{
			maxDay = 29;
		}
		else
		{
			maxDay = 28;
		}
	}

	char szName[100];
	sprintf_s(szName, 100, "%d%s(%4d%02d01-%4d%02d%02d).rar", m_areaCode, GetNetType().c_str(), year, month, year, month, maxDay);

	return szName;
}

/************************************************************************/
/* 
功能: 获取话单文件所在的ftp目录  不包括ip端口等，如 /2014/201409/MONTHBILLING/
说明: 处理详单和月汇总话单时，该目录值不相同  详单为: /2014/201409/BILLING/851/
参数:
返回:
*/
/************************************************************************/
string BillProcess::GetFtpParentDir()
{
	time_t t1 = time(NULL);
	printf("t1: %d\n", t1);
	struct tm* tm1 = localtime(&t1);

	int year = tm1->tm_year + 1900;
	int month = tm1->tm_mon + 1;

	char szDir[400];

	if (eBillType_Detail ==	m_pOwner->GetBillType()) // 详单
	{
		sprintf_s(szDir, 400, "/%04d/%04d%02d/billing/%s/%d", year, year, month, GetNetType().c_str(), m_areaCode);
	}
	else // 月汇总
	{
		// 月份减1
		if (month == 1)
		{
			month = 12;
			year--;
		}
		else
		{
			month--;
		}

		sprintf_s(szDir, 400, "/%04d/%04d%02d/monthbilling/", year, year, month);
	}

	return szDir;
}

/************************************************************************/
/* 
功能: 下载完文件后的处理，将话单文件上传到省IT指定FTP地址，并备份到指定目录
参数:
返回:
*/
/************************************************************************/
void BillProcess::HandleAllBillFiles(list<string>& filelist)
{
	char szFullSourceFileName[256];	// 源文件全路径
	string outputFullPath;			// 处理后的文件路径(不论源文件是否需要逐行处理，都有输出文件的概念)
	string outputFileName;			// 处理后的输出文件名
	map<string, string> m_outputFileList;	// 输出文件列表 key:文件名 value:全路径

	for (list<string>::iterator it=filelist.begin(); it!=filelist.end(); ++it)
	{
		sprintf_s(szFullSourceFileName, "%s\\%s\\%s", m_storedPath.c_str(), GetSubDirOfStoredPath().c_str(), (*it).c_str());	// 月汇总话单

		// 判断是否需要对源文件进行逐行处理
		if (m_bNeedHandleLineByLine) // 需要对源文件进行逐行处理
		{
			m_logger.info("[%s] begin to handle file: %s\n", GetLogDesc().c_str(), szFullSourceFileName);
			if (0 != HandleOneFileLineByLine(szFullSourceFileName, outputFullPath, outputFileName))
			{
				m_logger.info("[%s] [ERROR] handle one file failed\n", GetLogDesc().c_str());
				continue;
			}

			m_outputFileList[outputFileName] = outputFullPath;
		}
		else // 不需要对源文件逐行处理，一般是将整个文件进行拷贝或上传
		{
			m_immediateExtName = "";
			m_outputFileList[*it] = szFullSourceFileName;
		}
	}

	// 处理所有输出文件(目前已有的两种处理: 上传ftp 存放到指定目录) 注意：map中存放的文件名不带.tmp扩展，但真实文件名是以.tmp扩展，以防止在月话单情况下，未处理完所有话单，对方即已经过来采集，导致数据不全
	int destType = GetPrivateProfileInt(GetNetType().c_str(), "OutputDestinationType", eOutputDest_Local, CFG_CONF); 
	map<string, string>::iterator it2 = m_outputFileList.begin();
	for (; it2!=m_outputFileList.end(); ++it2)
	{
		string curOutputFullPath = it2->second + m_immediateExtName;
		m_logger.info("need ftp or local store: %s\n", curOutputFullPath.c_str());

		// 过滤空文件
		if (FileSystem::fileSize(curOutputFullPath.c_str()) <= 0)
		{
			m_logger.info("remove empty file: %s\n", curOutputFullPath.c_str());
			remove(curOutputFullPath.c_str());
			continue;
		}

		if ((destType & eOutputDest_Local) == eOutputDest_Local)	// 拷贝到本地目录
		{
			string dstFullPath = m_outputPath + "\\" + it2->first;

			// 注意，此处是直接把中间文件拷贝到目标路径的目标文件，如果存在极端情况下，拷贝过程中有请求方来采集数据导致异常，那么可先拷贝为中间文件，最后再改名
			FileSystem::copyfile(curOutputFullPath.c_str(), dstFullPath.c_str());

			m_logger.info("[%s] copy output file to local: %s\n", GetLogDesc().c_str(), dstFullPath.c_str());
		}

		if ((destType & eOutputDest_Ftp) == eOutputDest_Ftp)		// 上传ftp
		{
			string dstFullPath = m_upFtpUrlPath + "/" + it2->first;
			int ret = uploadFile(dstFullPath.c_str(), m_upFtpUserPwd.c_str(), curOutputFullPath.c_str());
			if (0 != ret)
			{
				m_logger.info("[%s] [ERROR] upload to ftp failed! curl ret = %d, file: %s\n",GetLogDesc().c_str(), ret, curOutputFullPath.c_str());
				continue;
			}

			m_logger.info("[%s] upload output file to ftp: %s\n", GetLogDesc().c_str(), dstFullPath.c_str());
		}

		// 备份
		string bakFile = m_bakPath + "\\" + it2->first;
		if (false == FileSystem::copyfile(curOutputFullPath.c_str(), bakFile.c_str()))
			m_logger.info("[ERROR] copy file failed when backup!\n");
		else
			m_logger.info("backup to: %s\n", bakFile.c_str());

		// 删除中间文件
		remove(curOutputFullPath.c_str());
		m_logger.info("remove: %s\n", curOutputFullPath.c_str());
	}

	
}

/************************************************************************/
/* 
功能: 加载配置项
参数:
返回:
*/
/************************************************************************/
bool BillProcess::LoadConfig()
{
	// 创建上传目录和备份目录
	IniFile conf;
	if(!conf.open(CFG_CONF))
	{
		m_logger.info("[ERROR] open conf file failed!\n");
		return false;
	}
	
	m_bakPath = conf.readString(GetNetType().c_str(),"BakUpPath","");
	if (m_bakPath.empty())
	{
		m_logger.info("[ERROR] up or bak path error!\n");
		return false;
	}

	// 存储路径 包括下载后存储路径，或者处理的源文件所在路径
	m_storedPath = conf.readString(GetNetType().c_str(),"StoreFilePath","");
	if (m_storedPath.empty())
	{
		m_logger.info("[ERROR] stored path error!\n");
		return false;
	}
	else
	{
		FileSystem::createMulDir(m_storedPath);
	}

	m_outputPath = conf.readString(GetNetType().c_str(),"OutputPath","");
	if (m_outputPath.empty())
	{
		m_logger.info("[INFO] output path empty!\n");
		//return false;	//  不能返回失败，因为某些情况下是不需要输出路径的
	}
	else
	{
		FileSystem::createMulDir(m_outputPath);
	}

	FileSystem::createMulDir(m_bakPath);

	// 
	m_bNeedHandleLineByLine = conf.readInt(GetNetType().c_str(), "NeedHandleLineByLine", 1);

	// 获取上传FTP信息
	string sUpFtpServer = conf.readString("ITFtpConf","FtpServer","");
	string sUpFtpPort = conf.readString("ITFtpConf","FtpPort","");
	string sUpFtpSubDir = conf.readString(GetNetType().c_str(),"SubDir","");
	m_upFtpUrlPath = "ftp://" + sUpFtpServer + ":" + sUpFtpPort + "/" + sUpFtpSubDir;
	m_logger.info("[INFO] up ftp path: %s\n", m_upFtpUrlPath.c_str());

	string sUpFtpUser = conf.readString("ITFtpConf","FtpUser","");
	string sUpFtpPassword = conf.readString("ITFtpConf","FtpPassword","");
	m_upFtpUserPwd = sUpFtpUser + ":" + sUpFtpPassword;
	m_logger.info("[INFO] up ftp usr: %s\n", m_upFtpUserPwd.c_str());
	
	return true;
}

/************************************************************************/
/* 
功能: 处理一个文件
参数:
返回:
*/
/************************************************************************/
int BillProcess::HandleOneFileLineByLine(const string& srcFullPath, string& dstFullPath, string& dstFileName)
{
	int ret = 0;

	// 遍历每行数据
	FILE *fp; 
	char line[1024];             //每行最大读取的字符数
	if((fp = fopen(srcFullPath.c_str(),"r")) == NULL) //判断文件是否存在及可读
	{ 
		m_logger.info("[ERROR] open source file failed: %s\n", srcFullPath.c_str()); 
		return -1; 
	} 	

	string outputContent = "";	// 一个源文件处理后的结果内容数据
	char szOutputLine[1024];	// 一行处理结果
	while (!feof(fp)) 
	{ 
		char *pLine = fgets(line,1024,fp);  // 读取一行数据(从实际情况来看，行尾均有换行，所以不需要考虑最后一行无换行问题)
		if (!pLine) // 最后一行空白行
		{
			break;
		}
		// 调用lua处理行数据(包括字段解析、主叫过滤等)得到一行目标数据	
		int retLine = 0;
		ret = TLuaExec::CallLuaFunction(0, "HandleLine", "s>is", line, &retLine, szOutputLine);
		if (0 != ret)
		{
			m_logger.info("[ERROR] call lua function HandleLine failed\n");
			fclose(fp);
			return -2;
		}
		if (0 != retLine)
		{
			m_logger.info("[ERROR] parse line failed: %s\n", line);
			continue;
		}

		if (0 == strcmp("", szOutputLine)) // 返回
		{
			//m_logger.info("caller be filtered\n");
			continue;
		}
		outputContent += szOutputLine;
	} 
	fclose(fp); 

	// 从lua中获取输出文件名
	char szOutputFileName[200] = "";
	char szExt[200];
	sprintf_s(szExt, 200, "%d", m_areaCode);
	TLuaExec::CallLuaFunction(0, "GetOutputFileName", "s>s", szExt, szOutputFileName);
	if (0 == strcmp("", szOutputFileName)) // 获取文件名失败
	{
		m_logger.info("get output filename failed\n");
		return 1;
	}

	dstFileName = szOutputFileName;

	// 获取输出文件全路径
	char szOutputFileFullPath[256];
	sprintf_s(szOutputFileFullPath, 256, "%s\\%s", m_outputPath.c_str(), szOutputFileName);
	dstFullPath = szOutputFileFullPath;

	// 输出文件加上扩展名
	strcat_s(szOutputFileFullPath, m_immediateExtName.c_str());
	m_logger.info("output file: %s\n", szOutputFileFullPath);

	// 输出到文件
	TLuaExec::CallLuaFunction(0, "HandleOneFileOutput", "ss>i", szOutputFileFullPath, outputContent.c_str(), &ret);
	if (0 != ret)
	{
		m_logger.info("output one file result to dest file failed\n");
		return 2;
	}	

	return 0;
}