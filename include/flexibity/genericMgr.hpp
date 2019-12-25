/*
 * genericMgr.hpp
 *
 *  Created on: Sept 14, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_GENERICMGR_HPP_
#define INCLUDE_FLEXIBITY_GENERICMGR_HPP_

#include <map>
#include <flexibity/log.h>
#include <functional>
#include <flexibity/exception.hpp>

namespace Flexibity{

using namespace std;

template <class T>
class genericMgr:
		public logInstanceName,
		public std::map<const std::string, T>{
public:

	INLINE_CLASS_EXCEPTION();

	static constexpr const char* nameOption = "name";
	static constexpr const char* itemCountOption = "size";
	static constexpr const char* itemsOption = "items";

	typedef T(*itemConstructor)(const Json::Value &cfg);

	genericMgr():_size(0){
		ILOG_INIT();
	}

	//TODO: refactor config
	void populateItems(const Json::Value& cfg, std::function<T (Json::Value &cfg)> itemConstructor){

		const string itemsSectionName (itemsOption);

		IDEBUG("populating items from: " << cfg.toStyledString());
		if(!cfg.isObject()){
			auto errStr = "manager object section is not JSON object in config: " + cfg.toStyledString();
			IERROR(errStr);
			throw exception(errStr);
		}

		if(!cfg.isMember(itemsSectionName)){
			auto errStr = "items section \"" + itemsSectionName +  "\" is not present in config: " + cfg.toStyledString();
			IERROR(errStr);
			throw exception(errStr);
		}

		auto itemsArr = cfg[itemsSectionName]; //TODO:: config validator
		if(!itemsArr.isArray()){
			auto errStr = "objects section \"" + itemsSectionName +  "\" is not an array in config: " + cfg.toStyledString();
			IERROR(errStr);
			throw exception(errStr);
		}

		_size = itemsArr.size();
		if(cfg.isMember(itemCountOption)){
			_size = cfg[itemCountOption].asUInt();

			if(_size == 0 || _size > itemsArr.size()){
				IWARN("Ignoring size option. Trying to add all items listed in config");
				_size = itemsArr.size();
			}
		}

		IDEBUG("found " << _size << " items in object config");

		size_t left = _size;
		for(auto& cCfg:itemsArr){
			auto item = itemConstructor(cCfg);
			if(item){
				_itemId ++;
				string name = item->instanceName();
				if(_appendId){
					name += std::to_string(_itemId);
				}
				auto result = this->insert({name, item});
				IINFO("Adding \"" << name << "\" to registry (append id policy: " << _appendId << ")");
				if(!result.second){
					auto errStr = "item name \"" + name + "\" is not unique";
					IERROR(errStr);
					throw exception(errStr);
				}
			}else
				IWARN("item " << _size - left << " was not constructed");
			if(--left == 0)
				break;
		}
	}
	T getItem(const std::string& name){
		auto i = this->find(name);
		if(i == this->end()){
			auto errStr = "item name \"" + name + "\" not found";
			IERROR(errStr);
			throw exception(errStr);
		}
		return i->second;
	}

	void setAppendId(bool enabled){
		_appendId = enabled;
	}

protected:
	Json::ArrayIndex _size;
	bool _appendId = false;
	uint32_t _itemId = 0;
};
}

#endif /* INCLUDE_FLEXIBITY_GENERICMGR_HPP_ */
