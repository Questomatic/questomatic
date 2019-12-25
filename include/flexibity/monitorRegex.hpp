/*
 * monitorRegex.hpp
 *
 *  Created on: Sep 25, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_MONITORREGEX_HPP_
#define INCLUDE_FLEXIBITY_MONITORREGEX_HPP_


#include <flexibity/log.h>
#include <boost/regex.hpp>
#include <flexibity/ioSerial.hpp>

namespace Flexibity{
/**
 * this class is not thread-safe!
 */


class monitorRegex:
		public Flexibity::ioSerialMsgReceiver{
public:
	typedef shared_ptr<monitorRegex> sPtr;

	static constexpr const char* breakOption = "break";
	static constexpr const char* idOption = "id";
	static constexpr const char* regexOption = "regexp";
	static constexpr const char* suppressOption = "suppress";
	static constexpr const char* timeoutOption = "timeout";
	static constexpr const char* bsOption = "bs";

	monitorRegex(const Json::Value& cfg){
		ILOG_INITSev(INFO);
		if(cfg.isMember(idOption))
			setInstanceName(cfg[idOption].asString());

		_breakOnMatch = cfg.get(breakOption, false).asBool();
		_suppress = cfg.get(suppressOption, false).asBool();

		auto regex = cfg.get(regexOption, "").asString();

		_searchBuf = make_shared<oSerialMsg>();

		_e.assign(regex, boost::regex_constants::normal);

		_bs = 0;
		IINFO("regexp: " << regex
				<< "; break: " << this->_breakOnMatch
				<< "; suppress: " << this->_suppress);
	}

	/**
	 * to trigger search, just pass in nullptr;
	 */
	virtual void feed( Flexibity::serialMsgContainer m ) override{

		if(m && m->size()){
			IDEBUG("Got new data from channel\n" << log::dump(m));
			*_searchBuf += *m;
		}else{
			IDEBUG("Got empty data from channel, just triggering search event");
		}

		bool matchesLeft = false;
		do{
			IDEBUG("New search iteration");
			if(_breakActive && !_unbreak){
				IDEBUG("Break is active");
				break;
			}
			matchesLeft = search();
			IDEBUG("matchesLeft: " << matchesLeft);
		}while(matchesLeft);

		return;
	}
	void unbreak(bool flush){ //feed empty data for unbreak! //TODO: fix dat shit!
		IINFO("flush: " << flush);
		if(flush)
			*_searchBuf = "";
		_unbreak = true;
	}
private:
	void feedReceiver(weak_ptr<ioSerialMsgReceiver> oStreamReceiver, oSerialMsgContainer inputStream, size_t feedSize, size_t eraseSize){

		IDEBUG("Stream Buf is " << log::dump(inputStream));

		if(auto r = oStreamReceiver.lock()){
			if(feedSize > 0){
				oSerialMsgContainer sendBuf = make_shared<oSerialMsg>();

				sendBuf->insert(0, *inputStream, 0, feedSize); //copy data to be sent from stream buffer
				IDEBUG("Feeding " << feedSize << " bytes to " << r->instanceName() << log::dump(inputStream->c_str(), feedSize));
				r->feed(sendBuf); //feed data to receiver
			}
		}

		if(eraseSize > 0){
			IDEBUG("Erasing " << eraseSize << " bytes" << log::dump(inputStream->c_str(), eraseSize));
			inputStream->erase(0, eraseSize);
		}
	}

	bool search(){

		string operName = "forward";  //for logging purposses
		if(auto r = _matchReceiver.lock()){
			r->setInstanceName("matchReceiver");
		}
		if(auto r = _streamReceiver.lock()){
			r->setInstanceName("streamReceiver");
		}

		if(_searchBuf->size() == 0){
			IDEBUG("monitor buffer is empty, skipping search");
			return false;
		}

		boost::sregex_iterator searchIterator(_searchBuf->begin(), _searchBuf->end(), _e,
						boost::match_default | boost::match_partial);
		boost::sregex_iterator end;

		oSerialMsgContainer streamBuf = make_shared<oSerialMsg>();

		IDEBUG("Searching in monitor buffer: " << Flexibity::log::dump(*_searchBuf));

		bool match = (searchIterator != end);

		if(!match){ //no match in search buffer. just forward all the data from it.
			IDEBUG("No match found in monitor buffer, " << operName << "ing" << Flexibity::log::dump(_searchBuf));
			feedReceiver(_streamReceiver, _searchBuf, _searchBuf->size(), _searchBuf->size()); // forward and cut all data from buffer
			return false;
		}

		// either full or partial match is in the buffer. Get the parameters
		auto matchPos = searchIterator->position();
		auto matchLength = searchIterator->length();
		bool full = (*searchIterator)[0].matched;

		// make match to be at the beginning of the buffer
		if(matchPos != 0){
			IDEBUG("Some match found at " << matchPos << "; matchLength=" << _searchBuf->size() - matchPos << ". Forwarding data before match (" << matchPos << " bytes) to make it at the beginning ...");
			feedReceiver(_streamReceiver, _searchBuf, matchPos, matchPos);

			//return true; // match still in the buffer, iterate one more time ???
			matchPos = 0; //we've sent the data before match and cut it from buffer, now the match is at the beginning of the buffer
		}

		if(!full){ // force buffering, as we cannot make a decision about match yet.
			IDEBUG("partial match at " << matchPos << "; matchLength=" << _searchBuf->size() - matchPos << ". waiting for more data..." << log::dump(_searchBuf));
			return false;
		}

		//Full match

		IINFO("FullMatch (length = " << matchLength << ") found in " << Flexibity::log::dump(_searchBuf));

		if(_breakActive && _unbreak){ // unbreak condition
			IINFO("unbreaking channel");
			if(_suppress){
				IINFO("Suppressing match (" << matchLength << "bytes) from buffer");
				feedReceiver(_streamReceiver, _searchBuf, 0, matchLength);
			} else {
				IINFO("Forwarding match (" << matchLength << "bytes) from buffer");
				feedReceiver(_streamReceiver, _searchBuf, matchLength, matchLength);
			}
			_breakActive = false; //reset break flag
			return true; // trigger new search on modified buffer after unbreak
		}

		if(!_breakActive){ //notify only on first match when break is still not active
			IINFO("trigger monitor");
			feedReceiver(_matchReceiver, _searchBuf, matchLength, 0);
			if(_breakOnMatch){
				IINFO("breaking channel");
				_breakActive = true;
				return false; // nothing to search in this buffer, we are on break.
			}

			if(_suppress) {
				IINFO("suppressing match (" << matchLength << "bytes) from stream")
				feedReceiver(_streamReceiver, _searchBuf, 0, matchLength);
			} else {
				IINFO("forwarding match (" << matchLength << "bytes) to stream")
				feedReceiver(_streamReceiver, _searchBuf, matchLength, matchLength);
			}

		}

		return true; //always try to re-iterate after search routine
	}

	/*  -------------------------------------------------------------------------
	 * | break | supress | action 												 |
	 *  -------------------------------------------------------------------------
	 * |   0   |    0    | pass input stream as is, just notify match			 |
	 * |   0   |    1	 | cut match from input stream, notify match             |
	 * |   1   |    0    | start buffering, notify match, leave match in stream  |
	 * |   1   |    1    | start buffering, notify match and cut it from stream  |
	 *  -------------------------------------------------------------------------
	 */

public:
	size_t _matches = 0;
	weak_ptr<ioSerialMsgReceiver> _matchReceiver; //TODO: setters/getters for this?
	weak_ptr<ioSerialMsgReceiver> _streamReceiver;

	serialMsgContainer searchBuf() const {
		return _searchBuf;
	}
private:
	boost::regex _e;
	bool _breakOnMatch = false;
	bool _suppress = false;
	bool _breakActive = false;
	bool _unbreak = false;

	size_t _bs; 		//TODO: buffer overflow trigger
	//size_t timeout; //TODO: timeouts???

	oSerialMsgContainer _searchBuf;
};

}

#endif /* INCLUDE_FLEXIBITY_MONITORREGEX_HPP_ */
