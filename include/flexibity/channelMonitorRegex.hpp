/*
 * channelRegexMonitor.hpp
 *
 *  Created on: Aug 20, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_CHANNELMONITORREGEX_HPP_
#define INCLUDE_FLEXIBITY_CHANNELMONITORREGEX_HPP_

#include <flexibity/jsonrpc/jsonRpcTransport.hpp>
#include <flexibity/ioSerial.hpp>
#include <boost/regex.hpp>
#include <flexibity/monitorRegex.hpp>

#include <flexibity/base64.hpp>

namespace Flexibity{
/**
 * this class is not thread-safe!
 */

class channelMonitorRegex:
	public monitorRegex{

public:
	typedef shared_ptr<channelMonitorRegex> sPtr;

	static constexpr const char* idOption = "id";
	static constexpr const char* dataOption = "data";
	static constexpr const char* serverOption = "server";

	channelMonitorRegex(const Json::Value& cfg, jsonRpcTransport::sPtr rpc = jsonRpcTransport::sPtr()):
		monitorRegex(cfg),
		_rpc(rpc){
		ILOG_INIT();
		_feed = make_shared<matchDispatcher>(this);
		_matchReceiver = _feed;
	}

	bool isControlledBy(jsonRpcTransport::sPtr rpc){
		if(auto ourRpc = _rpc.lock()){
			if(ourRpc == rpc){
				return true;
			}
		}

		return false;
	}

protected:

	class matchDispatcher:
		public ioSerialMsgReceiver{
	public:

		static constexpr const char* matchMethodName = "match";

		class invokeCallback:
			public JsonRpcCallback,
			public logInstanceName{
		public:
			invokeCallback(){
				ILOG_INIT();
			}
			virtual ~invokeCallback() override{

			}
			virtual void response(Json::Value const & response) override{
				IINFO("ack receinved for monitor match: \n" << response.toStyledString())
			}
		};

		matchDispatcher(channelMonitorRegex* mon):
			_mon(mon){
			_sessionCallback = make_shared<invokeCallback>();
		}

		virtual void feed(serialMsgContainer msg) override{
			IINFO("match feed " << log::dump(msg));
			if(auto rpc = _mon->_rpc.lock()){
				Json::Value data;
				data[idOption] = _mon->instanceName();
				auto encodedMsg = base64::encode(*msg);
				data[dataOption] = encodedMsg;
				IDEBUG("data as base64: " << encodedMsg);
				rpc->invoke(matchMethodName, data, _sessionCallback); //TODO: add session break logic
			}
		}
	private:
		channelMonitorRegex* _mon;
		shared_ptr<JsonRpcCallback> _sessionCallback;
	};

	weak_ptr<jsonRpcTransport> _rpc;
	//weak_ptr<serialChannel> _channel; //TODO: add channel information
	ioSerialMsgReceiver::sPtr _feed;
};

}



#endif /* INCLUDE_FLEXIBITY_CHANNELMONITORREGEX_HPP_ */
