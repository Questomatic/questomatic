/*
 * lock.hpp
 *
 *  Created on: Jan 21, 2016
 *      Author: rs
 */

#ifndef INCLUDE_FLEXIBITY_LOCK_HPP_
#define INCLUDE_FLEXIBITY_LOCK_HPP_

#include <mutex>

namespace Flexibity {

	typedef std::lock_guard<std::mutex> lock;
	typedef std::mutex lockM;

}


#endif /* INCLUDE_FLEXIBITY_LOCK_HPP_ */
