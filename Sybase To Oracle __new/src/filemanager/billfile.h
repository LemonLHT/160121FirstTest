#ifndef _BILL_FILE_
#define _BILL_FILE_

#include <cstdio>
#include <iostream>
#include <string>
#include "filemanager/fileparameter.h"

using namespace std;

enum
{
	ProvFile = 1,
	ActInfoFile = 2,
};
/*
struct BillKey
{
	string callrefid;
	unsigned int seqno;
	BillKey()
	{
		callrefid="";
		seqno=0;
	}
	void clear()
	{
		callrefid.clear();
		seqno=0;

	}

};
*/
struct BillParameter // the bill parameter.
{
	
	string caller;
	string called;
	string begintime;
	string endtime;
	unsigned int duration;
	float feesum;
	int separator;
	BillParameter()
	{
		caller="";
		called="";
		begintime="";
		endtime="";
		duration=0;
		feesum=0;
		separator=0;
	}

	void clear() //clear the struct of bill parameter.
	{
		caller="";
		called="";
		begintime="";
		endtime="";
		duration=0;
		feesum=0;
		separator=0;
	}
};

struct ActInfoParameter // the bill parameter.
{
	string billitem;
	float rate;
	string descript;
	int separator;
	int ratetype;

	ActInfoParameter()
	{
		billitem="";
		rate = 0;
		descript = "";
		separator=0;
		ratetype=0;
	}

	void clear() //clear the struct of bill city parameter.
	{
		billitem="";
		rate = 0;
		descript = "";
		separator=0;
		ratetype=0;
	}
};

class BillFile
{
public:
	BillFile();
	~BillFile();
public:
	bool open(PathPara &para,int filetype); // open the file.
	void close(PathPara para, int filetype, int *); // close the pointer of the file.
	void setFilePath(PathPara para); // set the path of tel.
	bool putFileHeader(FileHeader& header);
	void putSDR(BillParameter& sdr); //write the mobile CDR to file.
	void putActInfoSDR(ActInfoParameter& sdr); //write the mobile CDR to file.
private:
	int deleteFile(string filename); //delete file.	
	void moveFile(); // move file.
	void copyFile();
private:
	FILE* m_pFile; 
	PathPara m_path; 
};

#endif