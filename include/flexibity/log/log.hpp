/*
 * log.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_LOG_LOG_HPP_
#define INCLUDE_FLEXIBITY_LOG_LOG_HPP_


#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <string>
#include <memory>
#include <list>
#include <syslog.h>
#include <boost/thread.hpp>

#include <jsoncpp/json/json.h>

#include "flexibity/log/logConfig.h"
#include "flexibity/log.h"

namespace Flexibity{

log::log(const std::string& name, severity sev):
	_prettyName(name),
	_name(name),
	_sev(sev){
	truncName();

	//ILOG_INITSev(TRACE);
	//TODO: automatically add logger to registry?
}
log::~log(){
	//TODO: automatically remove logger from registry?
	//Flexibity::logConfig::get()->deleteLogger(std::shared_ptr<Flexibity::log>(this));
}

void log::truncName(){
	erase(_name.rfind("("));
	erase(_name.rfind("::"));
	erase(_name.rfind("<"));
}

void log::setName(const std::string& n){
	_name = n;
	truncName();
	logConfig::get()->updateLogger(shared_from_this());
}

/*static*/ std::shared_ptr<log> log::global(){
	static std::shared_ptr<log> instance = NULL;

	if(!instance){
		instance = std::make_shared<log>(log("GLOBAL"));

		Flexibity::logConfig::get()->addLogger(instance);
	}

	return instance;
}

/*static*/ int log::wildcmp(const char *wild, const char *string) {
  // Written by Jack Handy - <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
  const char *cp = NULL, *mp = NULL;

  while ((*string) && (*wild != '*')) {
    if ((*wild != *string) && (*wild != '?')) {
      return 0;
    }
    wild++;
    string++;
  }

  while (*string) {
    if (*wild == '*') {
      if (!*++wild) {
        return 1;
      }
      mp = wild;
      cp = string+1;
    } else if ((*wild == *string) || (*wild == '?')) {
      wild++;
      string++;
    } else {
      wild = mp;
      string = cp++;
    }
  }

  while (*wild == '*') {
    wild++;
  }
  return !*wild;
}

log::match log::nameMatch(const std::string& namePattern){
	if(name() == namePattern)
		return FULL;
	if(wildcmp(namePattern.c_str(), name().c_str()))
		return WILD;
	else
		return NONE;
}

const std::string log::sevStr()const{
	return Flexibity::log::sevToStr(_sev);
}

const std::string log::name()const {
	if(_instanceName.size())
		return _name + "|" + _instanceName + "|";
	else
		return _name;
};
bool log::canLog(severity sev) const{
	return sev <= _sev;
}
void log::setSev(severity sev){
	GDEBUG(name() << " set to " << sevToStr(sev));
	_sev = sev;
}
void log::setInstanceName(const std::string& in){
	_instanceName = in;
	logConfig::get()->updateLogger(shared_from_this());
}
const std::string log::f(const std::string& f)const{
	return f;
}
const std::string log::ln(int ln)const{
	return "[L:" + std::to_string(ln) + "]";
}
/*static*/ const std::string log::sevToStrColor(severity sev){
	string out = "\x1B";
	if(sev <= ERROR)
		out += "[31m";
	else if(sev == WARNING)
		out += "[33m";
	else if(sev == INFO)
		out += "[32m";
	else if(sev == DEBUG)
		out += "[37m";
	else
		out += "[39m";
	return out;
}

/*static*/ const std::string log::sevToStr(severity sev){
	switch(sev){
	case FATAL:   return "F";
	case ERROR:   return "E";
	case WARNING: return "W";
	case INFO:    return "I";
	case EVENT:   return "V";
	case DEBUG:   return "D";
	case TRACE:   return "T";
	default: 	  return "?";
	}
}

/*static*/ bool log::strToSev(const std::string& sevStr, log::severity &sev){
	for(int i = log::FATAL; i <= log::COUNT; i++){
		sev = (log::severity) i;
		if(sevStr.find(sevToStr((sev))) != std::string::npos){
			return true;
		}
	}

	GERROR("cannot convert " << sevStr << " to severity level" );
	sev = TRACE;
	return false;
}

