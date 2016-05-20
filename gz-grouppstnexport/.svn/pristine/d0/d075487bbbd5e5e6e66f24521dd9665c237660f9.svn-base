#ifndef  _EASY_TIMER_H_
#define _EASY_TIMER_H_

#pragma once

#include "cacti/util/Timer.h"
#include "timerprocess.h"

using namespace cacti;

class TimerPro;

class EasyTimer : public ActiveTimer
{
public:
	EasyTimer(TimerPro* timerpro);

	TimerID set(u32 expires,UserTransferMessage utm); // set timer.
	void    cancel(TimerID timerid); //cancel timer.

private:
	TimerPro* m_timerPro; //定时回调类指针
};

#endif 