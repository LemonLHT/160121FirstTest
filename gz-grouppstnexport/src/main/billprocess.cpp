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

	// ������������а�������Ŀ¼
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
	case EvtDealCDR: //ֻ����C������
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
	// ���Ե���lua
// 	int ret;
// 	TLuaExec::CallLuaFunction(0, "HandleOneFileOutput", "ss>i", "a", "1\n2\n", &ret);
// 	TLuaExec::CallLuaFunction(0, "HandleOneFileOutput", "ss>i", "", "a\nb\n", &ret);

	m_logger.info("[%s] begin to handle.\n", GetLogDesc().c_str());

	char buffer[512]={0};
	list<string> filelist;	// ftp������ļ����б�������·��

	// ��ȡ��Ҫ������ļ����б�
	GetNeedHandleFileList(filelist);

	if (filelist.empty())
	{	
		m_logger.info("[%s] end. no file\n", GetLogDesc().c_str());
		addAction();
		return;
	}

	// ����������ļ��б�
	for (list<string>::iterator it = filelist.begin();it!=filelist.end();++it)
	{
		m_logger.info("[%s] need handle file: file=%s\n",GetLogDesc().c_str(),(*it).c_str());
	}

	string content;//����ת����������

	// ��������Դ�ļ�
	HandleAllBillFiles(filelist);

	// ���Դ�ļ�
	if (m_bNeedHandleLineByLine)	// �������������Ҫ���Դ�ļ�
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
����: ��ȡ�»��ܻ������ļ��� ע: �ļ����е�ʱ��ӦΪ���µ�1�������һ��
����:
����:
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
	// �·ݼ�1
	if (month == 1)
	{
		month = 12;
		year--;
	}
	else
	{
		month--;
	}

	if (month==4 || month==6 || month==9 || month==11) // С��
	{
		maxDay = 30;
	}
	else if (month == 2)
	{
		if ((year%4==0 && year%100!=0) || year%400==0)	// ����
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
����: ��ȡ�����ļ����ڵ�ftpĿ¼  ������ip�˿ڵȣ��� /2014/201409/MONTHBILLING/
˵��: �����굥���»��ܻ���ʱ����Ŀ¼ֵ����ͬ  �굥Ϊ: /2014/201409/BILLING/851/
����:
����:
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

	if (eBillType_Detail ==	m_pOwner->GetBillType()) // �굥
	{
		sprintf_s(szDir, 400, "/%04d/%04d%02d/billing/%s/%d", year, year, month, GetNetType().c_str(), m_areaCode);
	}
	else // �»���
	{
		// �·ݼ�1
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
����: �������ļ���Ĵ����������ļ��ϴ���ʡITָ��FTP��ַ�������ݵ�ָ��Ŀ¼
����:
����:
*/
/************************************************************************/
void BillProcess::HandleAllBillFiles(list<string>& filelist)
{
	char szFullSourceFileName[256];	// Դ�ļ�ȫ·��
	string outputFullPath;			// �������ļ�·��(����Դ�ļ��Ƿ���Ҫ���д�����������ļ��ĸ���)
	string outputFileName;			// ����������ļ���
	map<string, string> m_outputFileList;	// ����ļ��б� key:�ļ��� value:ȫ·��

	for (list<string>::iterator it=filelist.begin(); it!=filelist.end(); ++it)
	{
		sprintf_s(szFullSourceFileName, "%s\\%s\\%s", m_storedPath.c_str(), GetSubDirOfStoredPath().c_str(), (*it).c_str());	// �»��ܻ���

		// �ж��Ƿ���Ҫ��Դ�ļ��������д���
		if (m_bNeedHandleLineByLine) // ��Ҫ��Դ�ļ��������д���
		{
			m_logger.info("[%s] begin to handle file: %s\n", GetLogDesc().c_str(), szFullSourceFileName);
			if (0 != HandleOneFileLineByLine(szFullSourceFileName, outputFullPath, outputFileName))
			{
				m_logger.info("[%s] [ERROR] handle one file failed\n", GetLogDesc().c_str());
				continue;
			}

			m_outputFileList[outputFileName] = outputFullPath;
		}
		else // ����Ҫ��Դ�ļ����д���һ���ǽ������ļ����п������ϴ�
		{
			m_immediateExtName = "";
			m_outputFileList[*it] = szFullSourceFileName;
		}
	}

	// ������������ļ�(Ŀǰ���е����ִ���: �ϴ�ftp ��ŵ�ָ��Ŀ¼) ע�⣺map�д�ŵ��ļ�������.tmp��չ������ʵ�ļ�������.tmp��չ���Է�ֹ���»�������£�δ���������л������Է����Ѿ������ɼ����������ݲ�ȫ
	int destType = GetPrivateProfileInt(GetNetType().c_str(), "OutputDestinationType", eOutputDest_Local, CFG_CONF); 
	map<string, string>::iterator it2 = m_outputFileList.begin();
	for (; it2!=m_outputFileList.end(); ++it2)
	{
		string curOutputFullPath = it2->second + m_immediateExtName;
		m_logger.info("need ftp or local store: %s\n", curOutputFullPath.c_str());

		// ���˿��ļ�
		if (FileSystem::fileSize(curOutputFullPath.c_str()) <= 0)
		{
			m_logger.info("remove empty file: %s\n", curOutputFullPath.c_str());
			remove(curOutputFullPath.c_str());
			continue;
		}

		if ((destType & eOutputDest_Local) == eOutputDest_Local)	// ����������Ŀ¼
		{
			string dstFullPath = m_outputPath + "\\" + it2->first;

			// ע�⣬�˴���ֱ�Ӱ��м��ļ�������Ŀ��·����Ŀ���ļ���������ڼ�������£��������������������ɼ����ݵ����쳣����ô���ȿ���Ϊ�м��ļ�������ٸ���
			FileSystem::copyfile(curOutputFullPath.c_str(), dstFullPath.c_str());

			m_logger.info("[%s] copy output file to local: %s\n", GetLogDesc().c_str(), dstFullPath.c_str());
		}

		if ((destType & eOutputDest_Ftp) == eOutputDest_Ftp)		// �ϴ�ftp
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

		// ����
		string bakFile = m_bakPath + "\\" + it2->first;
		if (false == FileSystem::copyfile(curOutputFullPath.c_str(), bakFile.c_str()))
			m_logger.info("[ERROR] copy file failed when backup!\n");
		else
			m_logger.info("backup to: %s\n", bakFile.c_str());

		// ɾ���м��ļ�
		remove(curOutputFullPath.c_str());
		m_logger.info("remove: %s\n", curOutputFullPath.c_str());
	}

	
}

/************************************************************************/
/* 
����: ����������
����:
����:
*/
/************************************************************************/
bool BillProcess::LoadConfig()
{
	// �����ϴ�Ŀ¼�ͱ���Ŀ¼
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

	// �洢·�� �������غ�洢·�������ߴ����Դ�ļ�����·��
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
		//return false;	//  ���ܷ���ʧ�ܣ���ΪĳЩ������ǲ���Ҫ���·����
	}
	else
	{
		FileSystem::createMulDir(m_outputPath);
	}

	FileSystem::createMulDir(m_bakPath);

	// 
	m_bNeedHandleLineByLine = conf.readInt(GetNetType().c_str(), "NeedHandleLineByLine", 1);

	// ��ȡ�ϴ�FTP��Ϣ
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
����: ����һ���ļ�
����:
����:
*/
/************************************************************************/
int BillProcess::HandleOneFileLineByLine(const string& srcFullPath, string& dstFullPath, string& dstFileName)
{
	int ret = 0;

	// ����ÿ������
	FILE *fp; 
	char line[1024];             //ÿ������ȡ���ַ���
	if((fp = fopen(srcFullPath.c_str(),"r")) == NULL) //�ж��ļ��Ƿ���ڼ��ɶ�
	{ 
		m_logger.info("[ERROR] open source file failed: %s\n", srcFullPath.c_str()); 
		return -1; 
	} 	

	string outputContent = "";	// һ��Դ�ļ������Ľ����������
	char szOutputLine[1024];	// һ�д�����
	while (!feof(fp)) 
	{ 
		char *pLine = fgets(line,1024,fp);  // ��ȡһ������(��ʵ�������������β���л��У����Բ���Ҫ�������һ���޻�������)
		if (!pLine) // ���һ�пհ���
		{
			break;
		}
		// ����lua����������(�����ֶν��������й��˵�)�õ�һ��Ŀ������	
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

		if (0 == strcmp("", szOutputLine)) // ����
		{
			//m_logger.info("caller be filtered\n");
			continue;
		}
		outputContent += szOutputLine;
	} 
	fclose(fp); 

	// ��lua�л�ȡ����ļ���
	char szOutputFileName[200] = "";
	char szExt[200];
	sprintf_s(szExt, 200, "%d", m_areaCode);
	TLuaExec::CallLuaFunction(0, "GetOutputFileName", "s>s", szExt, szOutputFileName);
	if (0 == strcmp("", szOutputFileName)) // ��ȡ�ļ���ʧ��
	{
		m_logger.info("get output filename failed\n");
		return 1;
	}

	dstFileName = szOutputFileName;

	// ��ȡ����ļ�ȫ·��
	char szOutputFileFullPath[256];
	sprintf_s(szOutputFileFullPath, 256, "%s\\%s", m_outputPath.c_str(), szOutputFileName);
	dstFullPath = szOutputFileFullPath;

	// ����ļ�������չ��
	strcat_s(szOutputFileFullPath, m_immediateExtName.c_str());
	m_logger.info("output file: %s\n", szOutputFileFullPath);

	// ������ļ�
	TLuaExec::CallLuaFunction(0, "HandleOneFileOutput", "ss>i", szOutputFileFullPath, outputContent.c_str(), &ret);
	if (0 != ret)
	{
		m_logger.info("output one file result to dest file failed\n");
		return 2;
	}	

	return 0;
}