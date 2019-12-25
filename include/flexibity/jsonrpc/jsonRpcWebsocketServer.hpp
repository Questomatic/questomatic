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

#include <websocketpp/config/debug_asio_no_tls.hpp>

#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_client.hpp>
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

class jsonRpcWebsocketServer:
		public logInstanceName{
	DECL_DEF_OPT_FIELD_DEFAULT(port, int, 9012);

protected:
	class outputStream:
			public JsonRpcOutputStream,
			public logInstanceName{
	public:
		typedef shared_ptr<outputStream> sPtr;
		virtual ~outputStream(){}

		outputStream(jsonRpcWebsocketServer* srv, websocketpp::connection_hdl ws):
			_connection(ws),
			_srv(srv){
			ILOG_INITSev(INFO);
		}

		virtual void send(const char * buf, size_t length){
			IDEBUG(Flexibity::log::dump(buf, length));
			if(auto con = _srv->getConnection(_connection))
				_srv->get_endpoint().send(_connection, buf, length, websocketpp::frame::opcode::text);
			else{
				IERROR("feed buffer is not set: "
						<< std::endl << buf);
			}
		}

	protected:
		websocketpp::connection_hdl _connection;
		jsonRpcWebsocketServer* _srv;
	};

public:
	typedef websocketpp::server<websocketpp::config::debug_asio> server;
	// pull out the type of messages sent by our config
	typedef server::message_ptr message_ptr;

	//boost::signals2::signal<void()> onDisconnect;
	//boost::signals2::signal<void()> onConnect;
	boost::signals2::signal<websocketpp::http::status_code::value(server::connection_ptr con)> onHttp;

	server::connection_ptr getConnection(websocketpp::connection_hdl hdl){
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

	jsonRpcWebsocketServer(const Json::Value& cfg):logInstanceName(){
		ILOG_INITSev(INFO);

		SET_OPTIONAL_FIELD(port, Int);

		_endpoint.clear_access_channels(websocketpp::log::alevel::all);
		_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
		_endpoint.set_access_channels(websocketpp::log::alevel::app);
		_endpoint.init_asio();
		_endpoint.set_reuse_addr(true);

		// Register our message handler
		_endpoint.set_message_handler(bind(&jsonRpcWebsocketServer::on_message,this,::_1,::_2));

		_endpoint.set_open_handler(bind(&jsonRpcWebsocketServer::on_open, this, ::_1));
		_endpoint.set_http_handler(bind(&jsonRpcWebsocketServer::on_http, this, ::_1));
		_endpoint.set_fail_handler(bind(&jsonRpcWebsocketServer::on_fail, this, ::_1));
		_endpoint.set_close_handler(bind(&jsonRpcWebsocketServer::on_close, this, ::_1));

		_endpoint.listen(websocketpp::lib::asio::ip::tcp::v4(), _port);

		// Start the server accept loop
		_endpoint.start_accept();

		// Start the ASIO io_service run loop asynchroniously. //TODO: protect _threadPool
		auto sz = _threadPool.size();
		if(sz == 0 || !_t->joinable()){
			IINFO("Creating ws thread");
			_t = _threadPool.create_thread(boost::bind(&jsonRpcWebsocketServer::run, this));
		}/*else
			IINFO("thread pool has " << sz << "threads in");*/

		IINFO("Websocket Control Server created. Connect with uri: ws:\\<get_ip_here>:" << _port );
	}

	~jsonRpcWebsocketServer(){
		stop();
	}

	server& get_endpoint() {
		return _endpoint;
	}

	void stop(){
		ITRACE("IN");
		_endpoint.stop_perpetual();
		_endpoint.stop();
		ITRACE("Join");
		_threadPool.join_all();
		ITRACE("OUT");
	}

	void join(){
		_threadPool.join_all();
	}

	/// Add a client method that the other endpoint can invoke
	void addMethod(const std::string &methodName,
			jsonRpcBoost::BoostJsonRpcMethod method){

		_methods.insert({methodName, method});
	}

	/// Add a client method that the other endpoint can invoke
	void removeMethod(const std::string &methodName){

		std::unique_lock<std::mutex> lck(_connectionsMutex);
		for (auto &c : _connections) {
			c.second.transport->removeMethod(methodName);
		}

		_methods.erase(methodName);
	}

	void notifyAll(
		    std::string const & methodName,
		    Json::Value const & params){
		std::unique_lock<std::mutex> lck(_connectionsMutex);
		for (auto &c : _connections) {
			//jsonRpc->addMethod(m.first, m.second);
			c.second.transport->invoke(methodName, params, std::weak_ptr<JsonRpcCallback>());
		}
	}

protected:

	void run(){
		IINFO("enter");
		_endpoint.run();
		IINFO("exit");
	}

	void on_open(websocketpp::connection_hdl hdl) {
		IINFO("called with hdl: " << hdl.lock().get());


		auto os = make_shared<outputStream>(this, hdl);

		auto jsonRpc = std::make_shared<Flexibity::jsonRpcTransport>();

		jsonRpc->setOutputStream(os);

		for (auto &m : _methods) {
			jsonRpc->addMethod(m.first, m.second);
		}

		std::unique_lock<std::mutex> lck(_connectionsMutex);
		_connections.insert({hdl, {jsonRpc, os}});

	}
	void on_http(websocketpp::connection_hdl hdl) {
		auto con = getConnection(hdl);

		if(!con)
			return;

		if(onHttp.empty()){
			con->set_body("JsonRpcWebsocketServer");
			con->set_status(websocketpp::http::status_code::ok);
		}else{
			con->set_status(*onHttp(con));
		}
	}

	void on_fail(websocketpp::connection_hdl hdl) {
		IERROR("");

		if(auto con = getConnection(hdl)){
			IERROR("Fail handler: " << con->get_ec() << " " << con->get_ec().message());
		}
	}

	void on_close(websocketpp::connection_hdl hdl) {
		IWARN("");

		std::unique_lock<std::mutex> lck(_connectionsMutex);
		_connections.erase(hdl);
	}
	// Define a callback to handle incoming messages
	void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
		IINFO("");
		std::unique_lock<std::mutex> lck(_connectionsMutex);
		try {

			IINFO( "called with hdl: " << hdl.lock().get()
							  << " and opcode: " << msg->get_opcode());

			auto it = _connections.find(hdl);
			if( it != _connections.end() ){

				auto p = msg->get_payload();
				IINFO(log::dump(p));
				((ioSerialMsgReceiver*)it->second.transport.get())->feed(p);
			}else
				IERROR("connection not found in list");

		} catch (const websocketpp::lib::error_code& e) {
			IERROR("Message reception failed because: " << e
					  << "(" << e.message() << ")" );
		}
	}

	virtual void setInstanceName(const std::string &n) override{
		logInstanceName::setInstanceName(n);
	}

private:

	boost::thread* _t;

	server _endpoint;
	boost::thread_group _threadPool;

	typedef struct {
		Flexibity::jsonRpcTransport::sPtr transport;
		outputStream::sPtr os;
	}jsonRpcServerTraits;

	typedef std::map<websocketpp::connection_hdl, jsonRpcServerTraits, std::owner_less<websocketpp::connection_hdl>> con_list;

	con_list _connections;

	std::mutex _connectionsMutex;

	std::map<std::string, jsonRpcBoost::BoostJsonRpcMethod> _methods;

};

}

#include <flexibity/jsonObjMacros.undef>

#endif /* INCLUDE_FLEXIBITY_JSONRPC_JSONRPCSERIAL_HPP_ */
