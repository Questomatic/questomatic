/*
 * jsonRpcWebsocket.hpp
 *
 *  Created on: Aug 14, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_JSONRPCWEBSOCKET_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_JSONRPCWEBSOCKETCLIENT_HPP_

#include <flexibity/jsonrpc/jsonRpcTransport.hpp>
#include <flexibity/ioSerial.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/logger/basic.hpp>
#include <websocketpp/logger/levels.hpp>

#include <websocketpp/config/core.hpp>
#include <websocketpp/config/core_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/config/asio_client.hpp>

#include <boost/thread.hpp>

#include <flexibity/jsonObjMacros.def>
#include <flexibity/lock.hpp>

#include <mutex>

namespace Flexibity{

using namespace std;

using websocketpp::lib::bind;

namespace w_1 = websocketpp::lib::placeholders;
namespace w_2 = websocketpp::lib::placeholders;

/// logger like basic but without flushing stream
template <typename concurrency, typename names>
class stdout_logger : public websocketpp::log::basic<concurrency, names> {
public:
	typedef websocketpp::log::basic<concurrency, names> base;
	typedef websocketpp::log::level level;
	typedef websocketpp::log::channel_type_hint channel_type_hint;

	stdout_logger<concurrency,names>(channel_type_hint::value hint =
		channel_type_hint::access)
	  : websocketpp::log::basic<concurrency,names>(hint), stream_out(&cout) {}

	stdout_logger<concurrency,names>(level channels, channel_type_hint::value hint =
		channel_type_hint::access)
	  : websocketpp::log::basic<concurrency,names>(channels, hint), stream_out(&cout) {}

	void write(level channel, std::string const & msg) {
		write(channel, msg.c_str());
	}

	void write(level channel, char const * msg) {
		typename base::scoped_lock_type lock(base::m_lock);
		if (!this->dynamic_test(channel)) { return; }
		*stream_out << "[" << timestamp << "] "
				  << "[" << names::channel_name(channel) << "] "
				  << msg << "\n";
		stream_out->flush();
	}

private:
	std::ostream *stream_out;
	channel_type_hint::value m_channel_type_hint;

	// COPYPASTE FROM BASIC
	static std::ostream & timestamp(std::ostream & os) {
		std::time_t t = std::time(NULL);
		std::tm lt = websocketpp::lib::localtime(t);
		#ifdef _WEBSOCKETPP_PUTTIME_
			return os << std::put_time(&lt,"%Y-%m-%d %H:%M:%S");
		#else // Falls back to strftime, which requires a temporary copy of the string.
			char buffer[20];
			size_t result = std::strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",&lt);
			return os << (result == 0 ? "Unknown" : buffer);
		#endif
	}
};

struct config_logger : public websocketpp::config::asio_client {
	typedef config_logger type;
	typedef websocketpp::config::asio base;

	typedef base::concurrency_type concurrency_type;

	typedef base::request_type request_type;
	typedef base::response_type response_type;

	typedef base::message_type message_type;
	typedef base::con_msg_manager_type con_msg_manager_type;
	typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

	typedef stdout_logger<concurrency_type, websocketpp::log::elevel> elog_type;
	typedef stdout_logger<concurrency_type, websocketpp::log::alevel> alog_type;

	typedef base::rng_type rng_type;

	struct transport_config : public base::transport_config {
		typedef type::concurrency_type concurrency_type;
		typedef type::alog_type alog_type;
		typedef type::elog_type elog_type;
		typedef type::request_type request_type;
		typedef type::response_type response_type;
		typedef websocketpp::transport::asio::basic_socket::endpoint
			socket_type;
	};

	typedef websocketpp::transport::asio::endpoint<transport_config>
		transport_type;
};

class jsonRpcWebsocketClient:
		public Flexibity::jsonRpcTransport{
	typedef websocketpp::client<config_logger> client;
	typedef jsonRpcWebsocketClient type;
	// pull out the type of messages sent by our config
	typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
	typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
	typedef client::connection_ptr connection_ptr;

	DECL_DEF_OPT_FIELD(uri, string)
	DECL_DEF_OPT_FIELD_DEFAULT(reconnectTimeo, int, 1000)
	static constexpr const char* nameOption = "name";

public:
	enum connState{
		DISCONNECTED,
		CONNECTING,
		CONNECTED,
		DISCONNECTING
	};

protected:
	class outputStream:
			public JsonRpcOutputStream,
			public logInstanceName{
	public:
		typedef shared_ptr<outputStream> sPtr;
		virtual ~outputStream(){}

		outputStream(jsonRpcWebsocketClient* ws):
			_ws(ws){
			ILOG_INITSev(INFO);
		}

		virtual void send(const char * buffer, size_t length){
			if(_ws){
				IDEBUG(Flexibity::log::dump(buffer, length));
				_ws->sendMsgToWS(std::string(buffer, length));
			}else{
				IERROR("feed buffer is not set: "
						<< std::endl << buffer);
			}
		}

	protected:
		jsonRpcWebsocketClient* _ws;
	};

public:
	jsonRpcWebsocketClient(const Json::Value& cfg):jsonRpcTransport(){
		ILOG_INITSev(INFO);
		_os = make_shared<outputStream>(this);

		setOutputStream(_os);

		_endpoint.set_access_channels(websocketpp::log::alevel::none);
		_endpoint.set_error_channels(websocketpp::log::elevel::all);

		// Initialize ASIO
		_endpoint.init_asio();

		// Register our handlers
		_endpoint.set_socket_init_handler(bind(&type::onSocketInit,this,w_1::_1));
		//m_endpoint.set_tls_init_handler(bind(&type::on_tls_init,this,w_1::_1));
		_endpoint.set_message_handler(bind(&type::onMessage,this,w_1::_1,w_2::_2));
		_endpoint.set_open_handler(bind(&type::onOpen,this,w_1::_1));
		_endpoint.set_close_handler(bind(&type::onClose,this,w_1::_1));
		_endpoint.set_fail_handler(bind(&type::onFail,this,w_1::_1));
		_endpoint.set_ping_handler(bind(&type::onPing,this,w_1::_1, w_2::_2));
		_endpoint.set_pong_handler(bind(&type::onPong,this,w_1::_1, w_2::_2));

		if(cfg.isMember(nameOption))
			setInstanceName(cfg[nameOption].asString());

		SET_OPTIONAL_FIELD(reconnectTimeo, Int);


		// Start the ASIO io_service run loop asynchroniously. //TODO: protect _threadPool
		auto sz = _threadPool.size();
		if(sz == 0 || !_t->joinable()){
			IINFO("Creating ws thread");
			_t = _threadPool.create_thread(boost::bind(&jsonRpcWebsocketClient::run, this));
		}/*else
			IINFO("thread pool has " << sz << "threads in");*/

		_uri = cfg[uriOption].asString();

		IINFO("Websocket Control host created with\n uri: " << _uri << "\n reconnect timeo: " << _reconnectTimeo );
	}

	~jsonRpcWebsocketClient(){
		lock l(_state_mutex);
		_state = DISCONNECTING;
		ITRACE("IN");
		_endpoint.stop_perpetual();
		_endpoint.stop();
		ITRACE("Join");
		_threadPool.join_all();
		ITRACE("OUT");
	}

	virtual void setInstanceName(const std::string &n) override{
		_os->setInstanceName(n);
		jsonRpcBoost::setInstanceName(n);
	}

	virtual bool isConnected() override{
		//TODO: protection logic
		auto con = getConnection(_hdl);
		if(!con)
			return false;

		if(con->get_state() != websocketpp::session::state::value::open)
			return false;
		return true;
	}

	virtual void connect() override{
		_endpoint.get_io_service().post(boost::bind(&jsonRpcWebsocketClient::connectOp, this));
	}

	virtual void disconnect() override{
		_endpoint.get_io_service().post(boost::bind(&jsonRpcWebsocketClient::disconnectOp, this));
	}

