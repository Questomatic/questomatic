/*
 * staticInjectMethod.hpp
 *
 *  Created on: Sep 25, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_STATICINJECTMETHOD_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_STATICINJECTMETHOD_HPP_


#include <flexibity/jsonrpc/jsonRpcSerial.hpp>
#include <flexibity/jsonrpc/genericMethod.hpp>
#include <flexibity/inject.hpp>

namespace Flexibity{

using namespace std;

class staticInjectMethod:
		public genericMethod{

public:
	typedef shared_ptr<staticInjectMethod> sPtr;
	static constexpr const char* command = "sinject";
	static constexpr const char* nameOption = "name";
	staticInjectMethod(staticInjectMgr::sPtr im):
		_im(im){
		ILOG_INIT();
	}

	virtual void invoke(
	      const Json::Value &params,
	      std::shared_ptr<Response> response) override{
		if(!response){
			return; // if notification, do nothing
		}
		(*_im->getItem(params[nameOption].asString()))();
		Json::Value v;
		response->sendResult(responseOK(v));
	}

protected:
	staticInjectMgr::sPtr _im;
};

}


#endif /* INCLUDE_FLEXIBITY_JSONRPC_STATICINJECTMETHOD_HPP_ */
