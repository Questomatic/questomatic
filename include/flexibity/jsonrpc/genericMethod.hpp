/*
 * genericMethod.hpp
 *
 *  Created on: Nov 24, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_GENERICMETHOD_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_GENERICMETHOD_HPP_

#include <flexibity/jsonrpc/jsonRpcBoost.hpp>

namespace Flexibity {

using namespace std;

class genericMethod:
		public JsonRpcMethod,
		public logInstanceName{

public:
	genericMethod(){
		ILOG_INIT();
	}

	Json::Value responseOK(const Json::Value& v = Json::Value(Json::nullValue)){
		Json::Value response;
		response["code"] = 0;
		response["data"] = v;
		response["message"] = "ok";
		return response;
	}

};

}


#endif /* INCLUDE_FLEXIBITY_JSONRPC_GENERICMETHOD_HPP_ */
