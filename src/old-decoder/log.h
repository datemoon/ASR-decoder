#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

#define __func__ __FUNCTION__

#include "src/util/namespace-start.h"
#define LOGERR(format,args...) \
	fprintf(stderr,"file:%s line:%d func:%s:" format,__FILE__,__LINE__,__func__,##args)

#ifdef DEBUG
#define LOGDEBUG(format,args...) \
	fprintf(stderr,"file:%s line:%d func:%s:" format,__FILE__,__LINE__,__func__,##args)
#else
#define LOGDEBUG(format,args...)
#endif
#include "src/util/namespace-end.h"
#endif
