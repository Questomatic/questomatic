#ifndef INCLUDE_FLEXIBITY_DUT_H_
#define INCLUDE_FLEXIBITY_DUT_H_

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <string>
#include <flexibity/log.h>

template<class T>
auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os)
{
	t.print(os);
	return os;
}

namespace Flexibity
{
	const unsigned int DUT_SIGNATURE = 0x024e4302;
	const std::array<unsigned char, 4> DUT_SIGNATURE_C = {0x02, 0x43, 0x4e, 0x02};
	const int MAX_DUT_LENGTH = 4096;

	struct DUTTransportHeader {
		unsigned int source_node;
		unsigned int flags;
		unsigned int dest_node;
		unsigned short dest_if;
		unsigned short source_if;
		unsigned short size;
		unsigned short ttl;
		unsigned short mode;
		unsigned short ttl2;

		void print(std::ostream& os) const {
			os << "DUTTransportHeader" << std::endl;
			os << "\tsource_node: "    << source_node << std::endl;
			os << "\tflags: "          << flags << std::endl;
			os << "\tdest_node: "      << dest_node << std::endl;
			os << "\tdest_if: "        << dest_if << std::endl;
			os << "\tsource_if: "      << source_if << std::endl;
			os << "\tsize: "           << size << std::endl;
			os << "\tttl: "            << ttl << std::endl;
			os << "\tmode: "           << mode << std::endl;
			os << "\tttl2: "           << ttl2 << std::endl;
		}
	};

	struct DUTControlHeader {
		unsigned short version_id;
		unsigned short size;
		unsigned short message_class;
		unsigned short message_type;
		unsigned short message_mode;
		unsigned short reserve1;
		unsigned short reserve2;
		unsigned short data_size;
		unsigned int time_tick;

		void print(std::ostream& os) const {
			os << "DUTControlHeader"  << std::endl;
			os << "\tversion_id: "    << version_id << std::endl;
			os << "\tsize: "          << size << std::endl;
			os << "\tmessage_class: " << message_class << std::endl;
			os << "\tmessage_type: "  << message_type << std::endl;
			os << "\tmessage_mode: "  << message_mode << std::endl;
			os << "\tdata_size: "     << data_size << std::endl;
			os << "\ttime_tick: "     << time_tick << std::endl;
		}
	};

	class DUTPacket {
		unsigned int signature;

	public:
		DUTTransportHeader transport_header;
		DUTControlHeader control_header;
		std::string _data;

		unsigned char received_lrc;
		unsigned char calculated_lrc; // for checking
		unsigned char *_packet_data = nullptr;

		size_t _packet_length;

		bool is_correct() const {
			return received_lrc == calculated_lrc;
		}

		bool is_echo() const {
			return (transport_header.dest_if == 0);
		}

		unsigned char calc_lrc (unsigned char *buf, size_t len) const
		{
			unsigned char bcc = 0;
			for (size_t i = 0; i < len; i++)
				bcc = buf[i] ^ bcc;
			return bcc;
		}

		unsigned char calc_lrc (const std::vector<unsigned char> &buf) const
		{
			unsigned char bcc = 0;
			for (auto i : buf)
				bcc = i ^ bcc;
			return bcc;
		}

	public:
		DUTPacket() = default;
		DUTPacket(const DUTPacket&) = default;

		std::ostream& print(std::ostream& os) const {
			os << log::dump((char *)&_packet_data[0], _packet_length) << std::endl;

			os << "DUTPacket" << std::endl;
			os << transport_header;

			if (is_echo()) {
				os << control_header;
				os << "Message: "<< _data << std::endl;
			}
			else {
				os << "ISO8583 message: "<< _data << std::endl;
			}

			std::string lrc_res = " (UNCORRECT)";
			if (is_correct()) {
				lrc_res = " (CORRECT)";
			}

			os << "LRC: " << std::hex << (unsigned int)received_lrc << std::dec << lrc_res <<std::endl;

			return os;
		}

		std::vector<unsigned char> serialize() {
			std::vector<unsigned char> data;

			data.insert(data.end(), (unsigned char*)&transport_header, (unsigned char*)&transport_header + sizeof(transport_header) );
			if (is_echo()) {
				data.insert(data.end(), (unsigned char*)&control_header, (unsigned char*)&control_header + sizeof(control_header));
			}

			data.insert(data.end(), _data.begin(), _data.end());
			if(is_echo())
				data.push_back('\0');

			auto lrc = calc_lrc(data);

			data.insert(data.begin(), begin(DUT_SIGNATURE_C), end(DUT_SIGNATURE_C));

			data.push_back(lrc);

			return data;
		}

		void parse(unsigned char *data, size_t len) {
			_packet_data = data;
			_packet_length = len;

			signature = *(unsigned int*)&data[0];

			if (signature == DUT_SIGNATURE) {
				transport_header = *(DUTTransportHeader*)(data + sizeof(DUT_SIGNATURE));

				auto data_offset = sizeof(DUT_SIGNATURE) + sizeof(DUTTransportHeader);
				// echo msg incoming
				if (is_echo()) {
					control_header = *(DUTControlHeader*)(data + data_offset);
					data_offset += sizeof(DUTControlHeader);
					_data = std::string(data + data_offset, data + data_offset + control_header.data_size);
				}
				// standard msg
				else {
					_data = std::string(data + data_offset, data + data_offset + transport_header.size);
				}

				received_lrc = *(unsigned char*)&data[len-1];
				calculated_lrc = calc_lrc(&data[sizeof(DUT_SIGNATURE)], len - sizeof(DUT_SIGNATURE) - sizeof(received_lrc));
			}
		}

		DUTPacket generate_echo_response() {
			DUTPacket packet(*this);
			std::swap(packet.transport_header.source_node, packet.transport_header.dest_node);
			packet.transport_header.dest_if = packet.transport_header.source_if;
			packet.transport_header.source_if = 0;
			packet.transport_header.mode &= ~0x0001;
			packet._data = "MediaMarkt_LoylNode=50149";
			packet.control_header.message_type = 1;
			packet.control_header.message_mode = 1;
			packet.control_header.data_size = packet._data.length() + 1;
			packet.transport_header.size = sizeof(DUTControlHeader) + packet.control_header.data_size;

			return packet;
		}

		DUTPacket generate_financial_response(const std::string &iso_message) {
			DUTPacket packet(*this);
			std::swap(packet.transport_header.source_node, packet.transport_header.dest_node);
			std::swap(packet.transport_header.dest_if, packet.transport_header.source_if);
//			packet.transport_header.mode &= ~0x0001;
			packet._data = iso_message;
			packet.transport_header.ttl = 10;
			packet.transport_header.ttl2 = 120;
			packet.transport_header.size = packet._data.length();

			return packet;
		}
	};
}

#endif // INCLUDE_FLEXIBITY_DUT_H_
