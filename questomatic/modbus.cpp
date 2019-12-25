#include "modbus.hpp"

#include <modbus.h>
#include <modbus-rtu.h>

#include <stdexcept>
#include <iostream>

#ifndef ILOG_INITSev
#define ILOG_INITSev(x)
#endif

#ifndef IWARN
#define IWARN(x) std::cout << x << std::endl
#endif

#ifndef IERROR
#define IERROR(x) std::cout << x << std::endl
#endif

Modbus::Modbus(const std::string& charDev):
_dev(charDev),
_ctx(NULL){
	ILOG_INITSev(INFO);

	auto ctx = modbus_new_rtu(_dev.c_str(), 9600, 'N', 8, 1);

	if(!ctx)
		throw std::runtime_error("Unable to create Modbus context");

	modbus_connect(ctx);

	modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
	modbus_set_response_timeout(ctx, 0, 500000); //500 ms
	//modbus_set_debug(ctx, 1);

	_ctx = ctx;

}

Modbus::~Modbus()
{
	auto ctx = static_cast<modbus_t*>(_ctx);
	modbus_close(ctx);
	modbus_free(ctx);
}

int Modbus::ReadRegisters(int slaveAddr, int addr, int nb, uint16_t *dest){
	auto retries = _defaultRetryCount;
	std::unique_lock<std::mutex> lck(_accessMutex);
	auto ctx = static_cast<modbus_t *>(_ctx);
	modbus_set_slave(ctx, slaveAddr);

	int readCount = -1;

	do{
		if(retries != _defaultRetryCount){
			IWARN("Retrying reading registers; slaveAddr: "  << slaveAddr << "; nb: " << nb << "; addr: " << addr  << "; readCount: " << readCount);
		}
		readCount = modbus_read_registers(ctx, addr, nb, dest);
	}while(readCount != nb && --retries > 0);

	if(retries <= 0){
		IERROR("Unable to read " << nb << " Registers " << "@" << addr << " from " << slaveAddr);
	}

	return readCount;
}


int Modbus::WriteRegister(int slaveAddr, int addr, int value) {
	auto retries = _defaultRetryCount;
	std::unique_lock<std::mutex> lck(_accessMutex);
	auto ctx = (modbus_t *) _ctx;
	modbus_set_slave(ctx, slaveAddr);
	int writeCount = -1;

	do{
		if(retries != _defaultRetryCount){
			IWARN("Retrying writing register; slaveAddr: "  << slaveAddr << "; addr: " << addr  << "; writeCount: " << writeCount);
		}
		writeCount =  modbus_write_register(ctx, addr, value);
	}while(writeCount != 1 && --retries > 0);

	if(retries <= 0){
		IERROR("Unable to write single register " << "@" << addr << " to " << slaveAddr);
	}

	return writeCount;
}

int Modbus::WriteRegisters(int slaveAddr, int addr, int nb, const uint16_t *src)
{
	auto retries = _defaultRetryCount;
	std::unique_lock<std::mutex> lck(_accessMutex);
	auto ctx = static_cast<modbus_t *>(_ctx);
	modbus_set_slave(ctx, slaveAddr);
	int writeCount = -1;

	do{
		if(retries != _defaultRetryCount){
			IWARN("Retrying writing registers; slaveAddr: "  << slaveAddr << "; addr: " << addr << "; nb: " << nb << "; writeCount: " << writeCount);

			if(writeCount == -1 )
				IWARN("MODBUS ERROR: " << modbus_strerror(errno));
		}
		writeCount =  modbus_write_registers(ctx, addr, nb, src);
	}while(writeCount != nb && --retries > 0);

	if(retries <= 0){
		IERROR("Unable to write multiple registers " << "@" << addr << " to " << slaveAddr);
	}

	return writeCount;
}


