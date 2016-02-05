#include "stdafx.h"
#include "filemanager.h"
#include "cacti/util/IniFile.h"
#include "main/ConverService.h"

FileManager::FileManager()
:m_logger(cacti::Logger::getInstance("atcinfo_billupload"))
,m_separator(1)
,interflag(0)
,m_linenum(1000)
{
	m_flag = 0;
	if(!readint())
		m_logger.error("%s\r\n","read conf failed for LineNum");
}
bool FileManager::readint()
{
	IniFile exportIni;
	if(!exportIni.open(CFG_CONF))
		return false;
	m_linenum = exportIni.readInt("Control","LineNum",10000);
	exportIni.clear();
	return true;
}

FileManager::~FileManager()
{

}

bool FileManager::init()
{
	if(!load(CFG_CONF))
		return false;
	if(!m_filePara.createFilePath(m_path))
		return false;

	if (!m_filePara.createactinfopath(m_path))
		return false;

	return true;
}

void FileManager::uninit()
{
	
	m_path.clear();
	return;
}
/*
	传入参数：数据库类。 是否出c网话单。 定时的时间vector ， 此次导出时间在vector中的下标
*/
void FileManager::exportBill(DBProcess& dbpro)
{
	char sql[1024];
	memset(sql,0,sizeof(sql));
	dbpro.getsql(sql);
	
	if(dbpro.intindex != 0)


	try
	{
		BaseQuery* query = dbpro.getQuery_sy();
		
		query->command(sql);
		query->execute();

		if(query->eof())
			return;			
		int colindex;
		while(!query->eof())
		{
			colindex=0;
			
		

			query->fetchNext();
			
			billpara.caller = query->getFieldByColumn(colindex++)->asString()
			billpara.called = query->getFieldByColumn(colindex++)->asString();
			billpara.begintime = query->getFieldByColumn(colindex++)->asString();
			billpara.endtime = query->getFieldByColumn(colindex++)->asString();
			billpara.feesum = query->getFieldByColumn(colindex++)->asFloat();
			billpara.duration = query->getFieldByColumn(colindex++)->asUnsignedLong();
			billpara.separator=m_separator;

			}


		}
		
		m_billFile.close(m_path,ProvFile,&rarSize);	

		
		FILE *bakfp=fopen(bakFileName,"a+");
		fprintf(bakfp,"%s\t%6d\t%5d        \r\n",
			m_path.filename.c_str(), rarSize, index
		);
		fclose(bakfp);
			
		
	}
	catch(BaseException& err)
	{
		m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
		return;
	}
	

	return ;
	
}


void FileManager::exportActInfo(DBProcess& dbpro)
{
	char sql[1024]={0};
	int colindex = 0;
	int size = 0;
	int index = 0;
	BaseQuery* query = dbpro.getQuery();
		
	ActInfoParameter actinfopara;
	
	try
	{

		m_filePara.createFileName(m_path,0,1);			
		m_filePara.createChkName(m_path,ActInfoFile);
		char bakFileName[128]={0};
		sprintf_s(bakFileName,"%s\\%s",m_path.actinfopath.c_str(),m_path.chkname.c_str()); //

		sprintf_s(sql,"select c_billitem, c_rate, c_descript, C_RATETYPE from BILL_ACTINFO");
		query->command(sql);
		query->execute();

		if (!query->eof())
		{
			m_billFile.open(m_path,ActInfoFile);
		}

		while(!query->eof())
		{
			colindex=0;
			
			actinfopara.clear();

			query->fetchNext();
			actinfopara.billitem = query->getFieldByColumn(colindex++)->asString();
			actinfopara.rate = query->getFieldByColumn(colindex++)->asFloat();
			actinfopara.descript = query->getFieldByColumn(colindex++)->asString();
			actinfopara.ratetype = query->getFieldByColumn(colindex++)->asUnsignedLong();

			actinfopara.separator=m_separator;
			
			m_billFile.putActInfoSDR(actinfopara);
			index++;
			
		}
		//add actinfo chk file....
		m_billFile.close(m_path, ActInfoFile, &size);	
		FILE *bakfp=fopen(bakFileName,"a+");
		fprintf(bakfp,"%s\t%6d\t%5d        \r\n",
			m_path.filename.c_str(), size, index
			);
		fclose(bakfp);
	
			
	}
	catch(BaseException& err)
	{
		m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
		return ;
	}
	return ;		
}

bool FileManager::load(char* filename)
{
	//load the configuration.
	IniFile tmpIni;
	if(!tmpIni.open(filename))
		return false;

	m_separator = tmpIni.readInt("Control","Separator",111);
	
	m_path.tmppath = tmpIni.readString("Path","TemPath","");
	m_path.uppath = tmpIni.readString("Path","UpPath","");
	m_path.bakuppath = tmpIni.readString("Path","BakUpPath","");
	
	m_path.actinfopath=tmpIni.readString("Path","ActInfoFilePath","");

	m_path.sysname = tmpIni.readString("Control","SysName","YY");
	tmpIni.clear();

	return true;
}