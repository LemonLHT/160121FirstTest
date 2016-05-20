// �»��ܻ�������

#pragma once

#include "billprocess.h"

class MonthBillProcess : public BillProcess
{
public:
	MonthBillProcess::MonthBillProcess(unsigned int threadno, const string& nettype, CDMAService* owner);

protected:
	string GetLogDesc();	// ��ȡ��־�е�������Ϣ

private:
	int GetNeedHandleFileList(list<string>& fileList);
};