#ifndef INCLUDE_FLEXIBITY_DEBUGSERVER_HPP_
#define INCLUDE_FLEXIBITY_DEBUGSERVER_HPP_

#include <jsoncpp/json/json.h>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace Flexibity {

	using boost::asio::ip::tcp;

	class tcp_connection: public boost::enable_shared_from_this<tcp_connection>
	{
	public:
		typedef boost::shared_ptr<tcp_connection> pointer;

		static pointer create(boost::asio::io_service& io_service,  unsigned char* buf, unsigned int bufsize)
		{
			return pointer(new tcp_connection(io_service, buf, bufsize));
		}

		tcp::socket& socket()
		{
			return socket_;
		}

		void start()
		{
			// boost::asio::mutable_buffer mbuf(_send_buf, _send_bufsize);
		boost::asio::async_write(socket_, boost::asio::buffer(_send_buf, _send_bufsize),
			boost::bind(&tcp_connection::handle_write, shared_from_this(),
			  boost::asio::placeholders::error,
			  boost::asio::placeholders::bytes_transferred));
		}

	private:
		tcp_connection(boost::asio::io_service& io_service,  unsigned char* buf, unsigned int bufsize): socket_(io_service), _send_buf(buf), _send_bufsize(bufsize)
		{
		}

		void handle_write(const boost::system::error_code&  error, size_t bt)
		{
			// std::cout << "Transfer error" << error.message() << std::endl;
			if (!error || error == boost::asio::error::message_size)
			{
				start();
			}
		}

		tcp::socket socket_;
		std::string message_;

		unsigned char *_send_buf;
		unsigned int _send_bufsize;
	};

	class tcp_server
	{
	public:
		tcp_server(boost::asio::io_service& io_service,  unsigned int port, unsigned char *buf, unsigned int bufsize )
		: acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), _send_buf(buf), _send_bufsize(bufsize)
		{
			start_accept();
		}

		boost::asio::io_service& io_service() {
			return acceptor_.get_io_service();
		}

	private:
		void start_accept()
		{
			tcp_connection::pointer new_connection = tcp_connection::create(acceptor_.get_io_service(), _send_buf, _send_bufsize);

			acceptor_.async_accept(new_connection->socket(),
				boost::bind(&tcp_server::handle_accept, this, new_connection,
				  boost::asio::placeholders::error));
		}

		void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error)
		{
			if (!error)
			{
				new_connection->start();
			}

			start_accept();
		}

		tcp::acceptor acceptor_;
		unsigned char *_send_buf;
		unsigned int _send_bufsize;
	};



	class debugServer : public logInstanceName
	{
	public:
		debugServer(const Json::Value& cfg, unsigned char * buffer, unsigned int bufsize){
			ILOG_INITSev(INFO);
			_enabled = cfg.get("enabled", false).asBool();
			_port = cfg.get("port", 8099).asUInt();

			_io_service = new boost::asio::io_service;
			server = new tcp_server(*_io_service, _port, buffer, bufsize);
		}
		~debugServer() {
			delete server;
			delete _io_service;
		}

		void poll() {
			if(_enabled)
				_io_service->poll();
		}

	private:
		bool _enabled;
		unsigned int _port;
		boost::asio::io_service *_io_service = nullptr;
		tcp_server *server = nullptr;
	};
}

#endif // INCLUDE_FLEXIBITY_DEBUGSRV_HPP_
