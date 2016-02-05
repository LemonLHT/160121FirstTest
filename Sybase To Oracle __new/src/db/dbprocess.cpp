#include "stdafx.h"
#include "dbprocess.h"
#include "cacti/util/IniFile.h"
#include "main/convertService.h"

#include "dbprocesssession.h"

std::vector<std::string> spiltChar(std::string str,std::string pattern);
DBProcess::DBProcess()
:m_logger(Logger::getInstance("conversion ")), m_dbmanager(NULL),m_flag(false)
{
	memset(m_str,0,sizeof(m_str));
}

DBProcess::DBProcess(vector<string> & orcl, vector<string> sy, int id,dbprocessession *cp)
:m_logger(Logger::getInstance("conversion ")), m_dbmanager(cp),m_orcl(orcl), m_sy(sy),processId(id),m_flag(false)
{
	memset(m_str,0,sizeof(m_str));
	m_pThread = new Comm::CThread(_sRun, this, false);//   pthread
	assert(m_pThread);
	m_pThread->Resume();
}

DBProcess::DBProcess(vector<string> & orcl, vector<string> sy,vector<string> col_orcl,vector<string> col_sy, char ch,dbprocessession * cp)
:m_logger(Logger::getInstance("conversion ")), m_dbmanager(cp),m_an_orcl(orcl), m_an_sy(sy),process_id(ch),m_flag(false),m_col_orcl(col_orcl),m_col_sy(col_sy)
{
	memset(m_str,0,sizeof(m_str));
	m_pThread = new Comm::CThread(__sRun, this, false);//   pthread
	assert(m_pThread);
	m_pThread->Resume();
}

DBProcess::~DBProcess()
{
	// thread kill
	
}
// init database .....
bool DBProcess::init()
{
// 
	return true;
}

