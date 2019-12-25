/*
 * removeMonitorMethod.hpp
 *
 *  Created on: Sep 25, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_REMOVEMONITORMETHOD_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_REMOVEMONITORMETHOD_HPP_


#include <flexibity/jsonrpc/jsonRpcSerial.hpp>
#include <flexibity/jsonrpc/genericMethod.hpp>
#include <flexibity/channelMgr.hpp>

namespace Flexibity{

using namespace std;

class removeMonitorMethod:
		public genericMethod{

public:
	static constexpr const char* command = "remove_monitor";
	removeMonitorMethod(channelMgr::sPtr mgr):
		_mgr(mgr){
		ILOG_INIT();
	}

	virtual void invoke(
	      const Json::Value &params,
	      std::shared_ptr<Response> response) override{
		if(!response){
			return; // if notification, do nothing
		}
		auto chnl = _mgr->getItem(params["channel"].asString());
		chnl->removeMonitor(params);
		Json::Value v;
		response->sendResult(responseOK(v));
	}

protected:

	channelMgr::sPtr _mgr;
};

}


#endif /* INCLUDE_FLEXIBITY_JSONRPC_REMOVEMONITORMETHOD_HPP_ */
