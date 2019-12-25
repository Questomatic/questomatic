/*
 * jsonRpcBoost.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_JSONRPC_JSONRPCBOOST_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_JSONRPCBOOST_HPP_

#ifndef JSONRPC_API
#define JSONRPC_API // get rid of this!
#endif

#include "jsonrpc/jsonrpc.h"

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include "flexibity/log.h"
#include "flexibity/util.hpp"
#include "flexibity/ioSerial.hpp"

namespace Flexibity {

/// Public interface for a complete async, separate threaded JSON-RPC endpoint
///
/// This class does not directly handle socket IO. It does handle
/// parsing input from, and formatting output to, the other endpoint.
///
/// When data is received from the other endpoint, it should be passed
/// into the JsonRpc object via one of the feedInput() overloaded
/// methods.
///
/// The invoke() method is used to invoke a method on the other
/// endpoint. The invocation will be sent via the current
/// JsonRpcOutputStream object.
///
/// If no output stream is set (which is true after construction, or
/// if setOutputStream is called with a null pointer, or if the
/// weak_ptr expires), then a remote method invocation will throw a
/// JsonRpcInvalidOutputStream exception.
///
/// All processing is done in a separate thread, and all methods are
/// called via boost::io_service::post() method
class jsonRpcBoost:
		public ioSerialMsgReceiver{
public:
	typedef shared_ptr<jsonRpcBoost> sPtr;

	typedef shared_ptr<JsonRpcMethod> BoostJsonRpcMethod;

	jsonRpcBoost() :
			_rpc(),
			_service(),
			_work(_service),
			_thread((boost::bind(&jsonRpcBoost::loop, this)))
			 {
		ILOG_INITSev(INFO);
		IINFO("");
	}

	virtual ~jsonRpcBoost() {
		_service.stop();
		if (_thread.joinable()) {
			_thread.join();
		}
		IINFO("");
	}

	/// Set output stream used for sending data to the other endpoint
	void setOutputStream(std::weak_ptr<JsonRpcOutputStream> outputStream){
		ITRACE("");
		_service.post(boost::bind(&jsonRpcBoost::setOutputStreamOp, this, outputStream));
	}

	/// Add a client method that the other endpoint can invoke
	void addMethod(const std::string &methodName,
			BoostJsonRpcMethod method){
		ITRACE("");
		std::shared_ptr<const std::string> mn(new std::string(methodName));//, strDeleter());
		_service.post(boost::bind(&jsonRpcBoost::addMethodOp, this, mn, method));
	}

	/// Add a client method that the other endpoint can invoke
	void removeMethod(const std::string &methodName){
		ITRACE("");
		std::shared_ptr<const std::string> mn(new std::string(methodName));//, strDeleter());
		_service.post(boost::bind(&jsonRpcBoost::removeMethodOp, this, mn));
	}

	boost::asio::io_service& getService(){
		return _service;
	}

	/// Invoke a method on the other endpoint
	///
	/// Note that only a weak reference to the callback is kept. If the
	/// callback is no longer valid when a response is received, the
	/// response will be dropped.
	///
	/// If the current output stream is invalid, an InvalidOutputStream
	/// exception is thrown.
	void invoke(const std::string &methodName, const Json::Value &params,
			std::weak_ptr<JsonRpcCallback> callback){

		std::shared_ptr<const std::string> mn(new std::string(methodName));//, strDeleter());
		std::shared_ptr<const Json::Value> p ( new Json::Value(params));//, jsonDeleter());
		IDEBUG(methodName << ", " << params.toStyledString());
		_service.post(boost::bind(&jsonRpcBoost::invokeOp, this, mn, p, callback));
	}

	// Input data received from the other endpoint to JsonReader
	virtual void feed(Flexibity::serialMsgContainer buffer) override{
		if(!buffer || buffer->size() == 0){
			return;
		}
		IDEBUG(Flexibity::log::dump(*buffer));
		BOOST_ASSERT(buffer->size() != 0);
		_service.post(boost::bind(&jsonRpcBoost::feedInputOp, this, buffer));
	}

protected:
	JsonRpc _rpc;
	boost::asio::io_service _service;
	boost::asio::io_service::work _work;
	boost::thread _thread;

	map<string, BoostJsonRpcMethod> _methods; //TODO: list cleanup???

protected:
	void setOutputStreamOp(std::weak_ptr<JsonRpcOutputStream> outputStream){
		IINFO("");
		_rpc.setOutputStream(outputStream);
	}
	void addMethodOp(std::shared_ptr<const std::string> methodName,
			BoostJsonRpcMethod method){
		ITRACE(*methodName);
		_methods.insert({*methodName, method});
		_rpc.addMethod(*methodName, method);
	}
	void removeMethodOp(std::shared_ptr<const std::string> methodName){
		ITRACE(*methodName);

		_methods.erase(*methodName);
		_rpc.removeMethod(*methodName);
	}
	void invokeOp(std::shared_ptr<const std::string> methodName, std::shared_ptr<const Json::Value> params,
				std::weak_ptr<JsonRpcCallback> callback){
		ITRACE(*methodName << ", " << params->toStyledString());
		_rpc.invoke(*methodName, *params, callback);
	}

	void feedInputOp(std::shared_ptr<const std::string> buffer) {
		IDEBUG(Flexibity::log::dump(*buffer));
		BOOST_ASSERT(buffer->size() != 0);
		_rpc.feedInput(*buffer);
	}

	void loop() {
		IINFO("enter");
		while (!_service.stopped()) {
			size_t ev = _service.run_one(); // processes the tasks
			IDEBUG("processed " << ev << " event(s)");
		}
		IINFO("exit");
	}
	jsonRpcBoost(const jsonRpcBoost& a) :
		_rpc(),
		_service(),
		_work(_service),
		_thread((boost::bind(&jsonRpcBoost::loop, this)))
	{
		ILOG_INIT();
		IINFO("");
	}
};

}

#endif /* INCLUDE_FLEXIBITY_JSONRPC_JSONRPCBOOST_HPP_ */