private:

	void connectOp(){
		if(_uri.size() && !isConnected()){
			IINFO("");
			start(_uri);
		}
	}

	void disconnectOp(){
		IINFO("");
		if(!isConnected()){
			IDEBUG("already disconnected");
			return;
		}

		auto con = getConnection(_hdl);

		if(!con)
			return;
		IINFO("");
		con->close(websocketpp::close::status::normal, "disconnecting");
	}

	void start(const std::string& uri) {
		lock l(_state_mutex);

		//TODO: reconnect logic!
		if(_state != DISCONNECTED){
			IERROR("Invalid state");
			return;
		}

		_uri = uri;
		websocketpp::lib::error_code ec;
		auto con = _endpoint.get_connection(uri, ec);

		if (ec) {
			IERROR(ec.message());
			return;
		}

		//con->set_proxy("http://humupdates.uchicago.edu:8443");

		_endpoint.connect(con);


		_state = CONNECTING;
	}

	void run(){
		IINFO("run loop has started");
		_endpoint.start_perpetual();
		_endpoint.run();
		IERROR("run loop has ended");
	}

	bool onPing(websocketpp::connection_hdl,std::string){
		IINFO("");
		return true;
	}

	void onPong(websocketpp::connection_hdl,std::string){
		IINFO("");
	}

	void onSocketInit(websocketpp::connection_hdl) {
		//m_socket_init = std::chrono::high_resolution_clock::now();
	}
	/*context_ptr on_tls_init(websocketpp::connection_hdl) {
		//m_tls_init = std::chrono::high_resolution_clock::now();
		context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);

		try {
			ctx->set_options(boost::asio::ssl::context::default_workarounds |
							 boost::asio::ssl::context::no_sslv2 |
							 boost::asio::ssl::context::no_sslv3 |
							 boost::asio::ssl::context::single_dh_use);
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
		return ctx;
	}*/

	void onFail(websocketpp::connection_hdl hdl) {
		auto con = getConnection(hdl);
		if(!con) //TODO: reconnect policy!
			return;

		IERROR("Fail handler"<<
		"\n state        :" << con->get_state() <<
		"\n localClose   :" << con->get_local_close_code() <<
		"\n localReason  :" << con->get_local_close_reason() <<
		"\n remoteClose  :" << con->get_remote_close_code() <<
		"\n remoteReason :" << con->get_remote_close_reason() <<
		"\n ec - em      :" << con->get_ec() << " - " << con->get_ec().message());

		lock l(_state_mutex);
		checkReconnect();

		_state = DISCONNECTED;
		_hdl.reset();

		//connect(_uri);
	}

	void onOpen(websocketpp::connection_hdl hdl) {
		IINFO("");
		_hdl = hdl;
		lock l(_state_mutex);
		_state = CONNECTED;
		onConnect();
	}

	void onMessage(websocketpp::connection_hdl hdl, message_ptr m) {
		auto p = m->get_payload();
		IDEBUG(log::dump(p));
		ioSerialMsgReceiver::feed(p);
	}

	void onClose(websocketpp::connection_hdl) {
		IINFO("connection closed!");
		lock l(_state_mutex);

		checkReconnect();

		onDisconnect(); //call Disconnect signal

		//TODO: reconnect logic!
		_state = DISCONNECTED;
		_hdl.reset();
	}

	void reconnectCb(websocketpp::lib::error_code c){
		IINFO("");
		start(_uri);
	}

	void checkReconnect(){
		//use with care! no _state protection
		if(_state != DISCONNECTING)
			_endpoint.set_timer(_reconnectTimeo, boost::bind(&jsonRpcWebsocketClient::reconnectCb, this, _1));
	}

	void sendMsgToWS(const std::string &buffer){
		if(!isConnected()){
			IWARN("Not connected, message not sent " << log::dump(buffer));
			return;
		}

		if(auto con = getConnection(_hdl)) //TODO: reconnect policy!
			con->send(buffer, websocketpp::frame::opcode::text);
	}

	client::connection_ptr getConnection(websocketpp::connection_hdl hdl){
		try{
			if(auto con = _endpoint.get_con_from_hdl(hdl)) //TODO: reconnect policy!
				return con;
			else{
				IDEBUG("Connection Expired!");
				return nullptr;
			}
		}catch(const websocketpp::exception& e){
			IDEBUG("Exception code " << e.code() << " caught: " << e.what());
			return nullptr;
		}
		catch(...){
			IERROR("Unknown Exception!");
			return nullptr;
		}
	}

	outputStream::sPtr _os;
	websocketpp::connection_hdl _hdl;
	boost::thread* _t;

	client _endpoint;
	boost::thread_group _threadPool;

	connState _state = DISCONNECTED;

	mutable lockM _state_mutex;
};

}

#include <flexibity/jsonObjMacros.undef>

#endif /* INCLUDE_FLEXIBITY_JSONRPC_JSONRPCSERIAL_HPP_ */
