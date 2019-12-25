#ifndef INCLUDE_FLEXIBITY_JSONRPC_STOPMETHOD_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_STOPMETHOD_HPP_

#include <flexibity/jsonrpc/genericMethod.hpp>
#include <flexibity/jsonrpc/audioPlayMethod.hpp>
#include <flexibity/log.h>
#include <sstream>
#include <map>
#include <flexibity/spawnUtil.hpp>
#include <sys/types.h>
#include <signal.h>

namespace Flexibity {

class audioStopMethod: public genericMethod {
public:
	static constexpr const char* command = "audio_stop";

	audioStopMethod() {
		ILOG_INIT();
	}

	virtual void invoke(
			  const Json::Value &params,
			  std::shared_ptr<Response> response) override {
		if (!response) {
			return; // if notification, do nothing
		}

		if(check_running()) {
			int k = kill(audioPlayMethod::_running, SIGKILL);
			if (k == 0 ) {
				response->sendResult(responseOK());
				return;
			} else {
				response->sendError("Can't stop playing. Already stopped?");
			}
		} else {
			response->sendError("nothing is playing");
			return;
		}

	}
private:
	bool check_running() {
		std::stringstream process;
		process << "/proc/" << audioPlayMethod::_running << "/cmdline";
		std::ifstream lf(process.str());
		return lf.is_open();
	}
};


}

#endif /* INCLUDE_FLEXIBITY_JSONRPC_STOPMETHOD_HPP_ */

