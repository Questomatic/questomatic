/*
 * outputStreamTh.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: romeo
 */

#ifndef INCLUDE_FLEXIBITY_UNUSED_OUTPUTSTREAMTH_HPP_
#define INCLUDE_FLEXIBITY_UNUSED_OUTPUTSTREAMTH_HPP_

#include <flexibity/jsonrpc/tests/outputStream.hpp>

namespace Flexibity{

#define LK(x) std::lock_guard<std::mutex> lk(x)

class JsOST: public JsOS{
public:
	JsOST(std::shared_ptr<JsonRpc> rpc):JsOS(rpc){
		_instances++;
		instance = _instances;
		t = new std::thread([=] { tProc(); });//(&JsOST::tProc, this);
		LGF("JsOST: " << _instances );
	}
	JsOST(const JsOST& a):JsOS(a) {
		_instances++;
		instance = _instances;
		t = new std::thread([=] { tProc(); });//(&JsOST::tProc, this);
		LGF("JsOSTcp: " << _instances );
	}
	virtual ~JsOST(){
		_instances--;
		exit = true;
		if(t->joinable()){
			t->join();
		}
		LGF("~JsOST: " << _instances );
	}
	virtual void send(const std::string &buffer){
		LK(m);
		LGF("must send to party (" << buffer.length() << "): " << std::endl
			<< buffer );
		q.push(buffer);
		LGF(instance << " q" << ": " << q.size());
	}
private:
	void tProc(){
		LGF("thread enter" );
		sleep(1);
		while(/*_instances > 0*/!exit){
			std::string s = "";
			{
				LK(m);
				LGF("t" << instance << ": " << q.size() );
				if(!q.empty()){
					s = q.front();
					//LG("got some str: " << s);
					q.pop();
				}
			}
			if(s.length()){
				LGF("got to pass: " << s);
				if(auto p = _rpcParty.lock()){
					auto end = s.rfind("\n");
					if(end !=  std::string::npos){
						LGF("skipping line break in buffer");
					}else
						end = s.length();

					p->feedInput(s.c_str(), end);
				}
			}
			sleep(1);
		}
		LGF("thread exit" );
	}
	std::queue<std::string> q;
	std::thread* t = NULL;
	static int _instances;
	std::mutex m;
	int instance = 0;
	bool exit = false;
};

/*static*/ int JsOST::_instances = 0;

}


#endif /* INCLUDE_FLEXIBITY_UNUSED_OUTPUTSTREAMTH_HPP_ */
