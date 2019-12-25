/*
 * util.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_UTIL_HPP_
#define INCLUDE_FLEXIBITY_UTIL_HPP_

#include "flexibity/log.h"

#define XSTR(s) STR(s)
#define STR(s) #s

namespace Flexibity{

template<typename T>
void observeWeakPtr(std::weak_ptr<T> weak)
{
	if (auto observe = weak.lock()) {
		GDEBUG("\tobserve() able to lock weak_ptr<>, value=" << /**observe <<*/ "\n");
	} else {
		GDEBUG("\tobserve() unable to lock weak_ptr<>\n");
	}
}

template <typename T>
struct objDeleter {    // a verbose obj deleter:
	objDeleter(){ILOG_INITSev(INFO);}
	void operator()(T* p) {
		IDEBUG("[deleter called]");
		delete p;
	}
	ILOG_DEF();
};

//ILOG_IMPL_STATIC(Flexibity::objDeleter<const char>);


struct strDeleter {    // a verbose obj deleter:
	strDeleter(){ILOG_INITSev(INFO);}
	void operator()(const std::string* p) {
		IDEBUG(typeid(*p).name() << "[deleter called]. Content was: " << std::endl << *p);
		delete p;
	}
	ILOG_DEF_STATIC();
};

ILOG_IMPL_STATIC(Flexibity::strDeleter);

struct jsonDeleter {    // a verbose obj deleter:
	jsonDeleter(){ILOG_INITSev(INFO);}
	void operator()(const Json::Value* p) {
		IDEBUG(typeid(*p).name() << "[deleter called]. Content was: " << std::endl << p->toStyledString());
		delete p;
	}
	ILOG_DEF_STATIC();
};

ILOG_IMPL_STATIC(Flexibity::jsonDeleter);

}


#endif /* INCLUDE_FLEXIBITY_UTIL_HPP_ */
