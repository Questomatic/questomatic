/*
 * mac.hpp
 *
 *  Created on: Jan 14, 2016
 *      Author: rs
 */

#ifndef INCLUDE_FLEXIBITY_MAC_HPP_
#define INCLUDE_FLEXIBITY_MAC_HPP_

#include "flexibity/log.h"

namespace Flexibity {

	class macAddress{
	public:
		static string get(){
			static string mac;

			if(0 == mac.size()){
				std::string macFN = "/sys/class/net/eth0/address";

				std::ifstream ifs(macFN);
				if (ifs.is_open()) {
					ifs >> mac;
					GINFO("Read " << mac << " from " << macFN);
				}
			}

			return mac;
		}

		static Json::Value getValue() {
			Json::Value value(Json::objectValue);
			value["mac"] = macAddress::get();
			return value;
		}
	};

}

#endif /* INCLUDE_FLEXIBITY_MAC_HPP_ */
