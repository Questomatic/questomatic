/*
 * injectMethod.hpp
 *
 *  Created on: Sep 25, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_INJECTMETHOD_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_INJECTMETHOD_HPP_


#include <flexibity/jsonrpc/genericMethod.hpp>
#include <flexibity/inject.hpp>
#include <flexibity/channelMgr.hpp>

namespace Flexibity{

using namespace std;

class injectMethod:
		public genericMethod{

public:
	static constexpr const char* command = "inject";
	injectMethod(channelMgr::sPtr cm):
		_cm(cm){
		ILOG_INIT();
	}

	virtual void invoke(
	      const Json::Value &params,
	      std::shared_ptr<Response> response) override{
		if(!response){
			return; // if notification, do nothing
		}
		auto i = make_shared<Flexibity::inject>(params, _cm);
		(*i)();
		auto v = Json::Value();
		response->sendResult(responseOK(v));
	}

protected:
	channelMgr::sPtr _cm;
};

}


#endif /* INCLUDE_FLEXIBITY_JSONRPC_INJECTMETHOD_HPP_ */
