#include "stdafx.h"
#include "ftpclient.h"
#include "cacti/util/IniFile.h"
#include "groupbillservice.h"
#include "cacti/util/FileSystem.h"


using namespace cacti;

FtpClient::FtpClient()
{

}

FtpClient::~FtpClient()
{

}

size_t FtpClient::writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

CURLcode FtpClient::getInMemory(const char * url,const char * userpwd,string& str)
{
	CURL *curl_handle;
	CURLcode res;
	struct MemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */ 
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL,url);
	curl_easy_setopt(curl_handle, CURLOPT_USERPWD,userpwd);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, -1L);

	res=curl_easy_perform(curl_handle);

	curl_easy_cleanup(curl_handle); 

	printf("%lu bytes retrieved\n", (long)chunk.size);

	if(CURLE_OK != res)
	{
		/* we failed */ 
		fprintf(stderr, "curl told us %d\n", res);
	}
	else
		str.assign(chunk.memory);

	if(chunk.memory)
		free(chunk.memory);

	curl_global_cleanup();

	return res;
}

size_t FtpClient::my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct FtpFile *out=(struct FtpFile *)stream;
	if(out && !out->stream) {
		/* open file for writing */ 
		out->stream=fopen(out->filename, "wb");
		if(!out->stream)
			return -1; /* failure, can't open file to write */ 
	}
	return fwrite(buffer, size, nmemb, out->stream);
}

CURLcode FtpClient::downloadFile(const char * url,const char * userpwd, const char * filename,int conTimeOut/* =6 */,int timeOut/* =60 */)
{
	CURL *curl;
    CURLcode res;
	struct FtpFile ftpFile={
		"ftp", /* name to store the file as if succesful */ 
		NULL
	};
    ftpFile.filename=filename;
    curl_global_init(CURL_GLOBAL_DEFAULT);
 
    curl = curl_easy_init();
    if(curl) {
     /*
     * You better replace the URL with one that works!
     */ 
    curl_easy_setopt(curl, CURLOPT_URL,url);
	 curl_easy_setopt(curl, CURLOPT_USERPWD,userpwd);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, conTimeOut); 
	curl_easy_setopt(curl, CURLOPT_TIMEOUT,timeOut);  
	/* Define our callback to get called when there's data to be written */ 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,my_fwrite);
    /* Set a pointer to our struct to pass to the callback */ 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpFile);
	curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 0);//PASV  
 
    /* Switch on full protocol/debug output */ 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, -1L);
 
    res = curl_easy_perform(curl);
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
 
    if(CURLE_OK != res) {
      /* we failed */ 
      fprintf(stderr, "curl told us %d\n", res);
    }
  } 
  curl_global_cleanup();
   if(ftpFile.stream)
	   {
	   fclose(ftpFile.stream); 
	   ftpFile.stream=NULL;
	   }
      /* close the local file */ 
  return res;
}

CURLcode FtpClient::removeFile(const char * url,const char * userpwd,const char * filename)
{
	CURL *curl_handle;
	CURLcode res;
	struct curl_slist *headerlist=NULL;
	string cmd = "DELE ";
	cmd+=filename;

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */ 
	curl_handle = curl_easy_init();

	headerlist = curl_slist_append(headerlist, cmd.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_URL,url);
	curl_easy_setopt(curl_handle, CURLOPT_USERPWD,userpwd);
	curl_easy_setopt(curl_handle, CURLOPT_POSTQUOTE, headerlist);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, -1L);

	res=curl_easy_perform(curl_handle);

	curl_easy_cleanup(curl_handle); 

	if(CURLE_OK != res)
	{
		/* we failed */ 
		fprintf(stderr, "curl told us %d\n", res);
	}

	curl_global_cleanup();

	return res;

}


