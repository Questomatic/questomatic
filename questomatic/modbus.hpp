/*
 * modbus.hpp
 *
 *  Created on: Jul 10, 2018
 *      Author: rs
 */

#ifndef ORANGE_CONTROLLER_MODBUS_HPP_
#define ORANGE_CONTROLLER_MODBUS_HPP_

#include <string>
#include <stdint.h>
#include <mutex>
#include <memory>
//#include <flexibity/log.h>

//using namespace Flexibity;


class Modbus
//		: public logInstanceName
{
public:
	Modbus(const std::string& charDev);
	~Modbus();
	typedef std::shared_ptr<Modbus> sPtr;
	int ReadRegisters(int slaveAddr, int addr, int nb, uint16_t *dest);
	int WriteRegister(int slaveAddr, int addr, int value);
	int WriteRegisters(int slaveAddr, int addr, int nb, const uint16_t *src);

protected:
	std::string _dev;
	std::mutex _accessMutex;
	void *_ctx;
	int _defaultRetryCount = 10;
};


#endif /* ORANGE_CONTROLLER_MODBUS_HPP_ */
