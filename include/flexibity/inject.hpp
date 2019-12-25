/*
 * inject.hpp
 *
 *  Created on: Sep 14, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_INJECT_HPP_
#define INCLUDE_FLEXIBITY_INJECT_HPP_

#include "flexibity/log.h"
#include "jsoncpp/json/json.h"
#include "flexibity/channelMgr.hpp"

#include <flexibity/base64.hpp>

namespace Flexibity {

using namespace std;

class inject:
		public logInstanceName
{

public:
	typedef shared_ptr<inject> sPtr;
	static constexpr const char* portOption = "port";
	static constexpr const char* channelOption = "channel";
	static constexpr const char* frontOption = "front";
	static constexpr const char* dataOption = "data";
	static constexpr const char* textOption = "text";
	static constexpr const char* idOption = "id";

public:

	INLINE_CLASS_EXCEPTION();

	inject(const Json::Value& cfg, channelMgr::sPtr cm){
		ILOG_INITSev(INFO);
		setInstanceName(cfg.get(idOption, "").asString());
		if(cfg.isMember(channelOption)){
			if(cfg.get(frontOption, false).asBool()){
				_injectReceiver = cm->getItem(cfg.get(channelOption, "").asString()); //injecting to front
			}else{
				_injectReceiver = cm->getItem(cfg.get(channelOption, "").asString())->getOut(); //injecting to port (rear end)
			}
		}else
			_injectReceiver = cm->_pm->getItem(cfg.get(portOption, "").asString());

		if(!_injectReceiver)
			throw exception("Inject receiver not set for inject \"" + instanceName() + "\"");

		if(cfg.isMember(dataOption)){
			string enc = cfg[dataOption].asString();
			IDEBUG(log::dump(enc));
			_data = base64::decode(enc);
		}else{
			_data = cfg[textOption].asString();
		}

		IINFO("Constructed inject \"" << instanceName() << "\" to \"" << _injectReceiver->instanceName() << "\" with data" << log::dump(_data) );

	}
	void operator ()(){
		IINFO("Inject to " << _injectReceiver->instanceName() << log::dump(_data));
		_injectReceiver->feed(_data);
	}
protected:
	ioSerialMsgReceiver::sPtr _injectReceiver;
	string _data;
};

class staticInjectMgr:
		public genericMgr<inject::sPtr>{
public:
	typedef shared_ptr<staticInjectMgr> sPtr;
	staticInjectMgr(const Json::Value& cfg, channelMgr::sPtr cm){
		ILOG_INIT(	);
		//TODO: rethrow nested???
		populateItems(cfg, [&](const Json::Value& iCfg){
			auto item = new Flexibity::inject(iCfg, cm);
			return inject::sPtr(item);
		});
	}
};

}


#endif /* INCLUDE_FLEXIBITY_INJECT_HPP_ */
