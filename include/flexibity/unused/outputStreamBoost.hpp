/*
 * outputStreamBoost.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_UNUSED_OUTPUTSTREAMBOOST_HPP_
#define INCLUDE_FLEXIBITY_UNUSED_OUTPUTSTREAMBOOST_HPP_

#include <flexibity/jsonrpc/tests/outputStream.hpp>
#include "flexibity/jsonrpc/jsonRpcBoost.hpp"
#include <memory>

namespace Flexibity{

class JsOSBoost: public JsonRpcOutputStream{
public:
	JsOSBoost(std::weak_ptr<Flexibity::jsonRpcBoost> rpc):
		service(),
		w(service),
		_rpcParty(rpc),
		bt((boost::bind(&JsOSBoost::loop, this))){

		LGF("");
		_instances++;
		_instance = _instances;

		LGF(_instances );
	}
	JsOSBoost(const JsOSBoost& a):
		service(),
		w(service),
		_rpcParty(a._rpcParty),
		bt((boost::bind(&JsOSBoost::loop, this))) {

		LGF("cp");
		_instances++;
		_instance = _instances;
		LGF("cp" << _instances );
	}
	virtual ~JsOSBoost(){
		_instances--;
		service.stop();
		if(bt.joinable()){
			bt.join();
		}
		LGF(_instances );
	}

	virtual void send(const std::string &buffer){
		auto buf = std::make_shared<std::string>(std::string(buffer));
		LGF("asked to post:" << endl << buf); //TODO: magage buffer!
		service.post(boost::bind(&JsOSBoost::sendOp, this, buf));
	}

private:
	void sendOp(std::shared_ptr<std::string> buffer){

		if(!buffer){
			LG("buffer is lost");
			return;
		}

		auto end = buffer->rfind("\n");
		if(end !=  std::string::npos){
			LGF("skipping line break in buffer");
		}else
			end = buffer->length();

		if(auto party = _rpcParty.lock()){
			LGF("feeding to party (" << buffer->length() << "): " << std::endl << buffer)
			//party->feedInput(buffer->c_str(), end);
		}else{
			LGF("feed buffer is not set: "
					<< std::endl << buffer);
		}
		/*LGF("received:" << endl << buffer);
		if(auto p = _rpcParty.lock())
			p->feedInput(buffer);
		else
			LGF("no buffer");*/
	}
	void loop(){
		LGF("enter");
		while(!service.stopped()){
			size_t ev = service.run_one(); // processes the tasks
			LGF("processed " << ev << " event(s)");
		}
		LGF("exit");
	}
	boost::asio::io_service service;
	boost::asio::io_service::work w;
	std::weak_ptr<Flexibity::jsonRpcBoost> _rpcParty;
	boost::thread bt;

	static int _instances;
	int _instance;

};

/*static*/ int JsOSBoost::_instances = 0;

}


#endif /* INCLUDE_FLEXIBITY_UNUSED_OUTPUTSTREAMBOOST_HPP_ */
