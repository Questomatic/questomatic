#ifndef INCLUDE_FLEXIBITY_ISOPROCESS_HPP_
#define INCLUDE_FLEXIBITY_ISOPROCESS_HPP_

#include <map>
#include <string>
#include <limits>
#include <iostream>
#include <bitset>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <type_traits>
#include <jsoncpp/json/json.h>

#include "isoField.hpp"

template< typename T >
std::string int_to_hex( T i )
{
  std::stringstream stream;
  stream << std::setfill ('0') << std::setw(sizeof(T)*2)
		 << std::hex << i;
  return stream.str();
}
template<typename T>
struct is_string : public std::integral_constant<bool, std::is_same<std::string, typename std::decay<T>::type>::value> {};

enum IsoSizePolicy_e : unsigned long {
	SIZE_LLVAR = std::numeric_limits<unsigned long>::max()-1,
	SIZE_LLLVAR = std::numeric_limits<unsigned long>::max()-2
};

using IsoConfig = std::map<size_t, IsoField>;
using IsoMessageMap = std::map<size_t, IsoField*>;

static IsoConfig g_isoConfig = {
	{0, {4,"Operation Header" , FieldFormat_e::FIELD_N} },
	{1, {16, "Secondary Bitmap", FieldFormat_e::FIELD_BITMAP} },
	{2, {SIZE_LLVAR, "Primary Account Number", FieldFormat_e::FIELD_N} },
	{3, {6, "Processing Code", FieldFormat_e::FIELD_N} },
	{4, {12, "Transaction Amount", FieldFormat_e::FIELD_N} },
	{11, {6, "System trace audit number (STAN)", FieldFormat_e::FIELD_N} },
	{12, {12, "Local Transaction Time", FieldFormat_e::FIELD_N} },
	{14, {4, "Expiration Date", FieldFormat_e::FIELD_N} },
	{22, {3, "Point of Service Entry Code", FieldFormat_e::FIELD_N} },
	{25, {4, "Point of Service Condition Code", FieldFormat_e::FIELD_N} },
	{30, {24, "Original amount", FieldFormat_e::FIELD_N} },
	{35, {SIZE_LLVAR, "Track 2 Data", FieldFormat_e::FIELD_Z} },
	{37, {12, "Retrieval References Number", FieldFormat_e::FIELD_ANP} },
	{38, {6, "Authorization Identification Response Code", FieldFormat_e::FIELD_ANP} },
	{39, {3, "Response Code", FieldFormat_e::FIELD_N} },
	{41, {8, "Card Acceptor Terminal Identification", FieldFormat_e::FIELD_ANS} },
	{42, {15, "Card Acceptor Identification", FieldFormat_e::FIELD_AN} },
	{44, {SIZE_LLLVAR, "Additional Response Data", FieldFormat_e::FIELD_ANS} },
	{49, {3, "Transaction Currency Code", FieldFormat_e::FIELD_AN} },
	{55, {SIZE_LLLVAR, "Reserved Private", FieldFormat_e::FIELD_B}},
	{62, {SIZE_LLLVAR, "Reserved Private", FieldFormat_e::FIELD_ANS} },
	{63, {SIZE_LLLVAR, "Reserved Private", FieldFormat_e::FIELD_ANS} }
};

class IsoMessage {
	std::string expand(const IsoField& field, const std::string& value) {
		std::stringstream stream;
		if (field.lengthType == SIZE_LLVAR) {
			stream << std::setfill ('0') << std::setw(2) << value.length();
		} else if (field.lengthType == SIZE_LLLVAR) {
			stream << std::setfill ('0') << std::setw(3) << value.length();
		}
		stream << value;
		return stream.str();
	}

	std::string expand(const IsoField& field, const int value) {
		std::stringstream stream;

		stream << std::setfill ('0') << std::setw(field.lengthType) << value;

		return stream.str();
	}

public:
	std::string protocol_version;
	std::string msg_type;
	IsoMessageMap message;

	IsoMessage(const std::string &msg_type, const std::string &protocol_version = "023"):msg_type(msg_type), protocol_version(protocol_version) {}
	IsoMessage(const IsoMessage&) = default;
	~IsoMessage(){
		for(auto &i : message) {
			delete i.second;
		}
		message.clear();
	}

