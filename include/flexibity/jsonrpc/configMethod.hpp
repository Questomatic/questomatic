#ifndef INCLUDE_FLEXIBITY_JSONRPC_CONFIGMETHOD_HPP_
#define INCLUDE_FLEXIBITY_JSONRPC_CONFIGMETHOD_HPP_

#include <flexibity/jsonrpc/genericMethod.hpp>
#include <flexibity/serialPortMgr.hpp>
#include <flexibity/log.h>
#include <sstream>
#include <flexibity/mountUtil.hpp>

namespace Flexibity{

class configMethod: public genericMethod {
public:
		static constexpr const char* command = "update_config";
		static constexpr const char* configFile = "/etc/tigerlash/tigerlash_portmanager.json";
		static constexpr const char* fromDev = "/dev/mmcblk0p3";
		static constexpr const char* toPath = "/etc/tigerlash";

		configMethod(serialPortMgr::sPtr pm) :_pm(pm) {
				ILOG_INIT();
				mountUtil::mount(fromDev, toPath);
				auto config = read_config();
				IINFO(config.toStyledString());
				try {
					apply_config(config["portManager"]);
				}
				catch(boost::system::system_error &e) {
					IERROR("Excpetion: " << e.what());
				}
		}

		virtual void invoke(
				  const Json::Value &params,
				  std::shared_ptr<Response> response) override {
			if(!response){
					return; // if notification, do nothing
			}

			if ( params.isMember("items") && params["items"].isArray() ) {
				IWARN(params.toStyledString());

				mountUtil::safe_remount(toPath, true);

				try {
					apply_config(params);
				}
				catch(boost::system::system_error& e) {
					response->sendError(e.what());
					mountUtil::safe_remount(toPath, false);
					return;
				}

				write_config(params);
				mountUtil::safe_remount(toPath, false);
				response->sendResult(responseOK());
			}
		}
private:


		Json::Value read_config() {
			std::ifstream config(configFile);
			Json::Reader reader;
			Json::Value root;
			if (config.is_open()) {
				bool parsedSuccess = reader.parse(config, root, false);
				if(!parsedSuccess) {

					IFATAL("config parse error");
					IFATAL(reader.getFormattedErrorMessages());
					IFATAL(root.toStyledString());
					return Json::Value(Json::nullValue);
				}
			} else {
				IWARN("Config '" << configFile << "' doesn't exist");
				return Json::Value(Json::nullValue);
			}
			config.close();
			return root;
		}

		void write_config(const Json::Value &val) {
			std::ofstream config(configFile);

			if(config.is_open()) {
				Json::Value root_obj;
				root_obj["portManager"] = val;

				Json::StyledWriter styledWriter;
				config << styledWriter.write(root_obj);
			}

			config.close();
		}

		bool apply_config(const Json::Value &value) {
			auto items = value["items"];

			for(auto item: items) {
				if ( item.isMember("name") ) {
					auto name = item.removeMember("name").asString();
					auto serial_by_name = _pm->getItem(name);

					serial_by_name->setOptions(item);
				}
				else {
					return false;
				}
			}
			return true;
		}

private:
	serialPortMgr::sPtr _pm;
};

}

#endif /* INCLUDE_FLEXIBITY_JSONRPC_CONFIGMETHOD_HPP_ */

