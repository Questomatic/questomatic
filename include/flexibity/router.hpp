/*
 * router.hpp
 *
 *  Created on: Aug 12, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_ROUTER_HPP_
#define INCLUDE_FLEXIBITY_ROUTER_HPP_

#include "flexibity/log.h"
#include "flexibity/ioSerial.hpp"
#include <map>

namespace Flexibity{

typedef Flexibity::ioSerial::serialMsg serialMsg;

class terminal{
	//Flexibity::ioSerial::msgT
	virtual void dispatch(serialMsg);
	virtual ~terminal();
};

class router{
	//typedef std::map<std::string, terminal> terminal

	bool dispatch(serialMsg);

	std::map<std::string, terminal> _terminals;
};

}


#endif /* INCLUDE_FLEXIBITY_ROUTER_HPP_ */
