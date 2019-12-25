/*
 * ioService.hpp
 *
 *  Created on: Aug 10, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_IOSERIAL_HPP_
#define INCLUDE_FLEXIBITY_IOSERIAL_HPP_

#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

#include <memory>

#include <flexibity/log.h>
#include <flexibity/util.hpp>
#include <flexibity/lock.hpp>
#include <termios.h>

#define bumpListeners(func, opts...) \
	for(auto& r:_listeners){ \
		if(r) r->func(opts); \
	}

#define setSerialOpt(getter, opt) \
	{\
		auto o = getter(cfg);\
		if(o){ \
			IDEBUG("setting " << opt); cout.flush();\
			_serialPort.set_option(*o);\
		}\
	}
#define setCustomOpt(getter, setter) \
	{\
		auto o = getter(cfg);\
		if(o)\
			setter(*o);\
	}

#define checkAndGet(name, as) \
	if(!p.isMember(name))\
		return nullptr;\
	auto o = p[name]; \
	IINFO("getting " << name << ": " << o.toStyledString()); \
	auto op = o.as();

namespace Flexibity {

using namespace std;

class ioSerial;

typedef const std::string serialMsg;
typedef std::string oSerialMsg;
typedef std::shared_ptr<serialMsg> serialMsgContainer;
typedef std::shared_ptr<oSerialMsg> oSerialMsgContainer;
using namespace boost::asio;
using namespace std;

class ioSerialEventListener{
public:
	typedef shared_ptr<ioSerialEventListener> sPtr;

	ioSerialEventListener():
			_ioSerial(NULL){}
	ioSerialEventListener(ioSerial* instance):
		_ioSerial(instance){}
	virtual ~ioSerialEventListener() {}
	virtual void onRead (const char * buf, size_t len) = 0;
	virtual void onWrite(serialMsgContainer msg) = 0;
	virtual void onEvent(serialMsgContainer msg) = 0;
	virtual void onError(const boost::system::error_code){} //TODO: custom error codes
	virtual void onClose(const boost::system::error_code){}
	virtual void onOpen (const boost::system::error_code){}
protected:
	ioSerial* _ioSerial;
};

class ioSerialPacketizedReader:public ioSerialEventListener{
public:
	ioSerialPacketizedReader(){}
	ioSerialPacketizedReader(ioSerial* instance):
		ioSerialEventListener(instance){}
	virtual ~ioSerialPacketizedReader() {}

	virtual void onRead (serialMsgContainer msg) = 0;
	virtual void onWrite(serialMsgContainer) {}
	virtual void onEvent(serialMsgContainer) {}
	virtual void onError(const boost::system::error_code){} //TODO: custom error codes
	virtual void onClose(const boost::system::error_code){}
	virtual void onOpen (const boost::system::error_code){}
	virtual void onWrite(const boost::system::error_code){}
	virtual void onRead (const char * buf, size_t len){
		if(len == 0)
			return;
		serialMsgContainer b(new serialMsg(buf, len));//, strDeleter());
		onRead(b);
	}
};

template <typename T>
struct optInfo{
	string name;
	shared_ptr<T> opt;
};

class ioSerialMsgReceiver:
		public logInstanceName{
public:

	typedef shared_ptr<ioSerialMsgReceiver> sPtr;

	ioSerialMsgReceiver(){
		ILOG_INITSev(INFO);
	}

	virtual void feed(serialMsgContainer) = 0;

	void feed(const char * const buffer, const std::size_t length) {
		if(length == 0){
			return;
		}

		Flexibity::serialMsgContainer b(new std::string(buffer, length));//, strDeleter());
		feed(b);
	}

	void feed(serialMsg& m) {
		if(m.size() == 0){
			return;
		}
		Flexibity::serialMsgContainer mc(new std::string(m));//, strDeleter());
		feed(mc);
	}
	virtual ~ioSerialMsgReceiver(){}
};

class communicationStub:
		public ioSerialMsgReceiver{
public:
	communicationStub(){
		ILOG_INIT();
		setInstanceName("communicationStub");
	}

	virtual void feed(serialMsgContainer msg) override{
		IDEBUG(log::dump(msg));
	}
};

class ioSerialDumper:
		public ioSerialPacketizedReader,
		public logInstanceName{
public:
	typedef shared_ptr<ioSerialDumper> sPtr;
	ioSerialDumper(ioSerial* instance);
	virtual ~ioSerialDumper() {
		txDump.close();
		rxDump.close();
	}

	virtual void onWrite(serialMsgContainer msg) {
		if(txDump.is_open()){
			txDump.write(msg->data(), msg->length());
			txDump.flush();
		}
	}
	virtual void onRead (serialMsgContainer msg){
		if(rxDump.is_open()){
			rxDump.write(msg->data(), msg->length());
			rxDump.flush();
		}
	}

	virtual void onError(const boost::system::error_code){} //TODO: custom error codes
	virtual void onClose(const boost::system::error_code){}
	virtual void onOpen (const boost::system::error_code){}
	virtual void onWrite(const boost::system::error_code){}
protected:
	void open(ofstream& f, const string& fn){
		f.open(fn, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
		if(!f.is_open())
			IERROR("Unable to open: " + fn);
	}
	ofstream txDump;
	ofstream rxDump;
};

class ioSerial:
		public ioSerialMsgReceiver{
public:
	typedef shared_ptr<ioSerial> sPtr;

	typedef shared_ptr<serial_port::parity> parityOpt;
	typedef shared_ptr<serial_port::character_size> characterSizeOpt;
	typedef shared_ptr<serial_port::stop_bits> stopBitsOpt;
	typedef shared_ptr<serial_port::flow_control> flowControlOpt;
	typedef shared_ptr<serial_port::baud_rate> baudRateOpt;
	typedef shared_ptr<bool> boolOpt;
	typedef shared_ptr<string> stringOpt;

	enum eSignal {
		RTS = TIOCM_RTS, //settable
		DTR = TIOCM_DTR,

		CTS = TIOCM_CTS,  //gettable
		DSR = TIOCM_DSR,
		RI = TIOCM_RI,
		CD = TIOCM_CD,
	};

	enum flush_type
	{
	  flushI = TCIFLUSH,
	  flushO = TCOFLUSH,
	  flushIO = TCIOFLUSH
	};

	struct signals { //should be in order with ioctl-types.h //TODO: strict mapping???
		unsigned int LE		:1;
		unsigned int DTR	:1;
		unsigned int RTS	:1;
		unsigned int ST		:1;
		unsigned int SR		:1;
		unsigned int CTS	:1;
		unsigned int CD		:1;
		unsigned int RI		:1;
		unsigned int DSR	:1;
	};

	static constexpr const char* deviceFileOption = "port";
	static constexpr const char* parityOption = "parity";
	static constexpr const char* characterSizeOption = "characterSize";
	static constexpr const char* stopBitsOption = "stopBits";
	static constexpr const char* flowControlOption = "flowControl";
	static constexpr const char* baudRateOption = "speed";
	static constexpr const char* rtsOption = "rts";
	static constexpr const char* dtrOption = "dtr";
	static constexpr const char* nameOption = "name";
	static constexpr const char* dumpOption = "dump";
	static constexpr const char* readUntilOption = "readUntil";

	static const std::pair<std::string, ioSerial::sPtr> getPair(boost::asio::io_service& io_service, Json::Value cfg){
		return {cfg[deviceFileOption].asString(), ioSerial::sPtr(new Flexibity::ioSerial(io_service, cfg))};
	}

	ioSerial(boost::asio::io_service& io_service):
		_active(false), _ioService(io_service), _serialPort(io_service){
		ILOG_INITSev(INFO);
	}

	ioSerial(boost::asio::io_service& io_service, const Json::Value& cfg):
		ioSerial(io_service) {
		open(cfg);
	}

	ioSerial(boost::asio::io_service& io_service, unsigned int baud,
					const string& device) :
						ioSerial(io_service){
		Json::Value o;
		o[baudRateOption] = baud;
		o[nameOption] = device;
		open(device);
	}

	virtual ~ioSerial(){
		boost::system::error_code e;
		closeOp(e);
	}

	virtual void feed(serialMsgContainer msg){
		if(!msg)
			return;
		ITRACE_IN(msg);
		if(!active()){
			IERROR("serial is already closed");
			return;
		}

		_ioService.post(boost::bind(&ioSerial::writeOp, this, msg, false));
	}

	void close() // call the do_close function via the io service in the other thread
	{
		IINFO("closed");
		_ioService.post(
				boost::bind(&ioSerial::closeOp, this,
						boost::system::error_code()));
	}

	bool active() // return true if the socket is still active
	{
		return _active;
	}

	void addEventListener(ioSerialEventListener::sPtr rd){
		ITRACE_IN("");
		_listeners.push_back(rd);
	}

	void setRTS(bool enabled){
		IDEBUG(enabled);
		setSignal(RTS, enabled);
	}

	void setDTR(bool enabled){
		IDEBUG(enabled);
		setSignal(DTR, enabled);
	}

	void flush(flush_type what){
		//TODO: thread safety???
		lock lk(_signals_mutex);
		if(!active())
			return;
		if (0 != ::tcflush(_serialPort.lowest_layer().native_handle(), what))
		{
			IERROR("Flush error");
		}
	}

	//signal can be mask
	int getSignal(int signal){
		lock lk(_signals_mutex);
		if(!active())
			return 0;
		int fd = _serialPort.native_handle();
		int data = 0;
		ioctl(fd, TIOCMGET, &data);

		string strSig = sigToName(signal);

		//bool result = (data & signal);
		//IDEBUG(strSig << " is " << result);
		return data & signal;
	}

	signals getSignals(){
		signals sig;
		int *i = (int*)&sig;
		*i = getSignal(DTR|DSR|RI|CTS|RTS|CD);
		return sig;
	}

	void setOptions(const Json::Value& cfg){
		try{
			setCustomOpt(getName, setName);
			setSerialOpt(getBaudRate, baudRateOption);
			setSerialOpt(getCharacterSize, characterSizeOption);
			setSerialOpt(getParity, parityOption);
			setSerialOpt(getStopBits, stopBitsOption);
			setSerialOpt(getFlowControl, flowControlOption);
			setCustomOpt(getRTS, setRTS);
			setCustomOpt(getDTR, setDTR);
			setCustomOpt(getDump, setDump);
		}catch (boost::system::system_error& e) {
			IERROR("Exception: " << e.what());
			throw e;
		}
	}


	//signal can be mask, all bits are set
	bool setSignal(int signal, bool level)
	{
		lock lk(_signals_mutex);
		if(!active())
			return false;
		signal &= RTS|DTR;
		string strSig = sigToName(signal);
		int fd = _serialPort.native_handle();
		if (level){
			IDEBUG("Setting " << strSig);
			if (ioctl(fd, TIOCMBIS, &signal) == -1) {
				IERROR("TIOCMBIS");
				return false;
			}
		}else{
			IDEBUG("Resetting " << strSig);
			if (ioctl(fd, TIOCMBIC, &signal) == -1) {
				IERROR("TIOCMBIC");
				return false;
			}
		}
		return true;
	}

	bool toggleSignal(int signal){
		lock lk(_signals_mutex);
		if(!active())
			return false;
		int fd = _serialPort.native_handle();
		int sigOrig, sig;
		if (ioctl(fd, TIOCMGET, &sigOrig) == -1) {
			IERROR("TIOCMGET");
			return false;
		}
		sig = ~sigOrig;
		sig &= signal;
		sig |= (sigOrig & ~signal);
		if (ioctl(fd, TIOCMSET, &sig) == -1) {
			IERROR("TIOCMSET");
			return false;
		}
		return true;
	}

private:
	static const int _readBufSize = 512; // maximum amount of data to read in one operation

#define MERGE_OPTION(opName, def) \
	if(!cfg.isMember(opName)) \
		o[opName] = def

	Json::Value mergeDefaultOptions(const Json::Value& cfg){
		IDEBUG("setting default options");
		Json::Value o(cfg);
		MERGE_OPTION(parityOption, "none");
		MERGE_OPTION(characterSizeOption, 8);
		MERGE_OPTION(stopBitsOption, "1");
		MERGE_OPTION(flowControlOption, "none");
		MERGE_OPTION(baudRateOption, 115200);
		MERGE_OPTION(rtsOption, false);
		MERGE_OPTION(dtrOption, false);
		MERGE_OPTION(dumpOption, false);
		MERGE_OPTION(readUntilOption, "");
		return o;
	}

#undef MERGE_OPTION

	string sigToName(int sig){
		string result;
		if(sig & TIOCM_RTS)
			result += "RTS ";
		if(sig & TIOCM_DTR)
			result += "DTR ";
		if(sig & TIOCM_CTS)
			result += "CTS ";
		if(sig & TIOCM_DSR)
			result += "DSR ";
		if(sig & TIOCM_RI)
			result += "RI ";
		if(sig & TIOCM_CD)
			result += "CD ";
		return result;
	}

	void open(const Json::Value& cfg){
		setCustomOpt(getName, setName);

		std::string deviceStr = "";

		if(cfg[deviceFileOption].isString()){
			deviceStr = cfg[deviceFileOption].asString();
		} else if (cfg[deviceFileOption].isArray()) {
			for(auto & device : cfg[deviceFileOption]) {
				if (boost::filesystem::exists(device.asString())) {
					deviceStr = device.asString();
					break;
				}
			}
			if(deviceStr.empty()) {
				IERROR("Device not found, searching through node: " << cfg[deviceFileOption].toStyledString());
				boost::system::system_error e(boost::system::error_code(-1,
																		boost::asio::error::get_system_category()
																		), "<Unknown device>");
				throw e;
			}
		}

		do_open(cfg, deviceStr);
	}

	void do_open(const Json::Value& cfg, const std::string& device) {
		setInstanceName(device);
		setCustomOpt(getName, setName);
		auto opts = mergeDefaultOptions(cfg);

		try{
			IINFO("Opening device " << device);
			_serialPort.open(device);
			flush(flushIO);
			// set the options after the port has been opened
			_active = true;
		}catch (boost::system::system_error& e) {
			IERROR("Exception: " << e.what() << " " << device << "\n");
			throw e;
		}
		if (not _serialPort.is_open()) {
			IERROR("Failed to open serial port");
			return;
		}

		_ioService.post(boost::bind(&ioSerial::readStartOp, this));
		IINFO("port opened");

		setOptions(opts);
	}

	void open(const std::string& device){
		Json::Value cfg;
		cfg[deviceFileOption] = device;
		open(cfg);
	}

	parityOpt getParity(const Json::Value& p){ //even, odd, none
		checkAndGet(parityOption, asString);
		if(op == "none")
			return make_shared<serial_port::parity>(serial_port::parity::none);
		if(op == "even")
			return make_shared<serial_port::parity>(serial_port::parity::even);
		if(op == "odd")
			return make_shared<serial_port::parity>(serial_port::parity::odd);

		return nullptr;
	}

	characterSizeOpt getCharacterSize(const Json::Value& p){ //5,6,7,8,9
		checkAndGet(characterSizeOption, asUInt);
		if(op < 5 || op > 8)
			return nullptr; //exception???
		return make_shared<serial_port::character_size>(
						serial_port::character_size(op));
	}

	stopBitsOpt getStopBits(const Json::Value& p){ //1,1.5,2
		checkAndGet(stopBitsOption, asString);
		if(op == "1")
			return make_shared<serial_port::stop_bits>(
						serial_port::stop_bits::one);
		if(op == "1.5")
			return make_shared<serial_port::stop_bits>(
						serial_port::stop_bits::onepointfive);
		if(op == "2")
			return make_shared<serial_port::stop_bits>(
						serial_port::stop_bits::two);
		return nullptr;
	}

	flowControlOpt getFlowControl(const Json::Value& p){ //none,software,hardware
		checkAndGet(flowControlOption, asString);
		if(op == "none")
			return make_shared<serial_port::flow_control>(serial_port::flow_control::none);
		if(op == "software")
			return make_shared<serial_port::flow_control>(serial_port::flow_control::software);
		if(op == "hardware")
			return make_shared<serial_port::flow_control>(serial_port::flow_control::hardware);

		return nullptr;
	}

	baudRateOpt getBaudRate(const Json::Value& p){
		checkAndGet(baudRateOption, asUInt);
		return make_shared<serial_port::baud_rate>(op); //baud rate (int);
	}

	stringOpt getName(const Json::Value& p){
		checkAndGet(nameOption, asString);
		return make_shared<string>(op);
	}

	boolOpt getDTR(const Json::Value& p){
		checkAndGet(dtrOption, asBool);
		return make_shared<bool>(op);
	}

	boolOpt getRTS(const Json::Value& p){
		checkAndGet(rtsOption, asBool);
		return make_shared<bool>(op);
	}

	boolOpt getDump(const Json::Value& p){
		checkAndGet(dumpOption, asBool);
		return make_shared<bool>(op);
	}

	stringOpt getReadUntil(const Json::Value& p){
		checkAndGet(readUntilOption, asString);
		return make_shared<string>(op);
	}

	void setDump(bool enabled){
		IDEBUG(enabled);
		if(enabled){
			ioSerialEventListener::sPtr d = ioSerialDumper::sPtr(new ioSerialDumper(this));
			addEventListener(d);
		}
	}

	void setName(const string& name){
		setInstanceName(name);
	}

	void setReadUntil(const string& readUntil){
		IINFO(readUntil);
	}

	void readStartOp(void) { // Start an asynchronous read and call read_complete when it completes or fails
		ITRACE_IN("");
		_serialPort.async_read_some(
				boost::asio::buffer(_readBuf, _readBufSize),
				boost::bind(&ioSerial::readCompleteOp, this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
	}

	void readCompleteOp(const boost::system::error_code& error,
			size_t bytes_transferred) { // the asynchronous read operation has now completed or failed and returned an error
		if (!error) { // read completed, so process the data
			IDEBUG(Flexibity::log::dump(_readBuf, bytes_transferred));
			bumpListeners(onRead, _readBuf, bytes_transferred);
			readStartOp(); // start waiting for another asynchronous read again
		} else
			closeOp(error);
	}

	void writeOp(serialMsgContainer msg, bool event) { // callback to handle write call from outside this class
		lock lk(_writeMsgs_mutex);
		if(!active()){
			IERROR("serial is already closed");
			return;
		}
		IDEBUG(Flexibity::log::dump(msg));

		if(event){
			bumpListeners(onEvent, msg);
			return;
		}

		bool write_in_progress = !_writeMsgs.empty(); // is there anything currently being written?
		_writeMsgs.push_back(msg); // store in write buffer
		if (!write_in_progress) // if nothing is currently being written, then start
			writeStartOp();
	}

	void writeStartOp(void) { // Start an asynchronous write and call write_complete when it completes or fails
		ITRACE_IN("");
		auto m = _writeMsgs.front();
		boost::asio::async_write(_serialPort,
				boost::asio::buffer(*m, m->size()),
				boost::bind(&ioSerial::writeCompleteOp, this,
						boost::asio::placeholders::error));
		bumpListeners(onWrite, m);
	}

	void writeCompleteOp(const boost::system::error_code& error) { // the asynchronous read operation has now completed or failed and returned an error
		ITRACE_IN("");
		lock lk(_writeMsgs_mutex);
		if (!error) { // write completed, so send next write data
			_writeMsgs.pop_front(); // remove the completed data
			if (!_writeMsgs.empty()) // if there is anthing left to be written
				writeStartOp(); // then start sending the next item in the buffer*/
		} else
			closeOp(error);
	}

	void closeOp(const boost::system::error_code& error) { // something has gone wrong, so close the socket & make this object inactive
		ITRACE_IN("");
		if (error == boost::asio::error::operation_aborted) // if this call is the result of a timer cancel()
			return; // ignore it because the connection cancelled the timer

		lock lk(_writeMsgs_mutex);
		if (error)
			IERROR("Error: " << error.message() << endl)
		/*else
			IERROR("Error: Connection did not succeed");*/
		_serialPort.close();
		_active = false;
		IWARN("Dropping output queue (" << _writeMsgs.size() << " msgs)")
		_writeMsgs.clear();
		bumpListeners(onClose, error);
	}

private:
	bool _active; // remains true while this object is still operating
	boost::asio::io_service& _ioService; // the main IO service that runs this connection
	boost::asio::serial_port _serialPort; // the serial port this instance is connected to
	char _readBuf[_readBufSize]; // data read from the socket
	std::deque<serialMsgContainer> _writeMsgs; // buffered write data
	mutable lockM _writeMsgs_mutex;
	mutable lockM _signals_mutex;

	std::list<ioSerialEventListener::sPtr> _listeners;
};

ioSerialDumper::ioSerialDumper(ioSerial* instance):
	ioSerialPacketizedReader(instance){
	ILOG_INITSev(INFO);
	if(instance){


		size_t pos = instance->instanceName().rfind("/");
		if(pos == string::npos){
			pos = 0;
		}else
			pos += 1;
		string fn(instance->instanceName(), pos);
		setInstanceName(fn);
		IINFO("opening dump files");
		open(rxDump, fn + ".rx.dump");
		open(txDump, fn + ".tx.dump");
	}else
		IERROR("instance is null");
}

}

#undef setSerialOpt
#undef setCustomOpt
#undef checkAndGet
#undef bumpListeners

#endif /* INCLUDE_FLEXIBITY_IOSERIAL_HPP_ */
