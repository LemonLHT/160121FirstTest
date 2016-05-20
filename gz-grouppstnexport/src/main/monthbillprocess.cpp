#include "monthbillprocess.h"
#include <Windows.h>
#include "TLuaExec.h"

MonthBillProcess::MonthBillProcess(unsigned int threadno, const string& nettype, CDMAService* owner) 
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
string MonthBillProcess::GetLogDesc()
{
	char szDesc[200];
	sprintf_s(szDesc, 200, "%d月汇总话单", m_areaCode);
	return szDesc;
}

//求子串
char* substr(const char*str, unsigned start, unsigned end)
{
	unsigned n = end - start;
	static char stbuf[256];
	strncpy(stbuf, str + start, n);
	stbuf[n] = 0; //字串最后加上0
	return stbuf;
}

/************************************************************************/
/* 
功能: 获取本次需要处理的文件名列表，不包含路径
参数:
返回:
*/
/************************************************************************/
int MonthBillProcess::GetNeedHandleFileList(list<string>& fileList)
{
	fileList.clear();

	// 手工实现如下操作
	// 1. 压缩包下载
	// 2. 解压到指定目录

	// 读取解压后所有文件列表
	char szStoredPath[256];
	sprintf_s(szStoredPath, 256, "%s\\*.*", m_storedPath.c_str());
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(szStoredPath, &ffd);
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // 文件夹
		{
			;
		}
		else // 文件
		{
			char *fileName = ffd.cFileName;
			//m_logger.info("[%s] visit file: %s\n", GetLogDesc().c_str(), fileName);

			// 简单过滤文件名
			// 1. 判断文件名长度
			int lenFileName = strlen(fileName);
			if (lenFileName <= 4) continue;
			// 2. 判断扩展名是否为txt
			char *ptr = strrchr(fileName, '.');
			if (!ptr) // 无扩展名，不处理
				continue;

			char *extName;
			int pos = ptr-fileName;//用指针相减 求得索引
			extName = substr(fileName, pos+1, strlen(fileName));

			if (0 != stricmp(extName, "txt")) // 非txt文件
				continue;

			//m_logger.info("[%s] need handle file: %s\n", GetLogDesc().c_str(), fileName);
			fileList.push_back(fileName);
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);
	FindClose(hFind);

	return 0;
}
