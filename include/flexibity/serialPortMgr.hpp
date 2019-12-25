/*
 * serialPortMgr.hpp
 *
 *  Created on: Aug 13, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_SERIALPORTMGR_HPP_
#define INCLUDE_FLEXIBITY_SERIALPORTMGR_HPP_

#include "flexibity/log.h"
#include "flexibity/ioSerial.hpp"
#include <jsoncpp/json/json.h>
#include <map>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <flexibity/genericMgr.hpp>

namespace Flexibity{
class serialPortMgr:
	public genericMgr<ioSerial::sPtr> {
public:
	typedef shared_ptr<serialPortMgr> sPtr;
	static constexpr const char* threadPoolOption = "threadPool";

	serialPortMgr(const Json::Value &cfg):
		_serial_io_service(),
		_threadPool(){
		ILOG_INITSev(INFO);
		setInstanceName(name());
		populateItems(cfg, [&](const Json::Value& iCfg){
			return make_shared<ioSerial>(_serial_io_service, iCfg);
		});

		size_t _size = 1;
		size_t tp = cfg[threadPoolOption].asUInt();
		if(tp != 0)
			_size = tp;
		for ( size_t i = 0; i < _size; ++i){
			_threadPool.create_thread(boost::bind(&boost::asio::io_service::run, &_serial_io_service));
		}
		IINFO(_threadPool.size() << " serial worker thread(s) created");
	}

	virtual ~serialPortMgr(){
		IINFO("waiting for " << _threadPool.size() << " thread(s) to exit");
		_serial_io_service.stop();
		_threadPool.join_all();
		clear();
		IINFO("Done");
	}
	boost::asio::io_service _serial_io_service;
private:

	boost::thread_group _threadPool;
};
}

#endif /* INCLUDE_FLEXIBITY_SERIALPORTMGR_HPP_ */
