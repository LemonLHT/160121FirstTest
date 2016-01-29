#ifndef _DB_PROCESS_H_
#define _DB_PROCESS_H_

#include "dbconnect/dbconnect.h"
#include "cacti/message/TypeDef.h"
#include "dbconnect/base/baseQuery.h"
#include "cacti/util/IniFile.h"
#include <string>
#include <vector>

using namespace std;
using namespace cacti;

struct Tabtype
{
	string col_name;
	string col_type;
	Tabtype()
	{
		col_name="";
		col_type="";
	}
	void clear() //clear the dbparam
	{
		col_name.clear();
		col_type.clear();
	}
};

typedef vector<Tabtype> vec_ttype;
struct DBParam //database parameter.
{
	string source;
	string name;
	string user;
	string password;
	time_t interval;
	int maxCon;
	string tabname_sy;
	string tabname_orcl;

	DBParam()
	{
		source="";
		name="";
		user="";
		password="";
		interval=0;
		tabname_sy="";
		tabname_orcl="";
		maxCon=0;
	}

	void clear() //clear the dbparam
	{
		source.clear();
		name.clear();
		user.clear();
		password.clear();
		tabname_sy.clear();
		tabname_orcl.clear();
		interval=0;
	}

	bool load_sy(char* filename) //load the dbparam.
	{
		IniFile cfgFile;
		if(!cfgFile.open(filename))
			return false;

		source = cfgFile.readString("Database_sy", "Source", "");
		name = cfgFile.readString("Database_sy", "Name", "");
		user = cfgFile.readString("Database_sy", "User", "");
		password = cfgFile.readString("Database_sy", "Password", "");
		interval     = cfgFile.readInt("Database_sy","Interval",10);
		maxCon   =cfgFile.readInt("Database_sy","maxCon",1);
		tabname_sy  = cfgFile.readString("Database_sy","TabName","");
		cfgFile.clear();

		if(user.empty()) //--------------here should change...
			return false;

		return true;		
	}


	bool load_orcl(char* filename) //load the dbparam.
	{
		IniFile cfgFile;
		if(!cfgFile.open(filename))
			return false;

		source = cfgFile.readString("Database_orcl", "Source", "");
		name = cfgFile.readString("Database_orcl", "Name", "");
		user = cfgFile.readString("Database_orcl", "User", "");
		password = cfgFile.readString("Database_orcl", "Password", "");
		interval     = cfgFile.readInt("Database_orcl","Interval",10);
		maxCon   =cfgFile.readInt("Database_orcl","maxCon",1);
		tabname_orcl = cfgFile.readString("Database_orcl","TabName","");
		cfgFile.clear();

		if(user.empty()||password.empty())
			return false;

		return true;		
	}

};

class DBProcess
{
public:
	DBProcess();
	~DBProcess();
public:
	bool init(); //init the class.
	void uninit(); //uninit the class.
	void get_col_type();// get table name and type.............
	void makesql();
	void getsql(char *sql){strncpy(sql,m_str,sizeof(sql));}
	bool executeUpdate(const char* sql); //update the table.
	BaseQuery* getQuery_sy(){return m_query_sy;}; // get the pointer of BaseQuery.
	BaseQuery* getQuery_orcl(){return m_query_orcl;}; // get the pointer of BaseQuery.
	bool conversion_start();

private:
	DbConnection* m_dbConnection_sy; 
	DbConnection* m_dbConnection_orcl; 
	BaseQuery* m_query_sy;
	BaseQuery* m_query_orcl;
	cacti::Logger& m_logger;
	char m_str[1024];
	DBParam m_dbParam;
	vec_ttype v_type;
	int strindex;
	int numindex;
	int dateindex; 
	bool m_activate_sy; //activate flag.
	bool m_activate_orcl;
};
#endif