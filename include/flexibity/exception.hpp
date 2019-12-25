/*
 * exception.hpp
 *
 *  Created on: Jan 25, 2016
 *      Author: rs
 */

#ifndef INCLUDE_FLEXIBITY_EXCEPTION_HPP_
#define INCLUDE_FLEXIBITY_EXCEPTION_HPP_

#include <exception>

namespace Flexibity{

using namespace std;

class exception:
		public std::exception{
public:
	exception(const string& what){
		_what = what;
	}

	virtual const char* what() const noexcept override {
		return _what.c_str();
	}

protected:
	string _what;
};

#define INLINE_CLASS_EXCEPTION() \
	class exception:\
		public Flexibity::exception{ \
	public: \
		exception(const string& what): \
			Flexibity::exception(what){} \
	}

}


#endif /* INCLUDE_FLEXIBITY_EXCEPTION_HPP_ */
