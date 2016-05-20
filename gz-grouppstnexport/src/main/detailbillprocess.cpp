#include "detailbillprocess.h"
#include "TLuaExec.h"

DetailBillProcess::DetailBillProcess(unsigned int threadno, const string& nettype, CDMAService* owner) 
: BillProcess(threadno, nettype, owner)
{

}

/************************************************************************/
/* 
����: ��ȡ��־�д�ӡʱ��������Ϣ
����:
����:
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
����: ��ȡ������Ҫ������ļ����б�������·��
����:
����:
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

	// �����ѯ�ļ��б�
	for (list<string>::iterator it = filelist.begin();it!=filelist.end();++it)
	{
		m_logger.info("[%s] query file: file=%s\n",GetLogDesc().c_str(),(*it).c_str());
	}

	//�����ļ�
	for (list<string>::iterator it = filelist.begin();it!=filelist.end();)
	{
		resultcode = downloadFixFile("GroupFtpConf",(*it).c_str(),buffer);
		m_logger.info("[%s] downloadFile: file=%s resultcode=%d.\n",GetLogDesc().c_str(),(*it).c_str(),resultcode);
		if (resultcode == 0)
		{
			resultcode = removeFixFile("GroupFtpConf",(*it).c_str(),buffer);
			if (0 != resultcode)	// ��Զ��ɾ��ʧ�ܣ����˳�����
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
			// ����ʧ�ܵ������Ҫ������������ڴ˴���ѭ��
			filelist.erase(it++);
		}
	}

	return 0;
}

/************************************************************************/
/* 
����: ��ȡ���ش������ļ����ڴ洢Ŀ¼����Ŀ¼ �����»��ܻ���������Ŀ¼�����ڣ������굥������Ŀ¼Ϊ����
����:
����:
*/
/************************************************************************/

string DetailBillProcess::GetSubDirOfStoredPath()
{
	char szDir[100];
	sprintf_s(szDir, 100, "%d", m_areaCode);
	return szDir;
}
