#ifndef INCLUDE_FLEXIBITY_JSONRPC_UPDATEMETHOD_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_UPDATEMETHOD_HPP_

#include <flexibity/jsonrpc/genericMethod.hpp>
#include <flexibity/log.h>
#include <flexibity/spawnUtil.hpp>
#include <sstream>
#include <sys/mount.h>

namespace Flexibity{

using namespace Flexibity::spawnUtil;

class updateMethod:
		public genericMethod {
public:
	static constexpr const char* cmd_exec_script = "/opt/tigerlash/rootfs-updater-stage1.sh";

	static constexpr const char* command = "update_firmware";
	static constexpr const char* urlOpt = "url";
	static constexpr const char* bytesOpt = "bytes";
	static constexpr const char* md5Opt = "md5";

	updateMethod() {
		ILOG_INIT();
	}

	virtual void invoke(
		  const Json::Value &params,
		  std::shared_ptr<Response> response) override {
		if(!response){
			return; // if notification, do nothing
		}

		if (params[urlOpt].empty()) {
			response->sendError("missing url param");
			return;
		}

		auto new_params = params_from_url(params[urlOpt].asString());

		if(new_params[urlOpt].empty() || new_params[bytesOpt].empty() || new_params[md5Opt].empty()) {
			response->sendError("missing params");
			return;
		}

		u_int32_t blocks = 0;
		if ( (new_params[bytesOpt].asUInt() % 512) != 0)
		{
			response->sendError("invalid size");
			return;
		} else {
			blocks = new_params[bytesOpt].asUInt() / 512;
		}

		std::stringstream  check;
		check << "/opt/tigerlash/rootfs-updater-stage0.sh";
		check << " " << new_params[urlOpt].asString() << " " << blocks << " " << new_params[md5Opt].asString();

		auto check_result = run_cmd(check.str().c_str(), true);

		int status = std::get<0>(check_result);

		Json::Value err;
		err["code"] = status;

		switch(status) {
		case 1:
			err["message"] = "missing params";
			response->sendError(err);
			break;
		case 2:
			err["message"] = "image doesn't exist";
			response->sendError(err);
			break;
		case 3:
			err["message"] = "invalid size";
			response->sendError(err);
			break;
		case 4:
			err["message"] = "md5 checksum mismatch";
			response->sendError(err);
			break;
		default:
			break;
		}

		if (status == 0)
		{
			mount(nullptr, "/tmp", nullptr, MS_REMOUNT | ~MS_NOEXEC, nullptr);
			run_cmd(cmd_exec_script, true);

			Json::Value v;
			response->sendResult(responseOK(v));
		}
	}
private:
	std::vector<std::string> split_string(const std::string& str, char delimeter) {
		std::istringstream ss(str);
		std::string token;

		std::vector<std::string> el;
		while(std::getline(ss, token, delimeter)) {
			el.push_back(token);
		}

		return el;
	}

	Json::Value params_from_url(const std::string& url) {
		Json::Value params;
		auto url_parts = split_string(url, '/');
		if (url_parts.size() > 1) {
			auto name_parts = split_string(url_parts[url_parts.size()-1], '-');

			params[urlOpt] = url;

			if (name_parts.size() > 4 ) {
				params[bytesOpt] = std::stoi(name_parts[3]);
				name_parts[4].erase(name_parts[4].find(".img"));
				params[md5Opt] = name_parts[4];
			}
		}
		return params;
	}
};

}

#endif /* INCLUDE_FLEXIBITY_JSONRPC_UPDATEMETHOD_HPP_ */
