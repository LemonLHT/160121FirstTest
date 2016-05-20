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
	static bool isComplete(); //�ж��Ƿ�����ɾ��򻰵���
	static void resetAction();//���ü�¼��

protected:
	virtual string GetLogDesc() = 0;	// ��ȡ��־�е�������Ϣ

	string GetNetType() { return m_netTypeName; }

private:
	virtual void onMessage(const ServiceIdentifier& sender, UserTransferMessage& utm);//Message mapping.
	virtual void onDefaultMessage(const ServiceIdentifier& sender, UserTransferMessage& utm);//Default Message mapping.
	virtual void run();
	virtual void response(UserTransferMessage utm);	
	void	onCDMABill(); //����C������
	TimerID setTimer(u32  expires,UserTransferMessage utm); // set the timer.
	void killTimer(TimerID id); // kill timer.
	ServiceIdentifier myIdentifier(u32 ref = 0); // get myself identifier.
	static void addAction(); //���Ӷ�����¼	

	void HandleAllBillFiles(list<string>& filelist);
	bool LoadConfig();

	string GetLastMonthBillFileName();
	string GetFtpParentDir();		// ��ȡ�����ļ����ڵ�ftpĿ¼

	virtual int GetNeedHandleFileList(list<string>& fileList) = 0;
	virtual int HandleOneFileLineByLine(const string& srcFullPath, string& dstFullPath, string& dstFileName);
	virtual string GetSubDirOfStoredPath() { return ""; } // ��ȡ���ش������ļ����ڴ洢Ŀ¼����Ŀ¼ �����»��ܻ���������Ŀ¼�����ڣ������굥������Ŀ¼Ϊ����

protected:
	unsigned int m_areaCode;		// ����
	string m_netTypeName;			// ������������Ŀǰ������ֵ: CDMA PSTN
	Logger& m_logger;

	string m_bakPath;				// ��������Ŀ¼
	string m_storedPath;			// Դ�ļ����·��
	string m_outputPath;			// ����ļ����·��
	int m_bNeedHandleLineByLine;	// �Ƿ���Ҫ���д���Դ�ļ� 0:����Ҫ 1:��Ҫ
	string m_immediateExtName;		// ����ļ����м��ļ���չ�� �� 1.txt.tmp ����ļ�δ���մ������֮ǰ����ʹ�ø���չ��

private:

	static RecursiveMutex			m_lock; //Thread lock.
	BoundedQueue<UserTransferMessage> m_queue; //Message queue.

	bool m_bStop; //Thread stop flag.

	EasyTimer * m_syncTimer;	//the object of the timing class.

	static int m_threadcount;//��¼�߳���
	static int m_doaction; //��¼������򻰵����͵�����
	string m_sourcename ;
	string m_destname;

	string m_upFtpUrlPath;	// �ϴ���ʡIT��Ŀ��FTP·��
	string m_upFtpUserPwd;	// �ϴ���ʡIT���˺�
	CDMAService *m_pOwner;

};
#endif