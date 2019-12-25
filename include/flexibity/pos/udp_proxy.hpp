#ifndef INCLUDE_FLEXIBITY_UDP_PROXY_H_
#define INCLUDE_FLEXIBITY_UDP_PROXY_H_

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <flexibity/pos/dut.hpp>
#include <flexibity/ioSerial.hpp>
#include <flexibity/genericMgr.hpp>
#include <flexibity/iso8583/isoProcess.hpp>
#include <jsoncpp/json/json.h>

const std::string ok_json = R"del(
{
   "id" : "0210",
   "value" :
   [
	   {
		   "description" : "Primary Account Number",
		   "id" : 2,
		   "value" : "9802107000112602"
	   },
	   {
		   "description" : "Processing Code",
		   "id" : 3,
		   "value" : "000000"
	   },
	   {
		   "description" : "Transaction Amount",
		   "id" : 4,
		   "value" : "000000000500"
	   },
	   {
		   "description" : "System trace audit number (STAN)",
		   "id" : 11,
		   "value" : "000017"
	   },
	   {
		   "description" : "Local Transaction Time",
		   "id" : 12,
		   "value" : "160525124007"
	   },
	   {
		   "description" : "Point of Service Entry Code",
		   "id" : 22,
		   "value" : "020"
	   },
	   {
		   "description" : "Retrieval References Number",
		   "id" : 37,
		   "value" : "000000097035"
	   },
	   {
		   "description" : "Authorization Identification Response Code",
		   "id" : 38,
		   "value" : "LOYALN"
	   },
	   {
		   "description" : "Response Code",
		   "id" : 39,
		   "value" : "000"
	   },
	   {
		   "description" : "Card Acceptor Terminal Identification",
		   "id" : 41,
		   "value" : "LOYL0358"
	   },
	   {
		   "description" : "Transaction Currency Code",
		   "id" : 49,
		   "value" : "810"
	   },
	   {
		   "description" : "Reserved Private",
		   "id" : 62,
		   "value" : "00470LOYL"
	   }
   ]
})del";

namespace Flexibity {
	namespace ip = boost::asio::ip;
	using namespace boost::asio;

