#ifndef __INC_TAS_H
#define __INC_TAS_H

#include "Service.h"
#include "cacti/mtl/MessageDispatcher.h"
#include "dagwservice.h"

using namespace cacti;

class DAGW : public Service
{
public:
	DAGW():Service("oagw", "V2.0.5_20141017","数据访问网关"){m_pTDBGWService = NULL;m_pMessageDispatcher=NULL; };
	virtual bool start();
	virtual void stop();
	virtual void snapshot();

	MessageDispatcher	*m_pMessageDispatcher;
	TDBGWService			*m_pTDBGWService;	

};
#endif //__INC_TAS_H
