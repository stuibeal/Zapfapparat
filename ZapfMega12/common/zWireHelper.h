/*
 * zWireHelper.h
 *
 *  Created on: 21.07.2023
 *      Author: Alfred3
 */

#ifndef COMMON_ZWIREHELPER_H_
#define COMMON_ZWIREHELPER_H_

#include <Wire.h>
#include "gemein.h"
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

private:
	uint8_t aRxBuffer[14];
	uint8_t aTxBuffer[14];
	uint16_t zapfMillis;
};

#endif /* COMMON_ZWIREHELPER_H_ */
