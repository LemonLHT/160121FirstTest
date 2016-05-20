// œÍµ•¥¶¿Ì

#pragma once

#include "billprocess.h"

class DetailBillProcess : public BillProcess
{
public:
	DetailBillProcess(unsigned int threadno, const string& nettype, CDMAService* owner);

protected:
	string GetLogDesc();

private:
	int GetNeedHandleFileList(list<string>& fileList);
	string GetSubDirOfStoredPath();
};