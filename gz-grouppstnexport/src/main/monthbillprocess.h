// 月汇总话单处理

#pragma once

#include "billprocess.h"

class MonthBillProcess : public BillProcess
{
public:
	MonthBillProcess::MonthBillProcess(unsigned int threadno, const string& nettype, CDMAService* owner);

protected:
	string GetLogDesc();	// 获取日志中的描述信息

private:
	int GetNeedHandleFileList(list<string>& fileList);
};