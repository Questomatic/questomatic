/*
 * jsonrpc-test.hpp
 *
 *  Created on: Aug 14, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_TESTS_JSONRPC_TEST_HPP_
#define INCLUDE_FLEXIBITY_TESTS_JSONRPC_TEST_HPP_

#include "flexibity/log.h"
#include "flexibity/jsonrpc/jsonRpcBoost.hpp"
#include <memory>

namespace Flexibity{
namespace test{

using namespace std;

class JsOS:
		public JsonRpcOutputStream,
		public logInstanceName{
public:
	virtual ~JsOS(){}

	JsOS(std::shared_ptr<Flexibity::jsonRpcBoost> rpc, string name = "JsOS"):
		_rpcParty(rpc){
		ILOG_INITSev(INFO);
		setInstanceName(name);
	}

	virtual void send(const std::string& buffer){
		if(auto party = _rpcParty.lock()){
			IDEBUG(Flexibity::log::dump(buffer));
			party->ioSerialMsgReceiver::feed(buffer);
		}else{
			IERROR("feed buffer is not set: "
					<< std::endl << buffer);
		}
	};

protected:
	std::weak_ptr<Flexibity::jsonRpcBoost> _rpcParty;
};

class JsM:
		public JsonRpcMethod,
		public logInstanceName{
public:
	JsM(std::string name){
		ILOG_INIT();
		setInstanceName(name);
	}
	virtual ~JsM(){}

	virtual void invoke(
		  const Json::Value &params,
		  std::shared_ptr<Response> response){
		static int m = 0;
		IINFO("Calling method! params:" << std::endl << params.toStyledString() );
		Json::Value respP;//(m++);
		respP["value"] = m++;
		respP["origin"] = _log->instance();
		if(response) //TODO: copy this to all other method instances, or better make a dedicated class!
			response->sendResult(respP);//respP);
		else
			IWARN("got a notification");
	}
};

class JsCb:
		public JsonRpcCallback,
		public logInstanceName{
public:
	JsCb(std::string name, int iter = 0){
		ILOG_INIT();
		setInstanceName(name);
	}
	virtual ~JsCb(){}

	virtual void response(Json::Value const & response){
		IINFO("invoking callback! params:" << std::endl << response.toStyledString() );

		/*try{
			int r = response["result"].asInt();
		}catch(std::exception& e){
			IERROR(e.what());
		}*/

		batchIter_c++;
		_iter_c++;
	}

	void reset(){ batchIter_c = 0; }
	int batchIter() { return batchIter_c; }
	int iter() { return _iter_c; }

private:
	int batchIter_c = 0;
	int _iter_c = 0;
};

template <class T>
class jsonrpcTest:
	public logInstanceName{
public:
	jsonrpcTest(){
		ILOG_INITSev(INFO);
	}
	void run(std::shared_ptr<T> rpc, const std::string mn, std::shared_ptr<JsCb> cb, int iter, int iterBatch, int batchTimeo){

		int iterLeft = iter;
		int lost = 0;
		while(iterLeft > 0){
			int iterBatchCnt = iterBatch;
			if(iterBatch > iterLeft)
				iterBatch = iterLeft;
			while(iterBatchCnt > 0 && iterLeft > 0){
				Json::Value vN(iter - iterLeft);
				rpc->invoke(mn, vN, cb);
				iterLeft--;
				iterBatchCnt--;
			}
			auto tr = std::chrono::high_resolution_clock::now();
			IINFO("batch(" << iterBatch << ") TotalSent(" << (iter - iterLeft) << " of " << iter <<  "), received: " << cb->iter() << ", totalLost: " << lost);
			int timeo = batchTimeo;
			while(--timeo > 0){
				usleep(1000);
				if(cb->batchIter() >= iterBatch){
					cb->reset();
					break;
				}
			}//TODO: show wait loop time
			if(timeo == 0){
				int lost_t = (iter - iterLeft) - cb->iter() - lost;
				lost += lost_t;
				IERROR("Batch timeout. lost in batch: " << lost_t << ", totalLost: " << lost);
				//break;
			}else{
				auto tn = std::chrono::high_resolution_clock::now();
				IINFO("Batch recieved in: " << LG_FMT_TIME(tn, tr));
			}
		}
		if(cb->iter() != iter){
			IERROR(lost << " callbacks lost!");
		}
	}
};

}
}


#endif /* INCLUDE_FLEXIBITY_TESTS_JSONRPC_TEST_HPP_ */
