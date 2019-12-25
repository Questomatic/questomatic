/*
 * config.hpp
 *
 *  Created on: Aug 12, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_CONFIG_HPP_
#define INCLUDE_FLEXIBITY_CONFIG_HPP_

#include "jsoncpp/json/json.h"
#include "flexibity/log.h"

#include <iostream>     // std::ios, std::istream, std::cout
#include <fstream>      // std::filebuf
#include <boost/filesystem.hpp>

namespace Flexibity {

class config:
		public Json::Value,
		public logInstanceName{
public:
	config(std::string fn):Json::Value(){
		_fn = fn;
		ILOG_INITSev(INFO);
		std::filebuf fb;
		if (!fb.open(fn, std::ios::in)) {
			IFATAL(fn << " not found");
		}
		std::istream is(&fb);
		Json::Reader reader;
		bool parsedSuccess = reader.parse(is, *this, false);
		if(!parsedSuccess) {

			IFATAL("config parse error");
			IFATAL(reader.getFormattedErrorMessages());
			IFATAL(this->toStyledString());
		}
		_valid = true;
		IINFO("config file " << boost::filesystem::absolute(_fn).string() << " loaded");
	}
	bool valid(){
		return _valid;
	}
private:
	std::string _fn;
	bool _valid = false;

public:
	static void setConfigFN(std::string fn){
		configFn = fn;
	}
	static std::string configFn;
	static config& get(){
		static config instance = config(configFn);
		if(instance._fn != configFn) //TODO: update config (by FN, datetime, etc...)
			GERROR("Config FN has been updated, but config is still old. Implement config update logic.");
		return instance;
	}
};

/*static*/ std::string config::configFn = "config.json";

}

#endif /* INCLUDE_FLEXIBITY_CONFIG_HPP_ */