void DBProcess::uninit()
{

}
void DBProcess::get_col_type_sy(string tabname)// get table name and type.............
{
	int colindex;
	char tmp[1024] = { 0 };
	Tabtype tmpttype;
	v_type_sy.clear();
	snprintf(tmp,1024,"SELECT b.name,c.name,a.name,b.usertype,b.colid,b.length,CASE WHEN b.status=0 THEN 'NOT NULL'  WHEN  b.status=8 THEN  'NULL'  END status,d.text  FROM sysobjects a,syscolumns b,systypes c,syscomments d  WHERE a.id=b.id AND b.usertype=c.usertype AND a.type='U'   AND b.cdefault*=d.id and a.name = '%s' ORDER BY b.colid",tabname.c_str());

	m_dbmanager->getquery_sy()->command(tmp);
	m_dbmanager->getquery_sy()->execute();

	if(m_dbmanager->getquery_sy()->eof())
		return;
	try
	{
		while(!m_dbmanager->getquery_sy()->eof())
		{
			colindex=0;
			tmpttype.clear();

			m_dbmanager->getquery_sy()->fetchNext();
			tmpttype.col_name = m_dbmanager->getquery_sy()->getFieldByColumn(colindex++)->asString();
			tmpttype.col_type = m_dbmanager->getquery_sy()->getFieldByColumn(colindex++)->asString();
			v_type_sy.push_back(tmpttype);

		}
	}
	catch(BaseException& err)
	{
		m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
		return;
	}	
}
void DBProcess::get_col_type(string tabname)
{

	int colindex;
	char tmp[1024] = { 0 };
	Tabtype tmpttype;
	v_type.clear();
	snprintf(tmp,1024,"select t.column_name,t.data_type,t.column_id  FROM user_tab_cols t,user_col_comments c WHERE lower(t.table_name) = lower('%s') and c.table_name = t.table_name and c.column_name = t.column_name and t.hidden_column = 'NO' order by t.column_id",tabname.c_str());
	
	m_dbmanager->getquery_orcl()->command(tmp);
	m_dbmanager->getquery_orcl()->execute();

	if(m_dbmanager->getquery_orcl()->eof())
		return;
	try
	{
		while(!m_dbmanager->getquery_orcl()->eof())
		{
			colindex=0;
			tmpttype.clear();

			m_dbmanager->getquery_orcl()->fetchNext();
			tmpttype.col_name = m_dbmanager->getquery_orcl()->getFieldByColumn(colindex++)->asString();
			tmpttype.col_type = m_dbmanager->getquery_orcl()->getFieldByColumn(colindex++)->asString();
			v_type.push_back(tmpttype);

		}
	}
	catch(BaseException& err)
	{
		m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
		return;
	}	
}
//get sql 
void DBProcess::makesql(string tabname)
{
	
	char tmp[1024] = { 0 };
	char str[1024] = { 0 };
	memset(str,0,sizeof(tmp));
	strcpy(str,"select ");
	int i;
	for (i = 0;i < v_type_sy.size();i++)
	{
		memset(tmp,0,sizeof(tmp));
		if( /*(strcmp(v_type[i].col_type.c_str(),"DATE") == 0) ||*/ (strcmp(v_type_sy[i].col_type.c_str(),"datetime") == 0))
		{
			snprintf(tmp,1024," rtrim(convert(char,%s,102))+' '+(convert(char,%s,108)) ",v_type_sy[i].col_name.c_str(),v_type_sy[i].col_name.c_str());		
		}
		else
		{
			if((  (strcmp(v_type[i].col_type.c_str(),"CHAR") == 0)  || (strcmp(v_type[i].col_type.c_str(),"VARCHAR2") == 0) ) &&((strcmp(v_type_sy[i].col_type.c_str(),"int") == 0) || (strcmp(v_type_sy[i].col_type.c_str(),"numberic") == 0)) )
				snprintf(tmp,1024," convert(char,%s) ",v_type_sy[i].col_name.c_str());
			else
				snprintf(tmp,1024," %s ",v_type_sy[i].col_name.c_str());
		}
		
		strcat(str, tmp);
		if (i != (v_type_sy.size()-1))
		{
			strcat(str," , ");
		}
			
	}
	snprintf(m_str,1024,"%s from %s",str,tabname.c_str());

}
bool DBProcess::droptable(string & name)
{

	
	char tmp[1024] = { 0 };

	try
	{
		// exits or not 
		snprintf(tmp,1024, "SELECT COUNT(*) as count FROM all_tables WHERE table_name= '%s_BAK' ",name.c_str());
		m_dbmanager->getquery_orcl()->command(tmp);
		m_dbmanager->getquery_orcl()->execute();
		int colindex=0;
		m_dbmanager->getquery_orcl()->fetchNext();
		int count = m_dbmanager->getquery_orcl()->getFieldByColumn(colindex++)->asLong();
		memset(tmp,0,sizeof(tmp));	
		if(count != 0)
		{
			snprintf(tmp,1024,"drop table %s_BAK ",name.c_str());
			m_dbmanager->getquery_orcl()->command(tmp);
			m_dbmanager->getquery_orcl()->execute();	
			memset(tmp,0,sizeof(tmp));

		}
		
		snprintf(tmp,1024,"create table %s_BAK as select * from %s where 1=1",name.c_str(),name.c_str());
		m_dbmanager->getquery_orcl()->command(tmp);
		m_dbmanager->getquery_orcl()->execute();	

	}
	catch(BaseException& err)
	{
		//Service::printConsole("failed %d\n",err.code);
		
		m_logger.error("%s  %s\r\n",err.name.c_str(),err.description.c_str());
		return false ;
	}
	return true;

	
}
//start to conversion from sybase to oralce
bool  DBProcess::conversion_start(unsigned long timeout)
{
	
	char sql[1024] = { 0 };
	char tmp_str[512] = { 0 };
	char ttmp[512] = { 0 };
	char tt[100] = { 0 };
	int i = 0;
	int index = 0;
	int colindex;
	long totleindex  = 0;
	
	Service::printConsole("[%d] is start to working\n",processId);
	int k;
	for(k= 0; k < m_orcl.size(); k++)
	{
		index = 0;
		totleindex = 0;
		Service::printConsole("[%d] Start to transfering from Sybase_tabname: %s to Oracle_tabname:  %s\n",processId,m_sy[k].c_str(),m_orcl[k].c_str());
		get_col_type_sy(m_sy[k]);// get v_type_sy for sybase;
		get_col_type(m_orcl[k]);// get v_type for orcl
		makesql(m_sy[k]);
		memset(ttmp, 0, sizeof(ttmp));
		snprintf(ttmp,1024, "insert into %s values ( ",m_orcl[k].c_str());
		// exists or not and create the bak table.  first 
		if(!droptable(m_orcl[k]))
		{
			Service::printConsole("drop and create datebase table_bak failed\n");
			return false;
		}
		try
		{

			// truncate table second  /// 
			memset(tt,0,100);
			snprintf(tt,100,"TRUNCATE TABLE %s",m_orcl[k].c_str());
			m_dbmanager->getquery_orcl()->command(tt);
			m_dbmanager->getquery_orcl()->execute();	

			m_dbmanager->getquery_sy()->command(m_str);
			m_dbmanager->getquery_sy()->execute();

			if(m_dbmanager->getquery_sy()->eof())
				return false;			
			
			while(!m_dbmanager->getquery_sy()->eof())
			{
				colindex=0;
				memset(tmp_str,0,1024);
				m_dbmanager->getquery_sy()->fetchNext();
				{
					char tmp[1024] = { 0 };
					for (i = 0;i < v_type.size();i++)
					{
						memset(tmp,0,1024);
						if( (strcmp(v_type[i].col_type.c_str(),"DATE") == 0) || (strcmp(v_type[i].col_type.c_str(),"TIMESTAMP(6)") == 0))
						{
							string t = m_dbmanager->getquery_sy()->getFieldByColumn(colindex++)->asString();
							if(strncmp(t.c_str(),"NULL",4) == 0)
								t = "";
							snprintf(tmp,1024," to_date(trim('%s'),'yyyy.mm.dd hh24:mi:ss') ",t.c_str());
						}
						else
						{
							if( (strcmp(v_type[i].col_type.c_str(),"CHAR") == 0) || (strcmp(v_type[i].col_type.c_str(),"VARCHAR2") == 0)){
								string t = m_dbmanager->getquery_sy()->getFieldByColumn(colindex++)->asString();
								if(strncmp(t.c_str(),"NULL",4) == 0)
									t.clear();
								snprintf(tmp,1024," TRIM('%s ') ",t.c_str());
							}
							if( strcmp(v_type[i].col_type.c_str(),"NUMBER") == 0)
								snprintf(tmp,1024," %f ",m_dbmanager->getquery_sy()->getFieldByColumn(colindex++)->asFloat());
						}
						if(i != v_type.size()-1)
						{
							strncat(tmp," , ",3);
						}
						else
						{
							strncat(tmp," ) ",3);
						}

						strncat(tmp_str,tmp,strlen(tmp));
					}

				}
				snprintf(sql,1024,"%s %s",ttmp,tmp_str);

				m_dbmanager->getquery_orcl()->command(sql);
				m_dbmanager->getquery_orcl()->execute();
				index ++;
				totleindex++;
				if(index == MAXCOLUMS)
				{
					m_dbmanager->getquery_orcl()->command("commit");
					m_dbmanager->getquery_orcl()->execute();	
					index = 0;
				}	
			}

			m_dbmanager->getquery_orcl()->command("commit");
			m_dbmanager->getquery_orcl()->execute();


		}

		catch(BaseException& err)
		{
			m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
			Service::printConsole("[%d] conversion from sybase to oracle failed !!oracle_tabname: %s \n",processId ,m_orcl[k].c_str());

			return false ;
		}

		Service::printConsole("[%d] Transfer  from Sybase to Oracle Successed !!Sybase_tabnae: %s  >> Oracle_tabname: %s .TotleIndex:%ld\n",processId, m_sy[k].c_str(),m_orcl[k].c_str(),totleindex);
	}
	
	Service::printConsole("[%d] Has been finished the work O(∩_∩)O~\n",processId);
	m_flag = true;
	return  true ;
	
}

