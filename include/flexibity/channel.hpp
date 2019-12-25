/*
 * ioService.hpp
 *
 *  Created on: Aug 13, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_CHANNEL_HPP_
#define INCLUDE_FLEXIBITY_CHANNEL_HPP_

#include <flexibity/channelMonitorRegex.hpp>
#include <flexibity/serialPortMgr.hpp>
#include <flexibity/serialFeedAggregator.hpp>
#include <flexibity/genericMgr.hpp>
#include <flexibity/jsonrpc/jsonRpcTransport.hpp>
#include <flexibity/exception.hpp>

namespace Flexibity {

using namespace std;

class ioSerial;

class serialChannel:
		public ioSerialMsgReceiver{
public:
	typedef ioSerial::sPtr port;
	typedef std::shared_ptr<serialChannel> sPtr;
	typedef std::shared_ptr<Flexibity::serialPortMgr> portMgr;

	static constexpr const char* nameOption = "name";
	static constexpr const char* iPortOption = "iPort";
	static constexpr const char* oPortOption = "oPort";
	static constexpr const char* monitorsOption = "monitors";

	static constexpr const char* unbreakCmd = "unbreak";

private:
	class rdProxy: public Flexibity::ioSerialPacketizedReader, public logInstanceName{
	public:
		rdProxy(serialChannel *cn): _cn(cn){ ILOG_INITSev(INFO); }
		virtual void onRead(Flexibity::serialMsgContainer m){
			IDEBUG(log::dump(m));
			_cn->feed(m);
		}
	private:
		serialChannel *_cn;
	};

public:
	serialChannel(boost::asio::io_service& io_service, port in, port out):_in(in), _out(out), _ioService(io_service){
		ILOG_INITSev(INFO);
		_reader = std::make_shared<rdProxy>(this);
		in->addEventListener(_reader);
	}

	serialChannel(boost::asio::io_service& io_service, portMgr mgr, Json::Value cfg): _ioService(io_service) {
		ILOG_INITSev(INFO);
		if(!cfg.isObject()){
			auto errStr = "serialChannel section is not an object";
			throw exception(errStr);
		}

		_reader = std::make_shared<rdProxy>(this);

		auto I = cfg[iPortOption].asString();
		auto O = cfg[oPortOption].asString();
		setInstanceName(cfg[nameOption].asString());
		IINFO("making channel between " << I << " and " << O << " with name " << instanceName());
		if(I == O)
			IWARN("loopback detected on " << instanceName() << " channel");

		if (!cfg[iPortOption].isNull()) {
			auto in = mgr->getItem(I);
			in->addEventListener(_reader);
			_in = in;
		} else {
			_in = std::make_shared<communicationStub>();
		}

		if (!cfg[oPortOption].isNull()) {
			_out = mgr->getItem(O);
		} else {
			_out = std::make_shared<communicationStub>();
		}

		_monitors.setInstanceName(name() + "-" + instanceName() + "-monitors");
		if(cfg.isMember(monitorsOption)){
			_monitors.populateItems(cfg[monitorsOption], [&](const Json::Value& iCfg){
				return make_shared<channelMonitorRegex>(iCfg, shared_ptr<jsonRpcTransport>());
			});
		}
	}

	serialChannel(boost::asio::io_service& io_service, portMgr mgr, string in, string out):
		serialChannel(io_service, mgr->getItem(in), mgr->getItem(out)){
	}

	virtual void feed(serialMsgContainer msg){
		IDEBUG(log::dump(msg));
		_ioService.post(boost::bind(&serialChannel::feedOp, this, msg, false, false));
	}

	void unbreak(bool flush){
		_ioService.post(boost::bind(&serialChannel::feedOp, this, nullptr, true, flush));
	}

	channelMonitorRegex::sPtr addMonitor(const Json::Value& cfg, jsonRpcTransport::sPtr rpc = jsonRpcTransport::sPtr()){
		IDEBUG(cfg.toStyledString());
		// get upgradable access
		boost::upgrade_lock<boost::shared_mutex> lock(_monitors_m);
		// get exclusive access
		boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

		channelMonitorRegex::sPtr m = nullptr;
		try{
			m = make_shared<channelMonitorRegex>(cfg, rpc);
		}catch(...){
			IERROR("Error creating monitor for channel \"" << instanceName() << "\" from request \n" << cfg.toStyledString() << dumpChannelItems());
			return nullptr;
		}

		string id = m->instanceName();

		if(0 == id.size()){
			id += std::to_string(_monitors.size()+1);
			id = instanceName() + "_" + id;

			if(rpc && rpc->instanceName().size())
				id = rpc->instanceName() + "_" + id;
		}

		m->setInstanceName(id);

		if(_monitors.insert({m->instanceName(), m}).second == false){
			IERROR("Unable to add monitor \"" << m->instanceName() << "\" to channel \"" << instanceName() << "\"" << dumpChannelItems());
			return nullptr;
		}
		IINFO("Added monitor \"" << m->instanceName() << "\" to channel \"" << instanceName() << "\"" << dumpChannelItems());
		return m;
	}

	void removeMonitor(const Json::Value& cfg){ //TODO: remove monitors only set by certain rpc server, FORCE logic
		IDEBUG(cfg.toStyledString());
		// get upgradable access
		boost::upgrade_lock<boost::shared_mutex> lock(_monitors_m);
		// get exclusive access
		boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
		auto id = cfg[monitorRegex::idOption].asString();
		try{
			auto m = _monitors.getItem(id);
			if(m && _monitors.erase(id) == 0)
				throw exception("Unable to erase monitor from map. Request: \n" + cfg.toStyledString() + dumpChannelItems());
		}
		catch(...){
			IERROR("Monitor not found. Request: \n" << cfg.toStyledString() << dumpChannelItems());
			throw;
		}
		IINFO(cfg.toStyledString() << dumpChannelItems());
		//TODO: cleanup and forward logic
	}

	void removeMonitors(jsonRpcTransport::sPtr rpc){ //TODO: remove monitors only set by certain rpc server, FORCE logic
		// get upgradable access
		boost::upgrade_lock<boost::shared_mutex> lock(_monitors_m);
		// get exclusive access
		boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
		auto itr = _monitors.rbegin();
		//TODO: cleanup and forward logic
		int count = 0;

		shared_ptr<ioSerialFeedAggregator> a = make_shared<ioSerialFeedAggregator>();
		while (itr != _monitors.rend()) {
			if (itr->second->isControlledBy(rpc)) {
					 auto search_buf = itr->second->searchBuf();
					 a->feed(search_buf);
			   _monitors.erase( --(itr.base()) );
			   count++;
			} else {
			   ++itr;
			}
		}
		auto monMessages = a->aggregate();
		_out->feed(monMessages);
		IINFO(count << " monitors deleted for \"" << rpc->instanceName() << "\"" << dumpChannelItems());
		IINFO("Data, recovered from monitors on break: " << log::dump(monMessages));
	}

	ioSerialMsgReceiver::sPtr getIn(){
		return _in;
	}

	ioSerialMsgReceiver::sPtr getOut(){
		return _out;
	}
protected:
	string dumpChannelItems(){
		string cfg = "\n channel configuration: " + _in->instanceName();
		for(auto&monPair:_monitors){
			auto mon = monPair.second;
			cfg += "->" + mon->instanceName();
		}
		cfg += "->" + _out->instanceName() + "\n";
		return cfg;
	}

	void feedOp(serialMsgContainer msg, bool unbreak, bool flush){
		// get shared access
		boost::shared_lock<boost::shared_mutex> lock(_monitors_m);

		shared_ptr<ioSerialFeedAggregator> a = make_shared<ioSerialFeedAggregator>();
		oSerialMsgContainer chainMsg;
		IDEBUG("Rolling data through through " << dumpChannelItems() << "with unbreak: " << unbreak << log::dump(msg))
		for(auto &monPair : _monitors){
			auto mon = monPair.second;
			mon->_streamReceiver = a;
			if(unbreak){
				IDEBUG("Sending unbreak to " << mon->instanceName());
				mon->unbreak(flush);
			}
			if(chainMsg){
				IDEBUG("Sending chain data (unbreak: " << unbreak <<  ") to port " << mon->instanceName() << log::dump(chainMsg))
				mon->feed(chainMsg);
			}else{
				IDEBUG("Sending msg data (unbreak: " << unbreak <<  ") to port " << mon->instanceName() << log::dump(msg))
				mon->feed(msg);
			}
			chainMsg = a->aggregate();
		}

		if(chainMsg){
			IDEBUG("Sending final chain data (unbreak: " << unbreak <<  ") to port " << _out->instanceName() << log::dump(chainMsg))
			_out->feed(chainMsg);
		}else{
			IDEBUG("Sending final msg data (unbreak: " << unbreak <<  ") to port " << _out->instanceName() << log::dump(msg))
			_out->feed(msg);
		}
	}


	shared_ptr<rdProxy> _reader;
	boost::shared_mutex _monitors_m;
	boost::asio::io_service& _ioService;

	genericMgr<channelMonitorRegex::sPtr> _monitors;
	ioSerialMsgReceiver::sPtr _in;
	ioSerialMsgReceiver::sPtr _out;
};


}

#endif /* INCLUDE_FLEXIBITY_CHANNEL_HPP_ */
