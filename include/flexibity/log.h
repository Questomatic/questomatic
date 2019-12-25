/*
 * log.h
 *
 *  Created on: Aug 5, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_LOG_H_
#define INCLUDE_FLEXIBITY_LOG_H_


#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <string>
#include <memory>
#include <list>

#include <boost/thread.hpp>

#include <jsoncpp/json/json.h>

namespace Flexibity{

//using namespace std;

#define LG_FMT_TIME(now_t, reft) \
		std::setw(10) << std::setprecision(6) << std::fixed << \
		((float)(std::chrono::duration_cast<std::chrono::microseconds>(now_t - reft)).count()/1000000.f)

#define LG_FMT_TID() \
		"0x" << std::hex << boost::this_thread::get_id() << std::dec

#define LG_FMT(m, now_t) \
        "[" << LG_FMT_TIME(now_t, Flexibity::startTime) << "]" \
				<<" "<< LG_FMT_TID()\
				<<  ": " << m << "\r" << std::endl

/**
 * Use this macro to define per-class, per-instance inline logger. Each instance of object will have its unique logger, that can be set up
 * to show instance name.
 */

#define ILOG_DEF() std::shared_ptr<Flexibity::log> _log;

/**
 * Use this two macros to define per-class static logger. Each instance of the class will share the same logger.
 */
#define ILOG_DEF_STATIC() static std::shared_ptr<Flexibity::log> _log;
#define ILOG_IMPL_STATIC(classname) /*static*/ std::shared_ptr<Flexibity::log> classname::_log;

/**
 * Use this initialization in constructors of the class
 */
#define ILOG_INIT() \
		while(1){ \
			if(!_log){ \
				_log = std::make_shared<Flexibity::log>(Flexibity::log(__PRETTY_FUNCTION__)); \
				Flexibity::logConfig::get()->addLogger(_log); \
			} else \
			  _log->setName(__PRETTY_FUNCTION__); \
			break; \
		};
#define ILOG_INITSev(sev) \
		while(1){ \
			if(!_log){ \
				_log = std::make_shared<Flexibity::log>(Flexibity::log(__PRETTY_FUNCTION__, Flexibity::log::sev)); \
				Flexibity::logConfig::get()->addLogger(_log); \
			} else {\
			  _log->setSev(Flexibity::log::sev); \
			  _log->setName(__PRETTY_FUNCTION__); \
			} \
			break; \
		};

/*class logStream:
		public std::ostringstream{
	basic_ostream& operator<< (basic_ostream& (*pf)(basic_ostream&)){
		//pf == std::endl)

		return *this;
	}
};*/

/**
 * Technical macro, do not use directly
 */
#define ILOGSEV(sev, m, fun) while(_log && _log->canLog(Flexibity::log::sev)){ \
		std::ostringstream s; \
		s << m;\
		Flexibity::logOut(Flexibity::log::sev, _log, s.str(), fun, __LINE__);\
		break;\
    }

#define GLOGSEV(sev, m, fun) do{ auto _log = Flexibity::log::global(); ILOGSEV(sev, m, fun); }while(0)

/**
 * Use this macro to log certain log-level message using inline per-class logger
 */
#define IFATAL(m) ILOGSEV(FATAL,   m, __FUNCTION__)
#define IERROR(m) ILOGSEV(ERROR,   m, __FUNCTION__)
#define IWARN(m)  ILOGSEV(WARNING, m, __FUNCTION__)
#define IINFO(m)  ILOGSEV(INFO,    m, __FUNCTION__)
#define IEVENT(m) ILOGSEV(EVENT,   m, __FUNCTION__)
#define IDEBUG(m) ILOGSEV(DEBUG,   m, __FUNCTION__)
#define ITRACE(m) ILOGSEV(TRACE,   m, __FUNCTION__)
#define ITRACE_IN(m)  ITRACE("in "  << m)
#define ITRACE_OUT(m) ILOGSEV("out " << m)

#define GFATAL(m) GLOGSEV(FATAL,   m, __PRETTY_FUNCTION__)
#define GERROR(m) GLOGSEV(ERROR,   m, __PRETTY_FUNCTION__)
#define GWARN(m)  GLOGSEV(WARNING, m, __PRETTY_FUNCTION__)
#define GINFO(m)  GLOGSEV(INFO,    m, __PRETTY_FUNCTION__)
#define GEVENT(m) GLOGSEV(EVENT,   m, __PRETTY_FUNCTION__)
#define GDEBUG(m) GLOGSEV(DEBUG,   m, __PRETTY_FUNCTION__)
#define GTRACE(m) GLOGSEV(TRACE,   m, __PRETTY_FUNCTION__)

#define LGF(m)    LG(__PRETTY_FUNCTION__ << ": " << m)


}

#include "flexibity/log/logConfig.h"
#include "flexibity/log/log.h"
#include "flexibity/log/logConfig.hpp"
#include <flexibity/log/log.hpp>

#endif /* INCLUDE_FLEXIBITY_LOG_H_ */
