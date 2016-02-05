#include "stdafx.h"
#include "convertprocess.h"
#include "cacti/util/IniFile.h"
#include "main/convertService.h"




ConverProcess::ConverProcess()
:m_logger(cacti::Logger::getInstance("Convertdatabase"))
{
}

ConverProcess::~ConverProcess()
{
	
}

bool ConverProcess::init()
{
	m_logger.info("Starting the db process...\n");
	
	int i ;
	for(i = 0 ; i < MAXCONN; i++)
	{
		dbprocessession *tmp   = new dbprocessession();
		if(tmp)
		{
			if(!tmp->init())
			{
				return false;
			}
		}
		m_dbsession.Put(tmp);
	}

	Service::printConsole("Connect database success\n");
	m_logger.info("NOTE:Connect database success\n");
	m_logger.info("Starting the  conversion (convertprocess)...\n");

	//get the table  columns_name and the clomuns_type  and make sql
	return true;
}

void ConverProcess::uninit()
{
	m_logger.info("Uninit ConverProcess Process.\n");
	//here 
	dbprocessession * tmp = NULL;
	while (m_dbsession.Get(tmp))
	{
		tmp->unint();
		delete tmp;
		tmp = NULL;
	}
	vector<DBProcess *>::iterator  iter = m_dbProcess.begin();
	for(;iter != m_dbProcess.end();iter++)
	{
		if((*iter) != NULL)
		{
			delete *iter;
			*iter = NULL;
		}
	}
		
}
void ConverProcess::stop()
{
	uninit();
}


bool ConverProcess::run()
{
	//get the database source 
	if(!init())
	{
		m_logger.info("Init the ConverProcess class failed.\n");
		return  false;
	}

	Service::printConsole("conversion Process running OK\n");
	//get tab
	if(!get_tabname())
	{
		Service::printConsole("INI FILE is wrong for tabname \n");
		return false;
	}
	//  create pthread and start to conversion  
	if(!start_dbprocess())
	{
		return false;
	}
	vector<DBProcess*>::iterator  iter;
	bool flag = true;
	int  success_process = 0;
	int process_tmp = 0;
	int k = 0;
	dbprocessession *tmp = NULL;
	//wait  pthread  end 
	while(flag)
	{
		flag = false;
		iter =   m_dbProcess.begin();
		for(;iter != m_dbProcess.end();iter++)
		{
			if(!(*iter)->getflag())
			{
				flag = true;
			}
			else
				success_process++;
		}
		for(k=0;k<(success_process-process_tmp);k++)
		{
			m_m_dbsession_tmp.Get(tmp);
			m_dbsession.Put(tmp);
			//Service::printConsole("%d connection \n",m_dbsession.GetCount());
		}
	 process_tmp = success_process;
	 success_process = 0;
	 Sleep(200);//200ms wait
	}

	Service::printConsole("conversion Process quit  sucessed\n");
	return true;
	
}

