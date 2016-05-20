
#ifndef _FTP_CLIENT_H_
#define _FTP_CLIENT_H_

#include <string>
#include <curl.h>
#include <list>

using namespace std;

#define LoadFtpConfFailed 100
#define NotNeedFtp 101

struct FtpClientConf
{
	string storefilepath;
	string fileprefix;
	int needftp;
	string ftpserver;
	string ftpport;
	string ftpuser;
	string password;
	string downloadpath;
	FtpClientConf()
	{
		storefilepath="";
		fileprefix="";
		needftp =0;
		ftpserver ="";
		ftpport ="";
		ftpuser ="";
		password ="";
		downloadpath ="";
	}

	bool isRight()
	{
		if (ftpserver.empty()||ftpuser.empty()||password.empty())
		{
			return false;
		}

		return true;
	}
};

struct MemoryStruct 
{
	char *memory;
	size_t size;
};

struct FtpFile 
{
	const char *filename;
	FILE *stream;
};

class FtpClient
{
public:
	FtpClient();
	~FtpClient();

	int uploadFile(const char * dsturl,const char * userpwd, const char * localfilename);

protected:
	static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
	CURLcode getInMemory(const char * url,const char * userpwd,string& str);
	static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream);
	CURLcode downloadFile(const char * url,const char * userpwd,const char * filename,int conTimeOut=10,int timeOut=60);
	int downloadFile(char * section,const char * filename);
	int downloadFixFile(char * section,const char * filename,string fileprefix);
	bool  loadFtpConf(char * section,FtpClientConf &ftp);
	CURLcode removeFile(const char * url,const char * userpwd,const char * filename);
	int removeFile(char * section,const char * filename);
	int removeFixFile(char * section,const char * filename,char* fileprefix);

	int getDirectoryFile(list<string>& filelist,char * section,char* fileprefix);
	int getDirectoryFile(list<string>& filelist,char * section); //获得目录下文件 1 关口局向程控话单 2 长途局向程控话单

	virtual string GetNetType() { return ""; }
};

#endif