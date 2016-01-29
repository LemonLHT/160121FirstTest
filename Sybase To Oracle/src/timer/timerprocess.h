#ifndef  _TIMER_PROCESS_H_
#define _TIMER_PROCESS_H_

#include "cacti/message/TransferMessage.h"

using namespace cacti;

class TimerPro
{
public:
	TimerPro();
	~TimerPro();

public:
	virtual void response(UserTransferMessage utm);	
};

#endif