bool DBProcess::makesql_sec(string tabname,string col)
{
	char tmp[1024] = { 0 };
	char str[1024] = { 0 };
	memset(str,0,sizeof(tmp));
	strcpy(str,"select ");
	int i,j;
	vector<string>  v_tmp = spiltChar(col,",");
	for(j=0;j<v_tmp.size();j++)
	{
			for (i = 0;i < v_type_sy.size();i++)
			{
				if(strncmp(v_type_sy[i].col_name.c_str(), v_tmp[j].c_str(),strlen( v_tmp[j].c_str())) == 0)
				{
					memset(tmp,0,sizeof(tmp));
					if( /*(strcmp(v_type[i].col_type.c_str(),"DATE") == 0) ||*/ (strcmp(v_type_sy[i].col_type.c_str(),"datetime") == 0))
					{
						snprintf(tmp,1024," rtrim(convert(char,%s,102))+' '+(convert(char,%s,108)) ",v_type_sy[i].col_name.c_str(),v_type_sy[i].col_name.c_str());		
					}
					else
					{
						if((  (strcmp(v_type[i].col_type.c_str(),"CHAR") == 0)  || (strcmp(v_type[i].col_type.c_str(),"VARCHAR2") == 0) ) &&((strcmp(v_type_sy[i].col_type.c_str(),"int") == 0) || (strcmp(v_type_sy[i].col_type.c_str(),"numberic") == 0)) )
							snprintf(tmp,1024," convert(char,%s) ",v_type_sy[i].col_name.c_str());
						else
							snprintf(tmp,1024," %s ",v_type_sy[i].col_name.c_str());
					}
					break;
				}		
			}
			if(i == v_type_sy.size())
			{
				Service::printConsole("Wrong:  columns is not right ...\n");
				return false;
			}
			strcat(str, tmp);
			if (j != (v_tmp.size()-1))
			{	
				strcat(str," , ");
			}
	}

	snprintf(m_str,1024,"%s from %s",str,tabname.c_str());
	return true;
}
bool  DBProcess::conversion_start_another(unsigned long timeout)
{

	char sql[1024] = { 0 };
	char tmp_str[512] = { 0 };
	char ttmp[512] = { 0 };
	char tt[100] = { 0 };
	int i  = 0;
	int j= 0;
	int index = 0;
	int colindex;
	long totleindex  = 0;

	Service::printConsole("[%c] is start to working\n",process_id);
	int k;
	for(k= 0; k < m_an_orcl.size(); k++)
	{
		index = 0;
		totleindex = 0;
		
		Service::printConsole("[%c] Start to transfering from Sybase_tabname: %s to Oracle_tabname:  %s\n",process_id,m_an_sy[k].c_str(),m_an_orcl[k].c_str());
		get_col_type_sy(m_an_sy[k]);// get v_type_sy for sybase;
		get_col_type(m_an_orcl[k]);// get v_type for orcl
		if(!makesql_sec(m_an_sy[k],m_col_sy[k]))
		{
			return false;
		}
		memset(ttmp, 0, sizeof(ttmp));
		snprintf(ttmp,1024, "insert into %s ( %s ) values (  ",m_an_orcl[k].c_str(),m_col_orcl[k].c_str());
		// exists or not and create the bak table.  first 
		if(!droptable(m_an_orcl[k]))
		{
			Service::printConsole("drop and create datebase table_bak failed\n");
			return false;
		}
		try
		{

			// truncate table second  /// 
			memset(tt,0,100);
			snprintf(tt,100,"TRUNCATE TABLE %s",m_an_orcl[k].c_str());
			m_dbmanager->getquery_orcl()->command(tt);
			m_dbmanager->getquery_orcl()->execute();	
		
			m_dbmanager->getquery_sy()->command(m_str);
			m_dbmanager->getquery_sy()->execute();
	
			if(m_dbmanager->getquery_sy()->eof())
				return false;			

			while(!m_dbmanager->getquery_sy()->eof())
			{
				colindex=0;
				memset(tmp_str,0,1024);
				m_dbmanager->getquery_sy()->fetchNext();
				{
					//
					vector<string> v_tmp = spiltChar(m_col_orcl[k],",");
 					char tmp[1024] = { 0 };
					for(j = 0;j < v_tmp.size();j++)
					{
						for (i = 0;i < v_type.size();i++)
						{
							if(strncmp(v_tmp[j].c_str(),v_type[i].col_name.c_str(),strlen(v_tmp[j].c_str())) == 0)
							{
								memset(tmp,0,1024);
								if( (strcmp(v_type[i].col_type.c_str(),"DATE") == 0) || (strcmp(v_type[i].col_type.c_str(),"TIMESTAMP(6)") == 0))
								{
									string t = m_dbmanager->getquery_sy()->getFieldByColumn(colindex++)->asString();
									if(strncmp(t.c_str(),"NULL",4) == 0)
										t = "";
									snprintf(tmp,1024," to_date(trim('%s'),'yyyy.mm.dd hh24:mi:ss') ",t.c_str());
								}
								else
								{
									if( (strcmp(v_type[i].col_type.c_str(),"CHAR") == 0) || (strcmp(v_type[i].col_type.c_str(),"VARCHAR2") == 0)){
										string t = m_dbmanager->getquery_sy()->getFieldByColumn(colindex++)->asString();
										if(strncmp(t.c_str(),"NULL",4) == 0)
											t.clear();
										snprintf(tmp,1024," TRIM('%s ') ",t.c_str());
									}
									if( strcmp(v_type[i].col_type.c_str(),"NUMBER") == 0)
										snprintf(tmp,1024," %f ",m_dbmanager->getquery_sy()->getFieldByColumn(colindex++)->asFloat());
								}
								break;

							}
							
						}
						if(j != v_tmp.size()-1)
						{
							strncat(tmp," , ",3);
						}
						else
						{
							strncat(tmp," ) ",3);
						}

						strncat(tmp_str,tmp,strlen(tmp));

					}
					
					//
				}
				snprintf(sql,1024,"%s %s",ttmp,tmp_str);

				m_dbmanager->getquery_orcl()->command(sql);
				m_dbmanager->getquery_orcl()->execute();
				index ++;
				totleindex++;
				if(index == MAXCOLUMS)
				{
					m_dbmanager->getquery_orcl()->command("commit");
					m_dbmanager->getquery_orcl()->execute();	
					index = 0;
				}	
			}

			m_dbmanager->getquery_orcl()->command("commit");
			m_dbmanager->getquery_orcl()->execute();


		}

		catch(BaseException& err)
		{
			m_logger.error("%s %s\r\n",err.name.c_str(),err.description.c_str());
			Service::printConsole("[%c] conversion from sybase to oracle failed !!oracle_tabname: %s \n",process_id ,m_an_orcl[k].c_str());

			return false ;
		}

		Service::printConsole("[%c] Transfer  from Sybase to Oracle Successed !!Sybase_tabnae: %s  >> Oracle_tabname: %s .TotleIndex:%ld\n",process_id, m_an_sy[k].c_str(),m_an_orcl[k].c_str(),totleindex);
	}

	Service::printConsole("[%c] Has been finished the work O(∩_∩)O~\n",process_id);
	
	return  true ;

}
std::vector<std::string> spiltChar(std::string str,std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str+=pattern;//扩展字符串以方便操作
	int size=str.size();
	
	for(int i=0; i<size; i++)
	{
		pos=str.find(pattern,i);
		if(pos<size)
		 {
			std::string s=str.substr(i,pos-i);
			result.push_back(s);
			i=pos+pattern.size()-1;
		}
	 }
	 return result;
}