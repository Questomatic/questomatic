#ifndef INCLUDE_FLEXIBITY_JSONRPC_PLAYMETHOD_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_PLAYMETHOD_HPP_

#include <flexibity/jsonrpc/genericMethod.hpp>
#include <flexibity/log.h>
#include <sstream>
#include <map>
#include <flexibity/spawnUtil.hpp>
#include <sys/types.h>
#include <signal.h>

namespace Flexibity {

class audioPlayMethod: public genericMethod {
public:
	static constexpr const char* command = "audio_play";
	static constexpr const char* urlOpt = "url";

	audioPlayMethod() {
		ILOG_INIT();
	}

	virtual void invoke(
			  const Json::Value &params,
			  std::shared_ptr<Response> response) override {
		if (!response) {
			return; // if notification, do nothing
		}

		if (params[urlOpt].empty()) {
			response->sendError("missing url param");
			return;
		}

		std::stringstream mpg123;
		mpg123 << "mpg123";
		mpg123 << " " << params[urlOpt].asString();

		if (check_running()) {
			GINFO("PID: " << _running << " Running: ");
			response->sendError("play already running");
			return;
		} else {
			auto spawn_result = spawnUtil::mpg123(params[urlOpt].asString().c_str());
			pid_t pid = std::get<1>(spawn_result);
			_running = pid;

			std::thread play_thread([=](){
				int status = 0;
				waitpid(pid, &status,0);

				GINFO("WIFEXITED: " << WIFEXITED(status));
				GINFO("WIFSIGNALED: " << WIFSIGNALED(status));

				if (WIFEXITED(status) != 0) {
					response->sendError("Can't play audio");
					return;
				} /*else if (WIFSIGNALED(status) != 0) {
					// skip
				} */else {
					response->sendResult(responseOK());
					return;
				}
			});
			play_thread.detach();

			return;
		}

	}
	static pid_t _running;
private:
	bool check_running() {
		std::stringstream process;
		process << "/proc/" << _running << "/cmdline";
		std::ifstream lf(process.str());
		return lf.is_open();
	}
};

pid_t audioPlayMethod::_running;

}

#endif /* INCLUDE_FLEXIBITY_JSONRPC_PLAYMETHOD_HPP_ */