	class udpProxyServer:
			public ioSerialMsgReceiver
	{
	public:
		using sPtr = std::shared_ptr<udpProxyServer>;
		udpProxyServer(io_service& io,
			int localport,
			const std::string& remotehost,
			int remoteport):
			_io(io),
			_input_socket(io, ip::udp::endpoint(ip::udp::v4(), localport)),
			_output_socket(io),
			_remotehost(remotehost),
			_remoteport(remoteport),
			_output_endpoint(ip::address_v4::from_string(_remotehost), _remoteport)
		{
			ILOG_INIT();
			start();
		}

		udpProxyServer(io_service& io,
			const Json::Value& cfg):
			_io(io),
			_input_socket(io, ip::udp::endpoint(ip::udp::v4(), cfg["localPort"].asInt())),
			_output_socket(io),
			_remotehost(cfg["host"].asString()),
			_remoteport(cfg["hostPort"].asInt()),
			_output_endpoint(ip::address_v4::from_string(_remotehost), _remoteport)
		{

			ILOG_INIT();
			setInstanceName(cfg["id"].asString());
			start();
		}

		virtual void feed(serialMsgContainer msg) override{
			_io.post(boost::bind(&udpProxyServer::feedOp, this, msg));
		}

		void start() {
			IINFO("start");
			_input_socket.async_receive_from(
				boost::asio::buffer(data_, MAX_DUT_LENGTH),
				_input_endpoint,
				boost::bind(&udpProxyServer::handle_input_receive,
					this,
					boost::asio::placeholders::bytes_transferred,
					boost::asio::placeholders::error));
		}

		void setOutput(std::shared_ptr<ioSerialMsgReceiver> ptr) {
			_output_ptr = ptr;
			if(auto p = _output_ptr.lock()) {
				IINFO("Switched to: " << p->instanceName())
			}
		}

		void toggleSelfResponse() {
			_self_response = !_self_response;
			IINFO("Switched to Self Response: " << std::boolalpha << _self_response << std::noboolalpha);
		}

	private:
		void handle_input_receive(size_t bytes_transferred, const boost::system::error_code& error) {
			if (!error) {
				DUTPacket packet;
				packet.parse(&data_[0], bytes_transferred);

				IINFO("Using Self Response: " << std::boolalpha << _self_response << std::noboolalpha);
				IINFO("Received: " << bytes_transferred << " bytes from " << _input_endpoint.address());
//				IINFO("Received: " << packet);

				serialMsgContainer oData = serialMsgContainer(new serialMsg((const char*)data_, bytes_transferred));
				IINFO(log::dump(oData));
				if (!packet.is_echo()) {
					if(auto p = _output_ptr.lock()) {
						IsoProcessor processor;
						IsoMessage iso_message = processor.extract(packet._data);
						IWARN(iso_message.to_json());

						if(_self_response) {
							if (iso_message.msg_type == "0200") {

								IsoMessage msg = IsoMessage::from_json(ok_json);
								msg.set(4, iso_message.get(4));
								msg.set(12, IsoMessage::iso_time());
								msg.set(11, iso_message.get(11));

								auto fin_packet = packet.generate_financial_response(processor.pack(msg));
								auto fin_packet_d = fin_packet.serialize();
								serialMsgContainer nData = serialMsgContainer(new serialMsg((char*)fin_packet_d.data(), fin_packet_d.size()));
								feedBack(nData);
							}
//							else if (iso_message.msg_type == "0202") {
//								static char state_0202 = 0;
//								IsoMessage msg("0212");

//								msg.set(3, iso_message.get(3));
//								msg.set(11, iso_message.get(11));
//								msg.set(12, IsoMessage::iso_time());

//								if (state_0202 == 0) {
//									msg.set(37, std::string("000000097035"));
//									state_0202 = 1;
//								} else {
//									state_0202 = 0;
//								}

//								msg.set(39, 6);
//								msg.set(41, std::string("LOYL0358"));
//								IINFO("Send: " << msg.to_json());
//								auto fin_packet = packet.generate_financial_response(processor.pack(msg));
//								auto fin_packet_d = fin_packet.serialize();
//								serialMsgContainer nData = serialMsgContainer(new serialMsg((char*)fin_packet_d.data(), fin_packet_d.size()));
//								feedBack(nData);
//							}
//							else {
//								p->feed(oData);
//							}
						}
						else {
							auto json = iso_message.to_json();
							auto fin_packet = packet.generate_financial_response(processor.pack(IsoMessage::from_json(json)));
							auto fin_packet_d = fin_packet.serialize();
							serialMsgContainer nData = serialMsgContainer(new serialMsg((char*)fin_packet_d.data(), fin_packet_d.size()));
							p->feed(nData);
						}
					}
				} else {
					auto echo_packet = packet.generate_echo_response();
					auto echo_packet_d = echo_packet.serialize();
					serialMsgContainer nData = serialMsgContainer(new serialMsg((char*)echo_packet_d.data(), echo_packet_d.size()));
					feedOp(nData);
				}

				start();
			}
		}

		void handle_output_send(size_t bytes_send, const boost::system::error_code& error) {
			if (error) {
				IERROR("Error sending-out data");
				return;
			}

			IINFO("data sent (" << bytes_send << ")");
		}


		void feedOp(serialMsgContainer data){
			IINFO("Send to: " << _output_endpoint.address() << "\n" << log::dump(data));
			_input_socket.async_send_to(
				boost::asio::buffer(*data, data->size()),
				_output_endpoint,
				boost::bind(&udpProxyServer::handle_output_send,
				this,
				boost::asio::placeholders::bytes_transferred,
				boost::asio::placeholders::error));
		}

		void feedBack(serialMsgContainer data){
//			IINFO(log::dump(data));
			_input_socket.async_send_to(
				boost::asio::buffer(*data, data->size()),
				_input_endpoint,
				boost::bind(&udpProxyServer::handle_output_send,
				this,
				boost::asio::placeholders::bytes_transferred,
				boost::asio::placeholders::error));
		}

		std::weak_ptr<ioSerialMsgReceiver> _output_ptr;

		io_service& _io;

		ip::udp::socket _input_socket;
		ip::udp::socket _output_socket;
		std::string _remotehost;
		int _remoteport;
		ip::udp::endpoint _input_endpoint;
		ip::udp::endpoint _output_endpoint;
		bool _self_response = false;
		unsigned char data_[MAX_DUT_LENGTH];
	};

	class pinpadMgr:
			public genericMgr<udpProxyServer::sPtr>{
	public:

		pinpadMgr(const Json::Value& cfg) {
			ILOG_INITSev(INFO);

			populateItems(cfg, [&](const Json::Value& iCfg) {
				return udpProxyServer::sPtr(new udpProxyServer(_ios, iCfg));
			});
		}

		void run() {
			_thread = boost::thread((boost::bind(&pinpadMgr::loop, this)));
		}
		boost::asio::io_service _ios;
		boost::thread _thread;

		void loop() {
			ITRACE("enter");
			while (!_ios.stopped()) {
				size_t ev = _ios.run_one(); // processes the tasks
				IDEBUG("processed " << ev << " event(s)");
			}
			IINFO("exit");
		}

		virtual ~pinpadMgr() {
			_ios.stop();
			if (_thread.joinable()) {
				_thread.join();
			}
			ITRACE("");
		}
	};

}

#endif // INCLUDE_FLEXIBITY_UDP_PROXY_H_