void log::erase(size_t pos){
	if(pos != std::string::npos)
		_name.erase(pos);
}

/*
 Address                   Hexadecimal values                  Printable
 --------------  -----------------------------------------------  ----------------

 0 00000000  4d 4d 53 02 00 00 02 00 0a 00 00 00 03 00 00 00  MMS.............
 16 00000010  3d 3d 68 01 00 00 3e 3d 0a 00 00 00 03 00 00 00  ==h...>=........
 32 00000020  00 01 0a 00 00 00 00 00 80 3f 00 40 4e 01 00 00  .........?.@N...
 48 00000030  42 6f 78 30 31 00 00 41 42 01 00 00 10 41 68 00  Box01..AB....Ah.
 64 00000040  00 00 08 00 26 76 0a c2 1f 0c a8 c1 00 00 00 00  ....&v..........
 ...

 Reference:

 Howard Burdick,
 Digital Imaging, Theory and Applications,
 McGraw Hill, 1997, page 219.

 Modified:

 09 August 2001
*/


const std::string log::dump(const char *pcBuf, size_t iLen) {
	long int addr;
	unsigned char *b;

	const int base = 16;
	const int int_addr_len = 8;
	const int hex_addr_len = 8;

	unsigned char buf[base + 4];
	long int cnt;
	long int cnt2;
	long m;
	long n;
	size_t offs = 0;
	size_t reqLen = iLen;

	std::ostringstream o;
	std::ostringstream trail;
	//std::shared_ptr<std::ostringstream> o;// = std::make_shared<std::ostringstream>(os);

	/*if (iLen == 0) {
		o << std::endl << "Nothing to dump!" << std::endl;
		return o.str();
	}*/
	/*
	 Dump the file contents.
	 */
	o << std::endl << "Dumping " << iLen << " bytes" << std::endl;
	o << std::left << "     Address                 Hexadecimal values                    Printable" << std::endl;
	trail << std::setw(int_addr_len) << std::setfill('-') << "-" << " " << std::setw(hex_addr_len) << "-" << " " << std::setw(base * 3 - 1) << "-" << "  " << std::setw(base) << "-" << std::endl;
	o << trail.str();
	//o << std::endl;

	addr = 0;

	while (iLen > 0) {

		if (iLen / base)
			cnt = base;
		else
			cnt = iLen % base;

		memcpy(buf, pcBuf, cnt);
		pcBuf += cnt;

		offs += cnt;
		iLen -= cnt;

		b = buf;
		/*
		 Print the address in decimal and hexadecimal.
		 */
		o << std::right << std::setw(int_addr_len) << std::setfill(' ') << std::dec << (int) addr << " " << std::setw(hex_addr_len)
				<< std::hex << std::setfill('0') << addr << " ";
		addr = addr + base;
		/*
		 Print 16 data items, in pairs, in hexadecimal.
		 */
		cnt2 = 0;
		for (m = 0; m < base; m++) {
			cnt2 = cnt2 + 1;
			if (cnt2 <= cnt) {
				o << std::setw(2) << std::hex << (int)*b++;
			} else {
				o << "  ";
			}
			o << " ";
		}
		/*
		 Print the printable characters, or a period if unprintable.
		 */
		o << " ";
		cnt2 = 0;
		for (n = 0; n < base; n++) {
			cnt2 = cnt2 + 1;
			if (cnt2 <= cnt) {
				if ((buf[n] < 32) || (buf[n] > 126)) {
					o << ".";
				} else {
					o << buf[n];
				}
			}
		}
		o << std::endl;
	}

	o << trail.str();
	o << "Dumped " << std::dec << offs << " of " << reqLen << " bytes" << std::endl;

	return o.str();
}

//ILOG_IMPL_STATIC(Flexibity::log);

}

#include "flexibity/log/logConfig.hpp"

#endif /* INCLUDE_FLEXIBITY_LOG_LOG_HPP_ */
