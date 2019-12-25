/*
 * addMonitorMethod.hpp
 *
 *  Created on: Sep 25, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_ADDMONITORMETHOD_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_ADDMONITORMETHOD_HPP_

#include <flexibity/jsonrpc/jsonRpcTransport.hpp>
#include <flexibity/jsonrpc/genericMethod.hpp>
#include <flexibity/channelMgr.hpp>

namespace Flexibity{

using namespace std;

class addMonitorMethod:
		public genericMethod{

public:
	static constexpr const char* command = "add_monitor";
	addMonitorMethod(channelMgr::sPtr mgr, jsonRpcTransport::sPtr rpc):
		_mgr(mgr), _rpc(rpc){
		ILOG_INIT();
	}

	virtual void invoke(
	      const Json::Value &params,
	      std::shared_ptr<Response> response) override{
		if(!response){
			return; // if notification, do nothing
		}
		auto chnl = _mgr->getItem(params["channel"].asString());
		auto mntr = chnl->addMonitor(params, _rpc);
		Json::Value v;
		if(mntr == nullptr){
			v = "unable to add monitor";
			response->sendError(v);
			return;
		}
		v[monitorRegex::idOption] = mntr->instanceName();
		response->sendResult(responseOK(v));
	}

protected:

	channelMgr::sPtr _mgr;
	jsonRpcTransport::sPtr _rpc;
};


}


#endif /* INCLUDE_FLEXIBITY_JSONRPC_ADDMONITORMETHOD_HPP_ */