	template<typename T>
	void set(size_t bit, const T &value) {
		static_assert(  is_string<decltype(value)>::value || std::is_integral<T>::value, "value must be std::string or int");

		IsoField *fd = nullptr;
		auto it = message.find(bit);
		if(it != message.end()) {
			fd = it->second;
			fd->value = expand(*fd, value);
		}
		else {
			fd = new IsoField(g_isoConfig.at(bit));
			fd->value = expand(*fd, value);
			message.emplace(bit, fd);
		}
	}

	std::string get(size_t bit)
	{
		return message.at(bit)->value;
	}

	Json::Value to_json() {
		Json::Value root;
		Json::Value val;

		for(auto &f : message)
		{
			Json::Value field;
			field["id"] = (Json::UInt)f.first;
			field["description"] = f.second->description;
			field["value"] = f.second->value;
			val.append(field);
		}

		root["id"] = msg_type;
		root["value"] = val;
		return root;
	}

	static IsoMessage from_json(const std::string &json) {
		Json::Value root;
		Json::Reader reader;

		reader.parse(json, root);

		return IsoMessage::from_json(root);
	}

	static IsoMessage from_json(const Json::Value &json) {
		std::string root_key = json["id"].asString();

		IsoMessage msg(root_key);
		if(json["value"].isArray()) {
			for(auto it : json["value"]) {
				if(it.isObject()) {
					msg.set(it["id"].asUInt(), it["value"].asString());
				}
			}
		}
		return msg;
	}

	static std::string iso_time() {
		std::time_t t = std::time(NULL);
		std::stringstream ss;
//		ss << std::put_time(std::localtime(&t), "%y%m%d%H%M%S");
		char mbstr[12];
		std::strftime(mbstr, sizeof(mbstr),  "%y%m%d%H%M%S", std::localtime(&t));
		ss << mbstr;
		return ss.str();
	}
};

class IsoProcessor
{
	IsoField* extractField(size_t bit, std::string &msg_str) {
		IsoField *field = new IsoField;

		unsigned long length = 0;
		unsigned long lengthType = g_isoConfig.at(bit).lengthType;

		if (lengthType == SIZE_LLVAR)
		{
			length = std::stoul(msg_str.substr(0,2));
			msg_str = msg_str.substr(2);
		}
		else if (lengthType == SIZE_LLLVAR)
		{
			length = std::stoul(msg_str.substr(0,3));
			msg_str = msg_str.substr(3);
		}
		else
			length = lengthType;


		field->lengthType = length;
		field->value = msg_str.substr(0,length);
		msg_str = msg_str.substr(length);
		return field;
	}

public:

	IsoMessage extract(const std::string& message) {
		headerLength = g_isoConfig.at(0).lengthType;

		std::string protocolVersion = message.substr(0,3);
		msgType = message.substr(3,4);
		IsoMessage msg(msgType,protocolVersion);
		bitmapS =  message.substr(3+headerLength,16);
		bitmap = std::stoull(bitmapS, 0, 16);

		std::bitset<sizeof(bitmap)*8> bitmapset(bitmap);

		std::string msg_str = message.substr(3+headerLength + bitmapS.length());

		for (size_t i = bitmapset.size(); i > 0; i--)
		{
			if (bitmapset[i]) {
				size_t bit = bitmapset.size() - i; // convert bit number order
				msg.set(bit, extractField(bit, msg_str)->value);
			}
		}
		return msg;
	}

	std::string pack(const IsoMessage &msg) {
		std::stringstream message;
		std::bitset<sizeof(unsigned long long)*8> bitmap;

		for(auto f: msg.message){
			size_t bit = 0;
			if (f.first > 0)
				bit = bitmap.size() - f.first;
			bitmap.set(bit);
		}

		std::string bmp = int_to_hex(bitmap.to_ullong());

		std::transform(bmp.begin(), bmp.end(), bmp.begin(), ::toupper);

		message << msg.protocol_version << msg.msg_type << bmp;
		for(auto f : msg.message) {
			message << f.second->value;
		}

		return message.str();
	}

	std::string msgType;
	std::string bitmapS;
	unsigned long long bitmap;
	unsigned long headerLength;

	static IsoConfig isoconfig;
};

#endif // ISOEXTRACT_H
