/*
 * zWireHelper.cpp
 *
 *  Created on: 21.07.2023
 *      Author: Alfred3
 */

#include "zWireHelper.h"
#include <Wire.h>
#include "gemein.h"
#include <Arduino.h>

zWireHelper::zWireHelper() {
	aRxBuffer[0] = 0;
	aRxBuffer[1] = 0;
	aTxBuffer[0] = 0;
	aTxBuffer[1] = 0;
	zapfMillis = 0;
}

zWireHelper::~zWireHelper() {
}

void zWireHelper::initialise() {
//nothing to do?
}

/**
 * @brief Methode um einen Befehl und einen integer an den Flowmeter zu schicken
 * 	  der Flowmeter uC gibt immer die aktuellen Milliliter zurück
 *
 * @param befehl	siehe Liste
 * @param wert		integer
 * @return		milliliter
 */
void zWireHelper::flowDataSend(uint8_t befehl, uint16_t wert) {
	aTxBuffer[0] = highByte(wert);
	aTxBuffer[1] = lowByte(wert);

	Wire.beginTransmission(FLOW_I2C_ADDR); // schicke Daten an den Flow
	Wire.write(befehl);        // mach Das du stück
	Wire.write(aTxBuffer[0]); // optionen siehe communication.h
	Wire.write(aTxBuffer[1]); //
	Wire.endTransmission();    // stop transmitting

	Wire.requestFrom(FLOW_I2C_ADDR, FLOW_I2C_ANTWORTBYTES); // Daten vom Flow müssen immer geholt werden
	while (Wire.available()) { // wenn Daten vorhanden, hol die Dinger
		aRxBuffer[0] = Wire.read(); // ein Byte als char holen
		aRxBuffer[1] = Wire.read(); // zweites Byte als char holen
	}
	zapfMillis = (aRxBuffer[0] << 8) + aRxBuffer[1]; // da der Flow immer die aktuellen ml ausgibt kann man die gleich in die Variable schreiben
}

/**
 * @brief Methode zum Senden eines Befehls mit zwei Optionen zum Flowmeter uC
 *
 * @param befehl	Siehe Liste, was soll er tun 0x00 - 0xFF
 * @param option1	Siehe Liste
 * @param option2	Siehe Liste
 * @return		milliliter
 */
void zWireHelper::flowDataSend(uint8_t befehl, uint8_t option1,
		uint8_t option2) {
	Wire.beginTransmission(FLOW_I2C_ADDR); // schicke Daten an den Flow
	Wire.write(befehl);        // mach Das du stück
	Wire.write(option1); // optionen siehe communication.h
	Wire.write(option2); //
	Wire.endTransmission();    // stop transmitting

	Wire.requestFrom(FLOW_I2C_ADDR, FLOW_I2C_ANTWORTBYTES); // Daten vom Flow müssen immer geholt werden
	while (Wire.available()) { // wenn Daten vorhanden, hol die Dinger
		aRxBuffer[0] = Wire.read(); // ein Byte als char holen
		aRxBuffer[1] = Wire.read(); // zweites Byte als char holen
	}
	zapfMillis = (aRxBuffer[0] << 8) + aRxBuffer[1]; // da der Flow immer die aktuellen ml ausgibt kann man die gleich in die Variable schreiben
}

uint16_t zWireHelper::getFreshZapfMillis() {
	flowDataSend(GET_ML, 0, 0);
	return zapfMillis;
}

