/*
 * log.h
 *
 *  Created on: Aug 5, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_LOG_LOG_H_
#define INCLUDE_FLEXIBITY_LOG_LOG_H_

#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <string>
#include <memory>
#include <list>

#include <boost/thread.hpp>

#include <jsoncpp/json/json.h>

#include "flexibity/log.h"

namespace Flexibity{

class logConfig;

using namespace std;

class log: public std::enable_shared_from_this<log>{
public:
	enum severity{
		FATAL,
		ERROR,
		WARNING,
		INFO,
		EVENT,
		DEBUG,
		TRACE,

		COUNT // to count values. do not use
	};

	enum match{
		NONE,
		WILD,
		FULL
	};

	log(const std::string& name = __PRETTY_FUNCTION__, severity sev = INFO);
	virtual ~log();

	match nameMatch(const std::string& namePattern);
	bool canLog(severity sev) const;

	void setSev(severity sev);
	void setInstanceName(const std::string& in);
	void setName(const std::string& n);

	const std::string name()const;
	const std::string instance()const {return _instanceName; };
	const std::string f(const std::string& f = __FUNCTION__)const;
	const std::string ln(int ln)const;
	const severity    sev()const { return _sev; };
	const std::string sevStr()const;

	static int wildcmp(const char *wild, const char *string);

	static const std::string sevToStr(severity sev);
	static const std::string sevToStrColor(severity sev);
	static bool strToSev(const std::string& sevStr, log::severity& sev);

	static std::shared_ptr<log> global();

	static const std::string dump(const char *pcBuf, size_t iLen);
	static const std::string dump(const std::string& buf){
		return Flexibity::log::dump(buf.c_str(), buf.size());
	};
	static const std::string dump(shared_ptr<const std::string> buf){
		if(!buf)
			return "\ntrying to dump nullptr";
		return Flexibity::log::dump(buf->c_str(), buf->size());
	};
private:
	void truncName();
	void erase(size_t pos);
	std::string _prettyName;
	std::string _name;
	std::string _instanceName;
	severity _sev;

	//ILOG_DEF_STATIC();
};

class logInstanceName{
public :
	static constexpr const char* noInstance = "<no-instance>";

	virtual void setInstanceName(const std::string& n){
		if(_log)
			_log->setInstanceName(n);
	}

	virtual const std::string instanceName(){
		if(_log)
			return _log->instance();
		else
			return noInstance;
	}

	virtual const std::string name(){
		if(_log)
			return _log->name();
		else
			return noInstance;
	}
	virtual ~logInstanceName(){}
protected:
	ILOG_DEF()
};

}

#endif /* INCLUDE_FLEXIBITY_LOG_LOG_H_ */
