#include "stdafx.h"
#include "fileparameter.h"
#include <ctime>
#include <direct.h>
#include "billfile.h"

FileParameter::FileParameter()
{

}

FileParameter::~FileParameter()
{

}

void FileParameter::createFileName(PathPara & pathpara, int flag,int interflag)
{
	//YY D 0001 20151213 001.AVL
	string filename = "";//filenae
	time_t rawtime;
	struct tm * timeinfo;
	char buff[512] = {0};

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	sprintf_s(buff,"%sA%04d",pathpara.sysname.c_str(),interflag);
	filename += buff;
	memset(buff, 0, sizeof(buff));
	strftime (buff,512,"%Y%m%d",timeinfo);
	filename += buff;
	memset(buff, 0, sizeof(buff));
	sprintf_s(buff,"%03d.AVL",flag);

	filename +=buff;

	pathpara.filename=filename;
	pathpara.m_fileHeader.m_filename = filename;//wenjianmingÎÄ¼þÃû
	
}
void FileParameter::createChkName(PathPara & pathpara, int filetype,int interflag)
{
	string filename = "";//filenae
	time_t rawtime;
	struct tm * timeinfo;
	char buff[512] = {0};


	switch (filetype)
	{
	case ProvFile: //bill
		{
			sprintf_s(buff,"%sA%04d",pathpara.sysname.c_str(),interflag);
			
		}
		break;
	case ActInfoFile://actinfo
		{
			sprintf_s(buff,"%sA%s",pathpara.sysname.c_str(),"0001");
		}
		break;
	default:
		break;
	}

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	
	filename += buff;
	memset(buff, 0, sizeof(buff));
	strftime (buff,512,"%Y%m%d",timeinfo);
	filename += buff;
	memset(buff, 0, sizeof(buff));
	sprintf_s(buff,".CHK");

	filename +=buff;

	pathpara.chkname = filename;
}

bool FileParameter::createFilePath(PathPara & pathpara)
{
	
	if(!FileSystem::createMulDir(pathpara.uppath))
		return false;
	if(!FileSystem::createMulDir(pathpara.tmppath))
		return false;
	if(!FileSystem::createMulDir(pathpara.bakuppath))
		return false;
	return true;
}

bool FileParameter::createactinfopath(PathPara & pathpara)
{
	if(!FileSystem::createMulDir(pathpara.actinfopath))
		return false;
	return true;
}

PathPara FileParameter::getPath(void)
{
	return m_path;//PathPara
}

void FileParameter::createActInfoName(PathPara & pathpara)//QUXIAO 
{
	//YY D 0001 20151213 000.AVL
	
	time_t rawtime;
	struct tm * timeinfo;
	char buff[512] = {0};

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	sprintf_s(buff,"%sD");
	strftime (buff,512,"%Y%m%d",timeinfo);

	pathpara.m_fileHeader.m_engendertime= buff;
	pathpara.filename += buff;

	sprintf_s(buff,".txt");

	pathpara.filename +=buff;

	//pathpara.filename=filename;
}