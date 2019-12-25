/*
 * serialFeedAggregator.hpp
 *
 *  Created on: Aug 20, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_SERIALFEEDAGGREGATOR_HPP_
#define INCLUDE_FLEXIBITY_SERIALFEEDAGGREGATOR_HPP_

#include "flexibity/ioSerial.hpp"

namespace Flexibity{

/**
 * not thread safe
 */
class ioSerialFeedAggregator:
	public Flexibity::ioSerialMsgReceiver{
public:
	virtual void feed( Flexibity::serialMsgContainer m ) override {
		msgs.push_back(m);
	}

	Flexibity::oSerialMsgContainer aggregate(){
		Flexibity::oSerialMsgContainer m = make_shared<Flexibity::oSerialMsg>();
		while(!msgs.empty()){
			*m += *(msgs.front());
			msgs.pop_front();
		}
		return m;
	}
	void dispatch(Flexibity::ioSerialMsgReceiver &r){
		r.feed(aggregate());
	}
	void dumpMessages(){
		IINFO("dumping \"" << instanceName() << "\" aggregator")
		for (auto i = msgs.begin(); i != msgs.end() ; i++) {
		  IINFO(log::dump(*i));
		}
	}
private:
	deque<Flexibity::serialMsgContainer> msgs;
};

}


#endif /* INCLUDE_FLEXIBITY_SERIALFEEDAGGREGATOR_HPP_ */
