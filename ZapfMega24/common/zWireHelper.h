/*
 * zWireHelper.h
 *
 *  Created on: 21.07.2023
 *      Author: Alfred3
 */

#ifndef COMMON_ZWIREHELPER_H_
#define COMMON_ZWIREHELPER_H_

#define FLOW_I2C_ADDR  0x12  // Flowzähl uC (in Cube 1 bit linksshiften!)
#define FLOW_I2C_ANTWORTBYTES  2 // die menge an Antwortbytes

#include "stdint.h"
#include "string.h"

class zWireHelper {
public:
	zWireHelper();
	virtual
	~zWireHelper();
	void
	initialise();
	void
	flowDataSend(uint8_t befehl, uint16_t wert);
	void
	flowDataSend(uint8_t befehl, uint8_t option1, uint8_t option2);
	inline uint16_t getMilliliter() {
		return zapfMillis;
	}
	uint16_t getFreshZapfMillis();

private:
	uint8_t aRxBuffer[14];
	uint8_t aTxBuffer[14];
	uint16_t zapfMillis;
};

#endif /* COMMON_ZWIREHELPER_H_ */
