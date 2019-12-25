/*
 * jsonRpcBoost.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_JSONRPCSESSION_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_JSONRPCSESSION_HPP_

#include "flexibity/jsonrpc/jsonRpcTransport.hpp"
#include <boost/asio/steady_timer.hpp>

#include <flexibity/channelMgr.hpp>
#include <flexibity/jsonrpc/serverMgr.hpp>
#include <flexibity/jsonrpc/addMonitorMethod.hpp>
#include <flexibity/jsonrpc/removeMonitorMethod.hpp>
#include <flexibity/jsonrpc/unbreakMethod.hpp>
#include <flexibity/jsonrpc/injectMethod.hpp>
#include <flexibity/jsonrpc/staticInjectMethod.hpp>
#include <flexibity/jsonrpc/updateMethod.hpp>
#include <flexibity/jsonrpc/configMethod.hpp>
#include <flexibity/jsonrpc/audioPlayMethod.hpp>
#include <flexibity/jsonrpc/audioStopMethod.hpp>
#include <flexibity/jsonObjMacros.def>

namespace Flexibity {

using namespace std;

class jsonRpcSession:
		public logInstanceName{
public:
	typedef shared_ptr<jsonRpcSession> sPtr;
	typedef basic_waitable_timer< chrono::steady_clock > steady_timer;


	DECL_OPT_FIELD(server);

	DECL_DEF_OPT_FIELD_DEFAULT(ackTimeout, int, 5000);
	DECL_DEF_OPT_FIELD_DEFAULT(pingPeriod, int, 10000);

public:
	enum state{
		STOP,
		INIT,
		ACTIVE
	};

#define ADD_METHOD(name, ...) \
		auto name = std::make_shared<Flexibity::name>(__VA_ARGS__); \
		_rpcTransport->addMethod(Flexibity::name::command, name);

	jsonRpcSession(const Json::Value& cfg, serialPortMgr::sPtr pm, channelMgr::sPtr channelMgr){
		ILOG_INIT();
		ITRACE("");
		_rpcTransport = serverMgr::serverFactory(cfg[serverOption], pm);

		SET_OPTIONAL_FIELD(ackTimeout, Int);
		SET_OPTIONAL_FIELD(pingPeriod, Int);

		auto ackTimeoutMax = (_pingPeriod * 100) / 95; //95% from _pingPeriod

		if(_ackTimeout > ackTimeoutMax )
			_ackTimeout = ackTimeoutMax;

		if(!_rpcTransport){
			auto errStr = "unable to create server!";
			IERROR(errStr);
			throw boost::system::system_error(boost::system::error_code(1, //TODO:: custom exceptions
					boost::asio::error::get_system_category()), errStr);
		}

		setInstanceName(_rpcTransport->instanceName());

		_rpcTransport->onDisconnect.connect(boost::bind(&jsonRpcSession::onTransportDisconnect, this));
		_rpcTransport->onConnect.connect(boost::bind(&jsonRpcSession::onTransportConnect, this));

		IINFO("creating session \"" << instanceName() << "\" with ackTimeout=" << _ackTimeout << "; pingPeriod=" << _pingPeriod );

		_ackTimeoutTimer = make_shared<steady_timer>(_rpcTransport->getService());
		_pingTimer = make_shared<steady_timer>(_rpcTransport->getService());

		_sessionCallback = make_shared<invokeCallback>(this);
		_channelMgr = channelMgr;

		ADD_METHOD(addMonitorMethod, channelMgr, _rpcTransport);
		ADD_METHOD(removeMonitorMethod, channelMgr);
		ADD_METHOD(unbreakMethod, channelMgr);
		ADD_METHOD(injectMethod, channelMgr);
		ADD_METHOD(updateMethod);
		ADD_METHOD(configMethod, pm);
		ADD_METHOD(audioPlayMethod);
		ADD_METHOD(audioStopMethod);
		//ADD_METHOD(staticInjectMethod, injMgr);
	}
#undef ADD_METHOD

	jsonRpcBoost::sPtr server(){
		return _rpcTransport;
	}

	void start(const Json::Value &params) {
		_startParams = params;
		_state = INIT;
		if(_rpcTransport->isConnected()){
			IINFO(params.toStyledString());
			_rpcTransport->invoke("start", params, _sessionCallback);
		}else{
			IDEBUG("Transport is disconnected, trying to reconnect...");
			_rpcTransport->connect();
		}
		waitAck();
	}

	void stop(){
		IINFO("");
		_ackTimeoutTimer->cancel();
		_pingTimer->cancel();
		_state = STOP;
	}

protected:

	state _state = STOP;

	Json::Value _startParams;

	void onTransportDisconnect(){
		IERROR("Transport was disconnected!");
		stop();
		_channelMgr->clearChannels(_rpcTransport);
	}

	void onTransportConnect(){
		start(_startParams);
		schedulePing();
	}

	void waitAck(){
		_ackTimeoutTimer->expires_from_now(chrono::milliseconds(_ackTimeout));
		_ackTimeoutTimer->async_wait(boost::bind(&jsonRpcSession::onAckTimeout, this, boost::asio::placeholders::error));
	}

	class invokeCallback:
		public JsonRpcCallback,
		public logInstanceName{
	public:
		invokeCallback(jsonRpcSession* sess):
		  _sess(sess){
			ILOG_INIT();
		}
		virtual ~invokeCallback() override{

		}
		virtual void response(Json::Value const & response) override{
			IINFO("\n" << response.toStyledString());
			if(isOK(response))
				_sess->onAck();
		}

		bool isOK(Json::Value const & response){
			return true; //TODO: implement
		}
		jsonRpcSession* _sess;
	};

	void ping() {
		_rpcTransport->invoke("alive", Json::Value(), _sessionCallback);
		schedulePing();
		waitAck();
	}

	void schedulePing(){
		_pingTimer->expires_from_now(chrono::milliseconds(_pingPeriod));
		_pingTimer->async_wait(boost::bind(&jsonRpcSession::onPing, this, boost::asio::placeholders::error));
	}

	void onAck() {
		_ackTimeoutTimer->cancel();
		schedulePing();
		IINFO("Ack received for \"" << _rpcTransport->instanceName() << "\"");
	}

	void onAckTimeout(const boost::system::error_code& e)
	{
		lock l(_ackTimeo_mutex);
		if (e != boost::asio::error::operation_aborted) {
			IWARN("Session timeout!!");
			_pingTimer->cancel();
			_rpcTransport->disconnect();
		}
	}

	void onPing(const boost::system::error_code& e) {
		if (e != boost::asio::error::operation_aborted) {
			IINFO("Session ping!!");
			ping();
		}
	}

	shared_ptr<JsonRpcCallback> _sessionCallback;
	shared_ptr<steady_timer> _ackTimeoutTimer;
	shared_ptr<steady_timer> _pingTimer;
	jsonRpcTransport::sPtr _rpcTransport;

	channelMgr::sPtr _channelMgr;

	mutable lockM _ackTimeo_mutex;
};

class sessionMgr:
		public genericMgr<jsonRpcSession::sPtr>{

public:
	typedef shared_ptr<sessionMgr> sPtr;
	sessionMgr(const Json::Value& cfg, serialPortMgr::sPtr pm, channelMgr::sPtr chnlMgr): _pm(pm), _chnlMgr(chnlMgr) {
		ILOG_INIT();

		populateItems(cfg, [&](const Json::Value& iCfg){
			return createSession(iCfg);
		});
	}

	void start(const Json::Value &params = Json::Value()){
		for(auto& item:*this){
			item.second->start(params);
		}
	}

	void stop(){
		for(auto& item:*this){
			item.second->stop();
		}
	}

	jsonRpcSession::sPtr createSession(const Json::Value& cfg) {
		return make_shared<jsonRpcSession>(cfg, _pm, _chnlMgr);
	}
private:
	serialPortMgr::sPtr _pm;
	channelMgr::sPtr _chnlMgr;
};

#include <flexibity/jsonObjMacros.undef>

}

#endif /* INCLUDE_FLEXIBITY_JSONRPC_JSONRPCBOOST_HPP_ */
