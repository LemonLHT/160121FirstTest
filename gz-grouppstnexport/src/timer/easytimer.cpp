#include "stdafx.h"
#include "easytimer.h"
#include "timerprocess.h"



#define new DEBUG_NEW

class EasyTimerTask : public TimerTask
{
public:
	EasyTimerTask(TimerPro* timerPro,UserTransferMessage utm)
		: m_timerPro(timerPro)
		, m_utm(utm)
	{
	}

	void onTimer()
	{
		m_timerPro->response(m_utm);
		
		delete this;
	}

	TimerPro* m_timerPro;
	UserTransferMessage m_utm;
};

EasyTimer::EasyTimer(TimerPro* timerpro)
	:m_timerPro(timerpro)
{
}

TimerID EasyTimer::set(u32 expires,UserTransferMessage utm)
{
	return ActiveTimer::set(expires, new EasyTimerTask(m_timerPro,utm));
}

void    EasyTimer::cancel(TimerID timerid)
{
	EasyTimerTask* task = (EasyTimerTask*) ActiveTimer::cancel(timerid);

	delete task;
}
