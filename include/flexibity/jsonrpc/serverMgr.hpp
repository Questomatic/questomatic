/*
 * serverMgr.hpp
 *
 *  Created on: Oct 8, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_SERVERMGR_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_SERVERMGR_HPP_

#include "flexibity/jsonrpc/jsonRpcSerial.hpp"
#include "flexibity/jsonrpc/jsonRpcWebsocketClient.hpp"
#include "flexibity/genericMgr.hpp"

namespace Flexibity{

class serverMgr:
		public genericMgr<jsonRpcTransport::sPtr>{

public:

	static constexpr const char* uriOption = "uri";
	static constexpr const char* nameOption = "name";

	static constexpr const char* serialPrefix = "serial://";
	static constexpr const char* wsPrefix = "ws://";
	static constexpr const char* wssPrefix = "wss://";

	serverMgr(const Json::Value& cfg, serialPortMgr::sPtr pm){
		ILOG_INIT();

		populateItems(cfg, [&](const Json::Value& iCfg){
			return serverFactory(iCfg, pm);
		});
	}

	static jsonRpcTransport::sPtr serverFactory(const Json::Value& iCfg, serialPortMgr::sPtr pm){
		string uri = iCfg[uriOption].asString();
		string name = iCfg[nameOption].asString();
		//setInstanceName(name);

		auto resource = getResource(uri, serialPrefix);
		if (resource.length() > 0) {
			auto srv = make_shared<jsonRpcSerial>(pm, resource);
			srv->setInstanceName(name);
			return srv;
		}

		resource = getResource(uri, wsPrefix);
		if (resource.length() > 0) {
			return make_shared<jsonRpcWebsocketClient>(iCfg);
		}

		//TODO: wss scheme

		return make_shared<jsonRpcTransport>();
	}

	static const string getResource(const string& uri, const string& prefix){
		auto pos = uri.find(prefix);
		if(pos == 0){
			return string(uri, prefix.length());
		}
		return "";
	}

};

}


#endif /* INCLUDE_FLEXIBITY_JSONRPC_SERVERMGR_HPP_ */
