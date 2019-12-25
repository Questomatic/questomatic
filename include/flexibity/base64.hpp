/*
 * base64.hpp
 *
 *  Created on: Jan 15, 2016
 *      Author: rs
 */

#ifndef INCLUDE_FLEXIBITY_BASE64_HPP_
#define INCLUDE_FLEXIBITY_BASE64_HPP_

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include "flexibity/log.h"

namespace Flexibity{
using namespace boost::archive::iterators;
using namespace std;

class base64{
public:

	typedef base64_from_binary<    // convert binary values to base64 characters
					transform_width<string::const_iterator, 6, 8>> // retrieve 6 bit integers from a sequence of 8 bit bytes
			toBase64Iterator; // compose all the above operations in to a new iterator

	typedef transform_width<binary_from_base64<string::const_iterator>, 8, 6> fromBase64Iterator;

	static string encode(string data){
		// Pad with 0 until a multiple of 3
		unsigned int paddedCharacters = 0;
		while(data.size() % 3 != 0){
		  paddedCharacters++;
		  data.push_back(0);
		}
		//GINFO("Padded " << paddedCharacters);
		std::string encodedString(toBase64Iterator(data.begin()), toBase64Iterator(data.end() - paddedCharacters));

		// Add '=' for each padded character used
		while(paddedCharacters > 0){
			encodedString.push_back('=');
			paddedCharacters--;
		}

		//GINFO("encoded to " << encodedString);

		return encodedString;
	}

	static string decode(string data){
		size_t pos;
		while ((pos = data.rfind("=")) != string::npos){
			data.erase(pos);
		}
		return string(fromBase64Iterator(data.begin()), fromBase64Iterator(data.end()));
	}
};

}

#endif /* INCLUDE_FLEXIBITY_BASE64_HPP_ */
