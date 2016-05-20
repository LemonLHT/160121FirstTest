#include "detailbillprocess.h"
#include "TLuaExec.h"

DetailBillProcess::DetailBillProcess(unsigned int threadno, const string& nettype, CDMAService* owner) 
: BillProcess(threadno, nettype, owner)
{

}

/************************************************************************/
/* 
功能: 获取日志中打印时的描述信息
参数:
返回:
*/
/************************************************************************/
string DetailBillProcess::GetLogDesc()
{
	char szDesc[200];
	sprintf_s(szDesc, 200, "%s-%d", m_netTypeName.c_str(), m_areaCode);
	return szDesc;
}

/************************************************************************/
/* 
功能: 获取本次需要处理的文件名列表，不包含路径
参数:
返回:
*/
/************************************************************************/
int DetailBillProcess::GetNeedHandleFileList(list<string>& filelist)
{
	char buffer[512]={0};
	sprintf_s(buffer,"%d",m_areaCode);
	int resultcode = getDirectoryFile(filelist,"GroupFtpConf",buffer);

	m_logger.info("[%s] getDirectoryFile: resultcode=%d.\n", GetLogDesc().c_str(), resultcode);

	if (resultcode !=0 ||filelist.empty())
	{	
		m_logger.info("[%s] end. no file\n", GetLogDesc().c_str());
		return -1;
	}

	// 输出查询文件列表
	for (list<string>::iterator it = filelist.begin();it!=filelist.end();++it)
	{
		m_logger.info("[%s] query file: file=%s\n",GetLogDesc().c_str(),(*it).c_str());
	}

	//下载文件
	for (list<string>::iterator it = filelist.begin();it!=filelist.end();)
	{
		resultcode = downloadFixFile("GroupFtpConf",(*it).c_str(),buffer);
		m_logger.info("[%s] downloadFile: file=%s resultcode=%d.\n",GetLogDesc().c_str(),(*it).c_str(),resultcode);
		if (resultcode == 0)
		{
			resultcode = removeFixFile("GroupFtpConf",(*it).c_str(),buffer);
			if (0 != resultcode)	// 如远端删除失败，则退出程序
			{
				m_logger.info("[%s] [ERROR]removeFile failed! file=%s resultcode=%d.\n",GetLogDesc().c_str(),(*it).c_str(),resultcode);
				filelist.erase(it++);
			} 
			else
			{
				++it;
			}
		}
		else
		{
			m_logger.info("[%s] [ERROR]downloadFile failed.\n",GetLogDesc().c_str());
			// 下载失败的情况需要做处理，否则会在此处死循环
			filelist.erase(it++);
		}
	}

	return 0;
}

/************************************************************************/
/* 
功能: 获取本地待处理文件所在存储目录的子目录 对于月汇总话单，该子目录不存在，对于详单，该子目录为区号
参数:
返回:
*/
/************************************************************************/

string DetailBillProcess::GetSubDirOfStoredPath()
{
	char szDir[100];
	sprintf_s(szDir, 100, "%d", m_areaCode);
	return szDir;
}
