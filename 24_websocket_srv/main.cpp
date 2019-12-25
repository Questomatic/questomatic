/*
 * Copyright (c) 2014, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** ====== WARNING ========
 * This example is presently used as a scratch space. It may or may not be broken
 * at any given time.
 */
#include <boost/asio.hpp>
#include <websocketpp/config/debug_asio_no_tls.hpp>

#include <websocketpp/server.hpp>
#include <json/json.h>
#include <set>

#include "flexibity/programOptions.hpp"

typedef websocketpp::server<websocketpp::config::debug_asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

class WebsocketServer {
public:
	WebsocketServer(uint16_t port) {
		_endpoint.clear_access_channels(websocketpp::log::alevel::all);
		_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
		_endpoint.set_access_channels(websocketpp::log::alevel::app);
		_endpoint.init_asio();
		_endpoint.set_reuse_addr(true);

		// Register our message handler
		_endpoint.set_message_handler(bind(&WebsocketServer::on_message,this,::_1,::_2));

		_endpoint.set_open_handler(bind(&WebsocketServer::on_open, this, ::_1));
		_endpoint.set_http_handler(bind(&WebsocketServer::on_http, this, ::_1));
		_endpoint.set_fail_handler(bind(&WebsocketServer::on_fail, this, ::_1));
		_endpoint.set_close_handler(bind(&WebsocketServer::on_close, this, ::_1));

		_endpoint.listen(websocketpp::lib::asio::ip::tcp::v4(), port);

		// Start the server accept loop
		_endpoint.start_accept();
		ILOG_INIT();
	}

	void on_open(websocketpp::connection_hdl hdl) {
		IINFO("called with hdl: " << hdl.lock().get());
		_connections.insert(hdl);

	}
	void on_http(websocketpp::connection_hdl hdl) {
		server::connection_ptr con = _endpoint.get_con_from_hdl(hdl);

		std::string res = con->get_request_body();

		std::stringstream ss;
		IWARN( "got HTTP request with " << res.size() << " bytes of body data.");

		con->set_body(ss.str());
		con->set_status(websocketpp::http::status_code::ok);
	}

	void on_fail(websocketpp::connection_hdl hdl) {
		server::connection_ptr con = _endpoint.get_con_from_hdl(hdl);

		IERROR("Fail handler: " << con->get_ec() << " " << con->get_ec().message());
	}

	void on_close(websocketpp::connection_hdl hdl) {
		IWARN("Close handler");
		_connections.erase(hdl);
	}
	// Define a callback to handle incoming messages
	void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
		IINFO( "called with hdl: " << hdl.lock().get()
				  << " and opcode: " << msg->get_opcode());

		try {
			std::string response = parse_and_response(msg->get_payload());
			_endpoint.send(hdl, response, websocketpp::frame::opcode::text);
		} catch (const websocketpp::lib::error_code& e) {
			IERROR("Echo failed because: " << e
					  << "(" << e.message() << ")" );
		}
	}

	void send_message(const std::string &msg) {
		for (auto it : _connections) {
			_endpoint.send(it,msg, websocketpp::frame::opcode::text);
		}
	}

	server& get_endpoint() {
		return _endpoint;
	}

	void run(){
		IINFO("enter");
		_endpoint.run();
		IINFO("exit");
	}

private:
	std::string parse_and_response(const std::string &msg) {
		Json::Value root;
		Json::Value response;
		Json::Value result;
		Json::Reader reader;

		if(!reader.parse(msg, root, true)) {
			IERROR("Invalid json");
		} else {
			result["code"] = 0;
			result["data"] = Json::Value::null;
			result["message"] = "ok";
			response["result"] = result;
			response["id"] = root["id"];
			response["jsonrpc"] = "2.0";
			IINFO(std::endl << root.toStyledString());
		}

		return Json::FastWriter().write(response);
	}

	server _endpoint;
	typedef std::set<websocketpp::connection_hdl,std::owner_less<websocketpp::connection_hdl>> con_list;

	con_list _connections;
	ILOG_DEF();
};

using namespace boost::asio;

class TerminalReader
{
public:
	TerminalReader(WebsocketServer * websocket): _websocket_service(websocket), _input(), _input_stream(_input) {
		_input_stream.assign(STDIN_FILENO);
		_input_stream.async_read_some(boost::asio::buffer(_input_data, sizeof(_input_data)), bind(&TerminalReader::handle_read_input, this, ::_1,::_2));
		ILOG_INIT();

	}
	void run(){
		IINFO("enter");
		_input.run();
		IINFO("exit");
	}

	void handle_read_input(const boost::system::error_code& error, std::size_t length) {
		if (!error)	{
			if(length > 0) {
				IINFO(std::endl << std::string(_input_data));
				_websocket_service->get_endpoint().get_io_service().post(bind(&WebsocketServer::send_message, _websocket_service, std::string(_input_data)));
				memset(_input_data, 0, sizeof(_input_data));
			}

		}
		_input_stream.async_read_some(boost::asio::buffer(_input_data, sizeof(_input_data)), bind(&TerminalReader::handle_read_input, this, ::_1,::_2));
	}

private:
	WebsocketServer* _websocket_service;
	io_service _input;
	posix::stream_descriptor _input_stream;
	char _input_data[4096];
	ILOG_DEF();
};

int main(int argc, char **argv ) {
	uint16_t port = 9012;
	Flexibity::programOptions options;

	options.desc.add_options()
			("port,p",  Flexibity::po::value<uint16_t>(&port), "Define the WebSocket port to bind tot.");

	options.parse(argc, argv);

	WebsocketServer server(port);
	TerminalReader reader(&server);


	GINFO("Starting websocket server on " << port << " port");
	std::thread srvThread([&](){ server.run(); });

	GINFO("reader start");
	reader.run();
	GINFO("waiting srv join");
	srvThread.join();
	GINFO("terminating");
}
