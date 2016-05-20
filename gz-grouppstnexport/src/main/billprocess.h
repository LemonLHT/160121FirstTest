#ifndef _CDMA_PROCESS_H_
#define _CDMA_PROCESS_H_

#include <string>
#include "cacti/logging/Logger.h"
#include "cacti/kernel/Thread.h"
#include "cacti/util/BoundedQueue.h"
#include "cacti/message/TransferMessage.h"
#include "cacti/mtl/ServiceSkeleton.h"
#include "easytimer.h"
#include "timerprocess.h"
#include "groupbillservice.h"
#include "cacti/util/FileSystem.h"
#include "ftpclient.h"

using namespace std;
using namespace cacti;

const int EXPORT_PROCESS_QUEUE_LEN = 4096;

class TimerPro;
class EasyTimer;
class CDMAService;

typedef enum
{
	eOutputDest_Local = 1,
	eOutputDest_Ftp,
} TOutputDestType;

//The billExport the process of class.

class BillProcess:public Thread,public TimerPro,public FtpClient
{
public:
	BillProcess(unsigned int threadno, const string& nettype, CDMAService* owner);
	~BillProcess();
public:
	bool init(); //Initialization.
	void uninit();//Reverse initialization. 

	bool Process(UserTransferMessage &utm); //Message was added to thread.
	void stop();//Stop the thread.
	static bool isComplete(); //判断是否处理完成局向话单。
	static void resetAction();//重置记录数

protected:
	virtual string GetLogDesc() = 0;	// 获取日志中的描述信息

	string GetNetType() { return m_netTypeName; }

private:
	virtual void onMessage(const ServiceIdentifier& sender, UserTransferMessage& utm);//Message mapping.
	virtual void onDefaultMessage(const ServiceIdentifier& sender, UserTransferMessage& utm);//Default Message mapping.
	virtual void run();
	virtual void response(UserTransferMessage utm);	
	void	onCDMABill(); //处理C网话单
	TimerID setTimer(u32  expires,UserTransferMessage utm); // set the timer.
	void killTimer(TimerID id); // kill timer.
	ServiceIdentifier myIdentifier(u32 ref = 0); // get myself identifier.
	static void addAction(); //增加动作记录	

	void HandleAllBillFiles(list<string>& filelist);
	bool LoadConfig();

	string GetLastMonthBillFileName();
	string GetFtpParentDir();		// 获取话单文件所在的ftp目录

	virtual int GetNeedHandleFileList(list<string>& fileList) = 0;
	virtual int HandleOneFileLineByLine(const string& srcFullPath, string& dstFullPath, string& dstFileName);
	virtual string GetSubDirOfStoredPath() { return ""; } // 获取本地待处理文件所在存储目录的子目录 对于月汇总话单，该子目录不存在，对于详单，该子目录为区号

protected:
	unsigned int m_areaCode;		// 区号
	string m_netTypeName;			// 话单类型名，目前仅两个值: CDMA PSTN
	Logger& m_logger;

	string m_bakPath;				// 话单备份目录
	string m_storedPath;			// 源文件存放路径
	string m_outputPath;			// 输出文件存放路径
	int m_bNeedHandleLineByLine;	// 是否需要逐行处理源文件 0:不需要 1:需要
	string m_immediateExtName;		// 输出文件的中间文件扩展名 如 1.txt.tmp 输出文件未最终处理完成之前，都使用该扩展名

private:

	static RecursiveMutex			m_lock; //Thread lock.
	BoundedQueue<UserTransferMessage> m_queue; //Message queue.

	bool m_bStop; //Thread stop flag.

	EasyTimer * m_syncTimer;	//the object of the timing class.

	static int m_threadcount;//记录线程数
	static int m_doaction; //记录处理局向话单类型的数量
	string m_sourcename ;
	string m_destname;

	string m_upFtpUrlPath;	// 上传到省IT的目的FTP路径
	string m_upFtpUserPwd;	// 上传到省IT的账号
	CDMAService *m_pOwner;

};
#endif