//
bool ConverProcess::get_tabname()
{
	IniFile  tabfile;
	int i = 1;
	char str[20] = { 0 };
	char str_key[20] = { 0 };
	if(!tabfile.open(CFG_CONF))
	{
		return false;
	}
	m_count = tabfile.readInt("System","PerCount",3);
	while(1)
	{
		memset(str,0,sizeof(str));
		snprintf(str,sizeof(str),"TabName%d", i);

		string tmp = tabfile.readString("Database_orcl",str,"");
		if(tmp == "")
		{
			break;
		}
		m_tabname_oracl.push_back(tmp);
		tmp.clear();
	
		tmp  =  tabfile.readString("Database_sy",str,"");
		// if the num of table is not equal, pop 
		if(tmp == "")
		{
			m_tabname_oracl.pop_back();
			break;	
		}
		m_tabname_sy.push_back(tmp);
	
		i++;
	}
	i = 1;
	while(1)
	{
		
		memset(str,0,sizeof(str));
		memset(str_key,0,sizeof(str_key));
		snprintf(str,sizeof(str),"TabName%d", i);
		snprintf(str_key,sizeof(str_key),"Key%d", i);

		string tmp_tab = tabfile.readString("PeerDatabase_orcl",str,"");
		string tmp_key = tabfile.readString("PeerDatabase_orcl",str_key,"");
		if((tmp_tab == "" )|| (tmp_key == ""))
		{
			break;
		}
		
		
		m_tab_orcl.push_back(tmp_tab);
		m_col_orcl.push_back(tmp_key);
		tmp_tab.clear();
		tmp_key.clear();
		

		tmp_tab  =  tabfile.readString("PeerDatabase_sy",str,"");
		tmp_key = tabfile.readString("PeerDatabase_sy",str_key,"");
		// if the num of table is not equal, pop 
		if((tmp_tab == "" )||( tmp_key==""))
		{
			m_tab_orcl.pop_back();
			m_col_orcl.pop_back();
			break;	
		}
		
		m_tab_sy.push_back(tmp_tab);
		m_col_sy.push_back(tmp_key);
		i++; 
	}
	tabfile.clear();
	return true;

}
//
bool ConverProcess::start_dbprocess()
{
	int count = 0;
	int  num = 0;
	bool f = false;
	if(m_tab_sy.size())
	{
		count = m_tab_sy.size();
		num = count /	PERCOUNT;
		if(count %PERCOUNT)
			num++;
		int i = 0; 
		vector<string>  tmp_orcl;
		vector<string>  tmp_sy;
		vector<string>  col_orcl;
		vector<string>  col_sy;
		for(i = 0; i < num;i++)
		{
			tmp_sy.clear();
			tmp_orcl.clear();
			col_sy.clear();
			col_orcl.clear();
			if(i == num -1)
			{
				if(count %PERCOUNT)
				{
					tmp_orcl.insert(tmp_orcl.begin(),m_tab_orcl.begin()+i*PERCOUNT,m_tab_orcl.begin() + i*PERCOUNT + (count%PERCOUNT));
					tmp_sy.insert(tmp_sy.begin(),m_tab_sy.begin()+i*PERCOUNT,m_tab_sy.begin() + i*PERCOUNT + (count%PERCOUNT));
					col_orcl.insert(col_orcl.begin(),m_col_orcl.begin()+i*PERCOUNT,m_col_orcl.begin() + i*PERCOUNT + (count%PERCOUNT));
					col_sy.insert(col_sy.begin(),m_col_sy.begin()+i*PERCOUNT,m_col_sy.begin() + i*PERCOUNT + (count%PERCOUNT));
					//start conversion  pthread do own process.		

				}
				else
				{			
					tmp_orcl.insert(tmp_orcl.begin(),m_tab_orcl.begin()+i*PERCOUNT,m_tab_orcl.begin() + i*PERCOUNT + PERCOUNT);
					tmp_sy.insert(tmp_sy.begin(),m_tab_sy.begin()+i*PERCOUNT,m_tab_sy.begin() + i*PERCOUNT + PERCOUNT);
					col_orcl.insert(col_orcl.begin(),m_col_orcl.begin()+i*PERCOUNT,m_col_orcl.begin() + i*PERCOUNT + PERCOUNT);
					col_sy.insert(col_sy.begin(),m_col_sy.begin()+i*PERCOUNT,m_col_sy.begin() + i*PERCOUNT + PERCOUNT);

				}	
			}
			else
			{
				tmp_orcl.insert(tmp_orcl.begin(),m_tab_orcl.begin()+i*PERCOUNT,m_tab_orcl.begin() + i*PERCOUNT + PERCOUNT);
				tmp_sy.insert(tmp_sy.begin(),m_tab_sy.begin()+i*PERCOUNT,m_tab_sy.begin() + i*PERCOUNT + PERCOUNT);
				col_orcl.insert(col_orcl.begin(),m_col_orcl.begin()+i*PERCOUNT,m_col_orcl.begin() + i*PERCOUNT + PERCOUNT);
				col_sy.insert(col_sy.begin(),m_col_sy.begin()+i*PERCOUNT,m_col_sy.begin() + i*PERCOUNT + PERCOUNT);
			}
			dbprocessession * dbsession = NULL;
			while(!m_dbsession.TryGet(dbsession) ){Sleep(20);} // need uninit 
			
			m_m_dbsession_tmp.Put(dbsession);
			DBProcess *tmp = new DBProcess(tmp_orcl,tmp_sy,col_orcl,col_sy,'A'+i,dbsession);
			if(!tmp)
			{
				m_logger.info("ERR: new  DBProcess  Failed! ");
				return false;
			}
			m_dbProcess.push_back(tmp);//  need delete 
			f= true;
		}
	}


	count = m_tabname_sy.size();
	// if no table return .
	if(count == 0)
	{
		Service::printConsole("No table!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		return f;
	}
	num = count /	PERCOUNT;
	if(count %PERCOUNT)
		num++;
	int i = 0; 
	vector <string> tmp_orcl;
	vector< string>  tmp_sy;
	for(i = 0; i < num;i++)
	{
		tmp_sy.clear();
		tmp_orcl.clear();
		if(i == num -1)
		{
			if(count %PERCOUNT)
			{
				tmp_orcl.insert(tmp_orcl.begin(),m_tabname_oracl.begin()+i*PERCOUNT,m_tabname_oracl.begin() + i*PERCOUNT + (count%PERCOUNT));
				tmp_sy.insert(tmp_sy.begin(),m_tabname_sy.begin()+i*PERCOUNT,m_tabname_sy.begin() + i*PERCOUNT + (count%PERCOUNT));
				//start conversion  pthread do own process.		
				
			}
			else
			{			
				tmp_orcl.insert(tmp_orcl.begin(),m_tabname_oracl.begin()+i*PERCOUNT,m_tabname_oracl.begin() + i*PERCOUNT + PERCOUNT);
				tmp_sy.insert(tmp_sy.begin(),m_tabname_sy.begin()+i*PERCOUNT,m_tabname_sy.begin() + i*PERCOUNT + PERCOUNT);
				
			}	
		}
		else
		{
			tmp_orcl.insert(tmp_orcl.begin(),m_tabname_oracl.begin()+i*PERCOUNT,m_tabname_oracl.begin() + i*PERCOUNT + PERCOUNT);
			tmp_sy.insert(tmp_sy.begin(),m_tabname_sy.begin()+i*PERCOUNT,m_tabname_sy.begin() + i*PERCOUNT + PERCOUNT);
		}
		dbprocessession * dbsession = NULL;
		while(!m_dbsession.TryGet(dbsession) ); // need uninit 
		
		m_m_dbsession_tmp.Put(dbsession);
		DBProcess *tmp = new DBProcess(tmp_orcl,tmp_sy,i+1,dbsession);
		if(!tmp)
		{
			m_logger.info("ERR: new  DBProcess  Failed! ");
			return false;
		}
		m_dbProcess.push_back(tmp);//  need delete
	}
	return true;
}


//   do nothing