bool FtpClient::loadFtpConf(char * section,FtpClientConf &ftp)
{
	IniFile ftpconf;
	if(!ftpconf.open(CFG_CONF))
		return false;

	ftp.storefilepath = ftpconf.readString(GetNetType().c_str(),"StoreFilePath","");
	FileSystem::createMulDir(ftp.storefilepath);
	ftp.fileprefix = ftpconf.readString(section,"CDRFilePrefix","");
	ftp.needftp = ftpconf.readInt(section,"NeedFTP",1);
	ftp.ftpserver = ftpconf.readString(section,"FtpServer","");
	ftp.ftpport = ftpconf.readString(section,"FtpPort","21");
	ftp.ftpuser = ftpconf.readString(section,"FTPUser","");
	ftp.password = ftpconf.readString(section,"FTPPassword","");
	ftp.downloadpath = ftpconf.readString(section,"DownloadPath","/");

	if (!ftp.isRight())
	{
		return false;
	}

	return true;
}
int FtpClient::getDirectoryFile(list<string>& filelist,char * section,char* fileprefix)
{
	FtpClientConf ftp;
	if (!loadFtpConf(section,ftp))
	{
		return LoadFtpConfFailed;
	}
	if (ftp.needftp == 0)
	{
		return NotNeedFtp;
	}

	char buffer[256];
	time_t rawtime;
	struct tm * timeinfo = NULL;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	strftime (buffer,256,"%Y/%Y%m/billing/",timeinfo);
	strcat_s(buffer,256,GetNetType().c_str());

	string str;
	string url = "ftp://"+ftp.ftpserver+":"+ftp.ftpport+"/"+ftp.downloadpath+"/"+buffer+"/"+fileprefix+"/";
	string userpwd = ftp.ftpuser+":"+ftp.password;
	int ret = getInMemory(url.c_str(),userpwd.c_str(),str);	// 获取到的目录文件列表应该是以 \r\n 分隔
	if (ret !=0 )
	{
		return ret;
	}

	sprintf_s(buffer,256,"%s_%s_",fileprefix,GetNetType().c_str());
	size_t  found=str.find(buffer);
	if(found==string::npos)
		return false;
	
	while (found!=string::npos)
	{
		// 从文件名的位置开始，查找结束符所在位置
		size_t lineEndPos = str.find("\r\n", found);
		if (lineEndPos == string::npos)	// 该分支为异常情况
			break;

		int fileNameLen = lineEndPos - found;
		string temp=str.substr(found,fileNameLen);

		// 排除后缀为 .tmp 的文件
		if (fileNameLen > 4)
		{
			string subprefix = str.substr(found+fileNameLen-4, 4);
			if (subprefix != ".tmp")
			{
				filelist.push_back(temp);
			}
		}
		
		//printf("filename:%s\n", temp.c_str());
		//cacti::Logger::getInstance("CExport").info("ftp file: %s\n", temp.c_str());
		found=str.find(buffer,found+fileNameLen);
	}

	return 0;
}

int FtpClient::getDirectoryFile(list<string>& filelist,char * section)
{
	FtpClientConf ftp;
	if (!loadFtpConf(section,ftp))
	{
		return LoadFtpConfFailed;
	}
	if (ftp.needftp == 0)
	{
		return NotNeedFtp;
	}

	string str;
	string url = "ftp://"+ftp.ftpserver+":"+ftp.ftpport+"/"+ftp.downloadpath+"/";
	string userpwd = ftp.ftpuser+":"+ftp.password;
	int ret = getInMemory(url.c_str(),userpwd.c_str(),str);

	if (ret !=0 )
	{
		return ret;
	}

	size_t  found=str.find(ftp.fileprefix);
	if(found==string::npos)
		return false;

	while (found!=string::npos)
	{
		string temp=str.substr(found,20);
		filelist.push_back(temp);
		found=str.find(ftp.fileprefix,found+20);
	}
	
	return 0;
}

int FtpClient::downloadFixFile(char * section,const char * filename,string fileprefix)
{
	FtpClientConf ftp;
	if (!loadFtpConf(section,ftp))
	{
		return LoadFtpConfFailed;
	}
	if (ftp.needftp == 0)
	{
		return NotNeedFtp;
	}

	string str;
	char buffer[256];
	time_t rawtime;
	struct tm * timeinfo = NULL;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	strftime (buffer,256,"%Y/%Y%m/billing/",timeinfo);
	strcat_s(buffer,256,GetNetType().c_str());
	string url = "ftp://"+ftp.ftpserver+":"+ftp.ftpport+"/"+ftp.downloadpath+"/"+buffer+"/"+fileprefix+"/"+filename;
	string userpwd = ftp.ftpuser+":"+ftp.password;
	string storefile =ftp.storefilepath + "\\"+fileprefix+ "\\";
	FileSystem::createMulDir(storefile);
	storefile+=filename;	

	return downloadFile(url.c_str(),userpwd.c_str(),storefile.c_str());
}


int FtpClient::downloadFile(char * section,const char * filename)
{
	FtpClientConf ftp;
	if (!loadFtpConf(section,ftp))
	{
		return LoadFtpConfFailed;
	}
	if (ftp.needftp == 0)
	{
		return NotNeedFtp;
	}

	string str;
	string url = "ftp://"+ftp.ftpserver+":"+ftp.ftpport+"/"+ftp.downloadpath+"/"+filename;
	string userpwd = ftp.ftpuser+":"+ftp.password;
	string storefile =ftp.storefilepath + "\\"+filename;	

	return downloadFile(url.c_str(),userpwd.c_str(),storefile.c_str());
}

int FtpClient::removeFile(char * section,const char * filename)
{
	FtpClientConf ftp;
	if (!loadFtpConf(section,ftp))
	{
		return LoadFtpConfFailed;
	}
	if (ftp.needftp == 0)
	{
		return NotNeedFtp;
	}

	string str;
	string url = "ftp://"+ftp.ftpserver+":"+ftp.ftpport+"/"+ftp.downloadpath+"/";
	string userpwd = ftp.ftpuser+":"+ftp.password;

	return removeFile(url.c_str(),userpwd.c_str(),filename);
}

