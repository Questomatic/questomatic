/*
 * serialPortMgr.hpp
 *
 *  Created on: Aug 13, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_CHANNELMGR_HPP_
#define INCLUDE_FLEXIBITY_CHANNELMGR_HPP_

#include <jsoncpp/json/json.h>
#include <flexibity/channel.hpp>
#include <flexibity/genericMgr.hpp>
#include <flexibity/jsonrpc/jsonRpcTransport.hpp>

namespace Flexibity{

class channelMgr:
		public genericMgr<serialChannel::sPtr>{
public:
	typedef shared_ptr<channelMgr> sPtr;
	static constexpr const char* threadPoolOption = "threadPool";

	channelMgr(const Json::Value& cfg, serialPortMgr::sPtr pm):
	 _channel_io_service(),
	 _work(_channel_io_service),
	_pm(pm),
	_threadPool() {
		ILOG_INITSev(INFO);

		populateItems(cfg, [&](const Json::Value& iCfg){
			return std::make_shared<Flexibity::serialChannel>(_channel_io_service, pm, iCfg);
		});

		size_t _size = 1;
		size_t tp = cfg[threadPoolOption].asUInt();
		if(tp != 0)
			_size = tp;
		for ( size_t i = 0; i < _size; ++i){
			_threadPool.create_thread(boost::bind(&boost::asio::io_service::run, &_channel_io_service));
		}
		IINFO(_threadPool.size() << " channel worker thread(s) created");
	}


	void loop() {
		IDEBUG("enter");
		while (!_channel_io_service.stopped()) {
			size_t ev = _channel_io_service.run(); // processes the tasks
			IDEBUG("processed " << ev << " event(s)");
		}
		IDEBUG("exit");
	}

	void clearChannels(jsonRpcTransport::sPtr _rpc){
		for(auto& channel:*this){
			channel.second->removeMonitors(_rpc);
		}
	}

	virtual ~channelMgr(){
		IINFO("waiting for " << _threadPool.size() << " thread(s) to exit");
		_channel_io_service.stop();
		_threadPool.join_all();
		IINFO("Done");
	}

	boost::asio::io_service _channel_io_service;
	boost::asio::io_service::work _work;
	Flexibity::serialPortMgr::sPtr	_pm;
	boost::thread_group _threadPool;
};



}

#endif /* INCLUDE_FLEXIBITY_SERIALPORTMGR_HPP_ */
