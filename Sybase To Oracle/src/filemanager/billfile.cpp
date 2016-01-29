#include "stdafx.h"
#include "billfile.h"
#include <stdio.h>

BillFile::BillFile()
:m_pFile(NULL)
{

}

BillFile::~BillFile()
{

}

bool BillFile::open(PathPara &para,int filetype)
{
	string filename = "";
	m_path = para;
	switch (filetype)
	{
	case ProvFile: //bill
		{

			filename = para.tmppath + "\\"+para.filename;

		}
		break;
	case ActInfoFile://actinfo
		{
			filename = para.actinfopath +"\\"+para.filename;
		}
		break;
	default:
		break;
	}

	para.pathname = filename;
	m_pFile = fopen(filename.c_str(),"wt");
	if (m_pFile == NULL)
	{
		return false;
	}

	return true;
}

void BillFile::close(PathPara para, int filetype, int * size)
{
	if(m_pFile != NULL)
	{
		fclose(m_pFile);
		m_pFile=NULL;		
	}
	else
		return;
	FILE *bakfp=fopen(para.pathname.c_str(),"r");
	if(bakfp)
	{
		fseek (bakfp, 0, SEEK_END);
		*size = ftell(bakfp);
		
	}
	fclose(bakfp);


	switch (filetype)
	{
	case ProvFile: //bill
		{

			moveFile();
			copyFile();
		}
		break;
	case ActInfoFile://actinfo
		{
			//do nothing
		}
		break;
	default:
		break;
	}

}

int BillFile::deleteFile(string filename)
{
	if( remove(filename.c_str()) != 0 )//win
		return 0;
	else
		return -1;
}

void BillFile::moveFile()
{
	string oldname=m_path.tmppath+"\\"+m_path.filename;
	string newname=m_path.uppath+"\\"+m_path.filename;

	int result = rename(oldname.c_str(),newname.c_str());

	if ( result == 0 )
		puts ( "AVL File successfully renamed" );
	else
		printf( "Error renaming AVL file,error code=%d\n",GetLastError() );

	return;
}

void BillFile::copyFile()
{
	string upfilename=m_path.uppath+"\\"+m_path.filename;
	string bakfilename=m_path.bakuppath+"\\"+m_path.filename;
	FileSystem::copyfile(upfilename.c_str(),bakfilename.c_str());
}

bool BillFile::putFileHeader(FileHeader& header)
{
	if (m_pFile == NULL)
	{
		return false;
	}
	fprintf(m_pFile,"%s%s%s%06d                                                                \r\n",
		header.m_headerid.c_str(),
		header.m_version.c_str(),
		header.m_engendertime.c_str(),
		header.m_count
		);

	return true;
}

void BillFile::putSDR(BillParameter& sdr)
{
	if (sdr.separator>=0)
	{  
		fprintf(m_pFile,"%s %s %s %s %d %f\r\n",
			sdr.caller.c_str(),
			sdr.called.c_str(),
			sdr.begintime.c_str(),
			sdr.endtime.c_str(),
			sdr.duration,
			sdr.feesum
			);

	}
	else
	{
		fprintf(m_pFile,"%s%c%s%c%s%c%s%c%d%c%f\r\n",
			sdr.caller.c_str(),
			sdr.separator,
			sdr.called.c_str(),
			sdr.separator,
			sdr.begintime.c_str(),
			sdr.separator,
			sdr.endtime.c_str(),
			sdr.separator,
			sdr.duration,
			sdr.separator,
			sdr.feesum
			
			);
	}
	
	fflush(m_pFile);
	return;
}

void BillFile::putActInfoSDR(ActInfoParameter& sdr)
{
	string str="";
	if(sdr.ratetype == 0)
	{
		str="元/分钟";
	}
	else if(sdr.ratetype == 1)
	{
		str="元/次";
	}
	else
	{
		str="元/月";
	}
	if (sdr.separator>=0)
	{
		fprintf(m_pFile,"%s %s %.2f%s\r\n",
			sdr.billitem.c_str(),
			sdr.descript.c_str(),
			sdr.rate,
			str.c_str()
			);
	}
	else
	{
		fprintf(m_pFile,"%s%c%s%c%.2f%s\r\n",
			sdr.billitem.c_str(),
			sdr.separator,
			sdr.descript.c_str(),	
			sdr.separator,
			sdr.rate,
			str.c_str()
			);
	}
	fflush(m_pFile);
	return;
}

void BillFile::setFilePath(PathPara para)
{
	m_path=para;
	return;
}