int FtpClient::removeFixFile(char * section,const char * filename,char* fileprefix)
{
	FtpClientConf ftp;
	if (!loadFtpConf(section,ftp))
	{
		return LoadFtpConfFailed;
	}
	if (ftp.needftp == 0)
	{
		return NotNeedFtp;
	}

	string str;
	char buffer[256];
	time_t rawtime;
	struct tm * timeinfo = NULL;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	strftime (buffer,256,"%Y/%Y%m/billing/",timeinfo);
	strcat_s(buffer,256,GetNetType().c_str());
	string url = "ftp://"+ftp.ftpserver+":"+ftp.ftpport+"/"+ftp.downloadpath+"/"+buffer+"/"+fileprefix+"/"+filename;
	string userpwd = ftp.ftpuser+":"+ftp.password;

	return removeFile(url.c_str(),userpwd.c_str(),filename);
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	//curl_off_t nread;
	/* in real-world cases, this would probably get this data differently
	as this fread() stuff is exactly what the library already would do
	by default internally */ 
	size_t retcode = fread(ptr, size, nmemb, (FILE*)stream);

	//nread = (curl_off_t)retcode;

	//fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
	//        " bytes from file\n", nread);
	return retcode;
}

/************************************************************************/
/* 
功能: 上传ftp文件
参数:
	dsturl: 目标ftp文件全路径，包含文件名
	userpwd: "用户名:密码"
	localfilename: 本地文件全路径
返回:
	0: 成功
	其它: 失败
*/
/************************************************************************/
int FtpClient::uploadFile(const char * dsturl,const char * userpwd, const char * localfilename)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	FILE *hd_src;
	struct stat file_info;
	curl_off_t fsize;

	struct curl_slist *headerlist=NULL;
	//static const char buf_1 [] = "RNFR " UPLOAD_FILE_AS;
	//static const char buf_2 [] = "RNTO " RENAME_FILE_TO;

	/* get the file size of the local file */ 
	if(stat(localfilename, &file_info)) {
		printf("Couldnt open '%s'\n", localfilename);
		return 1;
	}
	fsize = (curl_off_t)file_info.st_size;

	printf("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", fsize);

	/* get a FILE * of the same file */ 
	hd_src = fopen(localfilename, "rb");

	/* In windows, this will init the winsock stuff */ 
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */ 
	curl = curl_easy_init();
	if(curl) {
		/* build a list of commands to pass to libcurl */ 
		//headerlist = curl_slist_append(headerlist, buf_1);
		//headerlist = curl_slist_append(headerlist, buf_2);

		/* we want to use our own read function */ 
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

		/* enable uploading */ 
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		// password
		curl_easy_setopt(curl, CURLOPT_USERPWD,userpwd);

		/* specify target */ 
		curl_easy_setopt(curl,CURLOPT_URL, dsturl);

		/* pass in that last of FTP commands to run after the transfer */ 
		// 下行不调用也没用问题，如调用的效果为: 先上传到服务器为中间文件，再改名为最终文件名
		//curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);

		/* now specify which file to upload */ 
		curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

		/* Set the size of the file to upload (optional).  If you give a *_LARGE
		option you MUST make sure that the type of the passed-in argument is a
		curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
		make sure that to pass in a type 'long' argument. */ 
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
			(curl_off_t)fsize);

		/* Now run off and do what you've been told! */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));

		/* clean up the FTP commands list */ 
		curl_slist_free_all (headerlist);

		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
	fclose(hd_src); /* close the local file */ 

	curl_global_cleanup();

	return res;
}

/************************************************************************/
/* 
功能: 上传指定文件到ftp
参数:

*/
/************************************************************************/
// int FtpClient::uploadFixFile(char * section,const char * filename,string fileprefix)
// {
// 	FtpClientConf ftp;
// 	if (!loadFtpConf(section,ftp))
// 	{
// 		return LoadFtpConfFailed;
// 	}
// 	if (ftp.needftp == 0)
// 	{
// 		return NotNeedFtp;
// 	}
// 
// 	string str;
// 	char buffer[256];
// 	time_t rawtime;
// 	struct tm * timeinfo = NULL;
// 
// 	time ( &rawtime );
// 	timeinfo = localtime ( &rawtime );
// 	strftime (buffer,256,"%Y/%Y%m/billing/",timeinfo);
// 	strcat_s(buffer,256,GetNetType().c_str());
// 	string url = "ftp://"+ftp.ftpserver+":"+ftp.ftpport+"/"+ftp.downloadpath+"/"+buffer+"/"+fileprefix+"/"+filename;
// 	string userpwd = ftp.ftpuser+":"+ftp.password;
// 	string storefile =ftp.storefilepath + "\\"+fileprefix+ "\\";
// 	FileSystem::createMulDir(storefile);
// 	storefile+=filename;	
// 
// 	return downloadFile(url.c_str(),userpwd.c_str(),storefile.c_str());
// }