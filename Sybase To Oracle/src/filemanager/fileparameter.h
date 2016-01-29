#ifndef _FILE_PARAMETER_H_
#define _FILE_PARAMETER_H_

#include <string>
#include <iostream>
#include "cacti/util/FileSystem.h"
#include <map>

using namespace std;
using namespace cacti;

struct FileHeader 
{
	string m_headerid;
	string m_version;
	string m_engendertime;
	int m_count;
	
	string m_filename;
	int m_size;
	
	FileHeader()
	{
		m_headerid = "HD";
		m_version = "01";
		m_engendertime = ""; 
		m_count = 0;
		m_size = 0;
		m_filename="";
	}
	void clear()
	{
		m_headerid = "";
		m_version = "";
		m_engendertime = ""; 
		m_count = 0;
		m_size = 0;
		m_filename="";
	}
};

struct PathPara //the path parameter.
{
	FileHeader m_fileHeader;
	string tmppath; //the temp path.
	string uppath; //the formal path.
	string bakuppath;
	string actinfopath;//city path
	string filename; // the file name.
	unsigned int keyid;
	string sysname;
	string pathname;
	string chkname;

	PathPara()
	{
		tmppath="";
		uppath="";
		bakuppath="";
		actinfopath="";
		filename="";
		keyid=0;
		sysname="";
		pathname="";
	}

	void clear() // clear the path parameter.
	{
		tmppath.clear();
		uppath.clear();
		bakuppath.clear();
		filename.clear();
		actinfopath.clear();
		keyid=0;
		m_fileHeader.clear();
		sysname.clear();
		pathname.clear();
		
	}

};

class FileParameter 
{
public:
	FileParameter();
	~FileParameter();

	PathPara getPath(void);
	//创建地市文件名
	void createActInfoName(PathPara & pathpara);
	void createFileName(PathPara & pathpara, int flag,int interflag); //create the file name.
	void createChkName(PathPara & pathpara,int fieltype,int flag=0);//add 
	void clear(){}

	bool createFilePath(PathPara & pathpara); //create the file's path.
	bool createactinfopath(PathPara & pathpara); 
private:
	PathPara m_path;
};


#endif