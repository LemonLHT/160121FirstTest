#ifndef _FILE_MANAGER_H_
#define _FILE_MANAGER_H_

#include "db/dbprocess.h"
#include "cacti/logging/Logger.h"
#include "dbconnect/dbconnect.h"
#include "filemanager/fileparameter.h"
#include "filemanager/billfile.h"
#include <string>

using namespace std;
using namespace cacti;

class FileManager
{
public:
	FileManager();
	~FileManager();
	bool init();  // init the class.
	void exportBill(DBProcess& dbpro); //begin export bill.

	void exportPhoneBill(DBProcess&dbpro);
	void exportActInfo(DBProcess& dbpro); //begin export actinfo .
	
	void uninit(); // uninit the class.
private:
	bool readint();
	bool load(char* filename); // load the configuration.
private:
	Logger& m_logger;
	PathPara m_path; // the path's parameter.

	int m_separator; // separator.

	int m_linenum;
	int interflag;//add
	int m_flag;//add
	BillFile m_billFile; //the BillFile object.
	FileParameter m_filePara; //the FileParameter object.
	list<string> m_areaCodeList;


};

#endif