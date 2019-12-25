/*
 * logConfig.h
 *
 *  Created on: Aug 5, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_LOG_LOGCONFIG_H_
#define INCLUDE_FLEXIBITY_LOG_LOGCONFIG_H_


#include <list>
#include <memory>
#include <iomanip>
#include <iostream>
#include <string>
#include <map>

#include "flexibity/log/log.h"
#include "jsoncpp/json/json.h"

#include <flexibity/lock.hpp>

namespace Flexibity{

class log;

class logConfig:
		public std::enable_shared_from_this<logConfig>,
		public logInstanceName{
public:

	bool addLogger(std::shared_ptr<Flexibity::log> logger);

	bool deleteLogger(std::shared_ptr<Flexibity::log> logger);

	bool setSeverity(const std::string& logger, Flexibity::log::severity severity, bool toCfg = true);
	bool setSeverity(const std::string& logger, const std::string& sevStr, bool toCfg = true);

	void setSeverityCfgEntry(const std::string& exp, Flexibity::log::severity sev);
	void delSeverityCfgEntry(const std::string& exp);
	bool setConfig(const Json::Value& cfg);
	bool setSeverityCfg(const Json::Value& cfg);

	void updateLogger(std::shared_ptr<log> logger, size_t match = 1);

	void cleanup(bool run = true);
	void dump();
	void writeToSink(log::severity sev, const std::ostringstream& oss);
	~logConfig();

public:
	static std::shared_ptr<Flexibity::logConfig> get(/*default app config*/);
private:
	lockM _loggers_m;
	lockM _config_m;
	logConfig();
	logConfig(const Flexibity::logConfig&);
	bool updateLogger(std::shared_ptr<log> logger, std::map<std::string, Flexibity::log::severity>& map, size_t match);
	void setSeverityCfgEntry(const std::string& exp, std::map<std::string, Flexibity::log::severity>& map, Flexibity::log::severity sev);
	void delSeverityCfgEntry(const std::string& exp, std::map<std::string, Flexibity::log::severity>& map);
	void setSink(const std::string& sink);
	bool isWild(const std::string& exp);
	bool openSyslog();

	std::list<std::weak_ptr<Flexibity::log>> _loggers;
	std::map<std::string, Flexibity::log::severity> _strictConfig;
	std::map<std::string, Flexibity::log::severity> _wildConfig;
	std::string _current_sink;
	static std::shared_ptr<Flexibity::logConfig> _instance;
};

}


#endif /* INCLUDE_FLEXIBITY_LOG_LOGCONFIG_H_ */
