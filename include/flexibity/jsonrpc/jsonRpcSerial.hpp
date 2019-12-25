/*
 * jsonRpcSerial.hpp
 *
 *  Created on: Aug 14, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_JSONRPCSERIAL_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_JSONRPCSERIAL_HPP_

#include <flexibity/jsonrpc/jsonRpcTransport.hpp>
#include <flexibity/serialPortMgr.hpp>

namespace Flexibity{

using namespace std;

class jsonRpcSerialReader:
		public Flexibity::ioSerialPacketizedReader{
public:
	jsonRpcSerialReader(jsonRpcBoost::sPtr rpc):
		_rpc(rpc){};
private:
	virtual void onRead(serialMsgContainer msg) override{
		if(_rpc){
			_rpc->feed(msg);
		}
	}

	virtual void onClose(const boost::system::error_code e) override{
		GWARN("Close");
	}
	jsonRpcBoost::sPtr _rpc;
};

class jsonRpcSerialOutputStream:
		public JsonRpcOutputStream,
		public logInstanceName{
public:
	typedef shared_ptr<jsonRpcSerialOutputStream> sPtr;
	virtual ~jsonRpcSerialOutputStream(){}

	jsonRpcSerialOutputStream(ioSerialMsgReceiver::sPtr rpc, const string &name = "JsRpcSerOS"):
		_rpcParty(rpc){
		ILOG_INITSev(INFO);
		setInstanceName(name);
	}

	virtual void send(const char * buffer, size_t length) override{
		if(_rpcParty){
			IDEBUG(Flexibity::log::dump(buffer, length));
			_rpcParty->feed(buffer, length);
		}else{
			IERROR("feed buffer is not set: "
					<< std::endl << buffer);
		}
	};

protected:
	ioSerialMsgReceiver::sPtr _rpcParty;
};

class jsonRpcSerial:
		public Flexibity::jsonRpcTransport{
private:
	class rdProxy:
			public Flexibity::ioSerialPacketizedReader,
			public logInstanceName{
	public:
		typedef shared_ptr<rdProxy> sPtr;

		rdProxy(jsonRpcSerial *cn): _cn(cn){
			ILOG_INIT();
		}
		virtual void onRead(Flexibity::serialMsgContainer m) override{
			_cn->feed(m);
		}
	private:
		jsonRpcSerial *_cn;
	};

public:

	typedef shared_ptr<jsonRpcSerial> sPtr;
	static constexpr const char* portNameOption = "portName";
	static constexpr const char* nameOption = "name";

	jsonRpcSerial(ioSerial::sPtr p):jsonRpcTransport(){
		ILOG_INIT();
		init(p);
	}

	jsonRpcSerial(serialPortMgr::sPtr pm, const string& portName): jsonRpcSerial(pm->getItem(portName)){}

	jsonRpcSerial(serialPortMgr::sPtr pm, const Json::Value& cfg){
		ILOG_INIT();

		auto p = pm->getItem(cfg[portNameOption].asString());
		init(p);
		if(cfg.isMember(nameOption))
			setInstanceName(cfg[nameOption].asString());
	}

	virtual void setInstanceName(const std::string &n) override{
		_os->setInstanceName(n);
		_reader->setInstanceName(n);
		jsonRpcBoost::setInstanceName(n);
	}
protected:
	void init(ioSerial::sPtr p){
		_os = make_shared<jsonRpcSerialOutputStream>(p);
		_reader = make_shared<rdProxy>(this);

		setOutputStream(_os);
		p->addEventListener(_reader);
	}

private:
	jsonRpcSerialOutputStream::sPtr _os;
	rdProxy::sPtr _reader;
};

}


#endif /* INCLUDE_FLEXIBITY_JSONRPC_JSONRPCSERIAL_HPP_ */
