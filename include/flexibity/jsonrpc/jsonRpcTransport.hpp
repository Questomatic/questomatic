/*
 * jsonRpcTransport.hpp
 *
 *  Created on: Jan 21, 2016
 *      Author: rs
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_JSONRPCTRANSPORT_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_JSONRPCTRANSPORT_HPP_

#include <flexibity/jsonrpc/jsonRpcBoost.hpp>
#include <boost/signals2.hpp>

namespace Flexibity {

class jsonRpcTransport:
		public jsonRpcBoost{
public:
	typedef shared_ptr<jsonRpcTransport> sPtr;
	boost::signals2::signal<void()> onDisconnect;
	boost::signals2::signal<void()> onConnect;

	virtual bool isConnected(){
		return true;
	}

	virtual void disconnect(){

	}

	virtual void connect(){

	}
};

}

#endif /* INCLUDE_FLEXIBITY_JSONRPC_JSONRPCTRANSPORT_HPP_ */
