/*
 * logConfig.hpp
 *
 *  Created on: Aug 4, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_LOG_LOGCONFIG_HPP_
#define INCLUDE_FLEXIBITY_LOG_LOGCONFIG_HPP_

#include <list>
#include <memory>
#include <iomanip>
#include <iostream>
#include <string>
#include <syslog.h>
#include "flexibity/log.h"

#include "flexibity/log/logConfig.h"
#include "flexibity/log/syslog.h"

namespace Flexibity{

static std::map<log::severity, int> syslogmap = {
	{log::severity::INFO, LOG_INFO},
	{log::severity::DEBUG, LOG_DEBUG},
	{log::severity::TRACE, LOG_DEBUG},
	{log::severity::WARNING, LOG_WARNING},
	{log::severity::ERROR, LOG_ERR},
	{log::severity::FATAL, LOG_EMERG}
};

static auto startTime = std::chrono::high_resolution_clock::now();
static std::mutex cout_io_mutex;

void logOut(Flexibity::log::severity sev, std::shared_ptr<Flexibity::log> log, std::string m, std::string fun, int line){
	auto t = std::chrono::high_resolution_clock::now();
	std::lock_guard<std::mutex> lk(Flexibity::cout_io_mutex);
	std::ostringstream oss;
	oss << Flexibity::log::sevToStrColor(sev) << "[" << Flexibity::log::sevToStr(sev) << "]\x1B[0m" << LG_FMT(log->name() << "." << log->f(fun) << log->ln(line) << ": " << m, t);
	logConfig::get()->writeToSink(sev, oss);
}

logConfig::logConfig(){
	//we need to setup _instance here first just to avoid recursion call of static get method
	_instance = std::shared_ptr<Flexibity::logConfig>(this); //TODO: get rid of singleton storage somehow??? do we need this?

	ILOG_INITSev(INFO);
	IDEBUG("Logger registry created");
};

logConfig::logConfig(const Flexibity::logConfig&){};

logConfig::~logConfig(){
	cleanup();
}

void logConfig::cleanup(bool run){
	if(!run){
		IDEBUG("cleanup skipped");
		return;
	}
	lock lk(_loggers_m);
	int del = 0;
	for(auto it = _loggers.begin(); it != _loggers.end();){
		auto il = it->lock();

		if(!il){
			it = _loggers.erase(it);
			del++;
		}else{
			it++;
			ITRACE(il->name() << ".uc: " <<il.use_count());
		}
	}
	IDEBUG(del << " logger links deleted");
	if (_current_sink == "syslog")
		syslog_get().close();
}

void logConfig::dump(){
	std::ostringstream o;
	{
		lock lk(_loggers_m);
		//o << std::endl << "---------------" << std::endl;
		o << std::endl << "--- dumping loggers configuration ---" << std::endl << " logger instances in registry: " << _loggers.size() << std::endl;
		o << std::endl;
		size_t invalid = 0;
		for(auto& i:_loggers){
			if(auto il = i.lock()){
				o << "  " << " (" << il.use_count() - 1 << ")" << il->name() << ": " << il->sevStr() << std::endl; //we need use_count()-1 because we own shared_ptr to logger instance too
			}else
				invalid++;
		}
		o << std::endl << " invalid links: " << invalid << std::endl;
		o << std::endl << " strict config settings (" << _strictConfig.size()  << "):" << std::endl;
		for(auto& i:_strictConfig){
			o << "  " << i.first << ": " << Flexibity::log::sevToStr(i.second) << std::endl;
		}
		o << std::endl << " wild config settings (" << _wildConfig.size()  << "):" << std::endl;
		for(auto& i:_wildConfig){
			o << "  " << i.first << ": " << Flexibity::log::sevToStr(i.second) << std::endl;
		}
		o << "---------------" << std::endl;
	}
	IINFO(o.str());
}

bool logConfig::addLogger(std::shared_ptr<Flexibity::log> logger){
	bool doCleanup = false;
	{
		ITRACE("logger " << logger->name() << " requested to add to registry. Registry size now is: " << _loggers.size());
		lock lk(_loggers_m);
		for(auto& i:_loggers){
			if(auto il = i.lock()){
				if(il == logger){
					IWARN("logger " << logger->name() << "already added to registry");
					return false;
				}
			}else{
				doCleanup = true;
			}
		}
		_loggers.push_back(logger);
		IDEBUG("logger " << logger->name() << " successfully added to registry");
	}
	updateLogger(logger);
	cleanup(doCleanup);
	return true;
}

bool logConfig::deleteLogger(std::shared_ptr<Flexibity::log> logger){ return false; }

bool logConfig::updateLogger(std::shared_ptr<log> logger, std::map<std::string, Flexibity::log::severity>& map, size_t match = 1){
	lock lk(_config_m);
	bool matched = false;
	for(auto& i:map){
		if(match == 0)
			return true;
		if(logger->nameMatch(i.first)){
			match--;
			if(match == 0){
				IDEBUG("Logger" << logger->name() << " matches " << i.first << " config entry. " << log::sevToStr(i.second) << " level will be applied. " << match << " iter left.");
				logger->setSev(i.second);
				matched = true;
			}
		}
	}
	return matched;
}

void logConfig::updateLogger(std::shared_ptr<log> logger, size_t match){
	ITRACE_IN("");
	if(!updateLogger(logger, _strictConfig, match))
		updateLogger(logger, _wildConfig, match);
}

void logConfig::writeToSink(log::severity sev, const std::ostringstream &oss) {
	if (_current_sink == "syslog") {
		syslog_get()(syslogmap[sev]);
	}
	std::cout << oss.str();
}


bool logConfig::setSeverityCfg(const Json::Value& cfg){
	//std::lock_guard<std::mutex> lk(_config_m); //avoid deadlock

	if(!cfg.isObject())
		return false;

	for (auto it = cfg.begin(); it != cfg.end(); it++) {
		if(it.key().isString() && (*it).isString()){
			setSeverity(it.key().asString(), (*it).asString()); //TODO: use setSeverityCfg
		}
	}

	return true;
}

void logConfig::setSink(const std::string& sink) {
	// validate
	if (sink != "stdout" && sink != "syslog") {
		_current_sink = "stdout";
		return;
	}

	if (sink == "syslog")
		this->openSyslog();

	_current_sink = sink;
	IINFO("Set sink to: " << _current_sink);
}

bool logConfig::setConfig(const Json::Value& cfg) {
	setSink(cfg["sink"].asString());
	return setSeverityCfg(cfg["loggers"]);
}

bool logConfig::openSyslog() {
//	openlog ("tigerlash", 0, LOG_DAEMON);
	syslog_get().open("tigerlash", 0, LOG_DAEMON);
	std::clog.rdbuf( syslog_get().rdbuf() );
	std::cout.rdbuf( syslog_get().rdbuf() );
	std::cerr.rdbuf( syslog_get().rdbuf() );

	return true;
}

bool logConfig::isWild(const std::string &exp){
	if(exp.find("*") != string::npos || exp.find("?") != string::npos)
		return true;
	return false;
}

void logConfig::setSeverityCfgEntry(const std::string& exp, std::map<std::string, Flexibity::log::severity>& map, Flexibity::log::severity sev){
	lock lk(_config_m);
	auto c = map.find(exp);
	if(c == map.end()){
		map.insert({exp, sev});
		IINFO(exp << ":" << log::sevToStr(sev) << " added");
	}else{
		IINFO(exp << ":" << log::sevToStr(c->second) << " modified to " << log::sevToStr(sev));
		c->second = sev;
	}
}

void logConfig::setSeverityCfgEntry(const std::string& exp, Flexibity::log::severity sev){
	if(isWild(exp))
		setSeverityCfgEntry(exp, _wildConfig, sev);
	else
		setSeverityCfgEntry(exp, _strictConfig, sev);
}

void logConfig::delSeverityCfgEntry(const std::string& exp, std::map<std::string, Flexibity::log::severity>& map){
	lock lk(_config_m);
	auto c = map.find(exp);
	if(c != map.end()){
		map.erase(c);
		IINFO(exp << "removed");
	}
}

void logConfig::delSeverityCfgEntry(const std::string& exp){
	if(isWild(exp))
		delSeverityCfgEntry(exp, _wildConfig);
	else
		delSeverityCfgEntry(exp, _strictConfig);
}

bool logConfig::setSeverity(const std::string& logger, Flexibity::log::severity severity, bool toCfg){
	bool doCleanup = false;
	bool result = false;
	{
		lock lk(_loggers_m);
		ITRACE("logger: " << logger << ", sev: " << Flexibity::log::sevToStr(severity));

		for(auto& i:_loggers){
			if(auto il = i.lock()){

				if(il->nameMatch(logger)){
					il->setSev(severity);
					result = true;
				}
			}else{
				doCleanup = true;
			}
		}
	}
	cleanup(doCleanup);

	if(toCfg || (result && toCfg)){
		setSeverityCfgEntry(logger, severity);
	}
	return result;
};
bool logConfig::setSeverity(const std::string& logger, const std::string& sevStr, bool toCfg){
	Flexibity::log::severity s;
	if(!log::strToSev(sevStr, s))
		return false;

	return setSeverity(logger, s);
};

/*static*/ std::shared_ptr<Flexibity::logConfig> logConfig::get(/*default app config*/){
	if(!_instance){
		// //TODO: static global logger
		//_instance = std::shared_ptr<logConfig>(logConfig(/*default app config*/)); //copy is done here
		new Flexibity::logConfig(/*default app config*/); //no copy here
		log::global();
	}

	return _instance;
}

/*static*/ std::shared_ptr<Flexibity::logConfig> logConfig::_instance = nullptr;
}


#endif /* INCLUDE_FLEXIBITY_LOG_LOGCONFIG_HPP_ */
