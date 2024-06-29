/*
 * tempControl.cpp
 *
 *  Created on: 26.06.2024
 *      Author: stuibeal
 */

#include "tempControl.h"

tempControl::tempControl() {
	auslauf = nullptr;
	block = nullptr;
	gehaeuse = nullptr;
	zulauf = nullptr;
	kuehlwasser = nullptr;

	oneWire30 = nullptr;
	oneWire32 = nullptr;
	oneWire34 = nullptr;
	oneWire35 = nullptr;
	oneWire36 = nullptr;

	blockTemp = 0;
	hausTemp = 0;
	zulaufTemp = 0;
	hahnTemp = 0;
	kuehlwasserTemp = 0;

	blockInnenTemp = 0;
	blockAussenTemp = 0;
	batterieStatus = 0x02;
	eingangsSpannung = 120;
	kuehlWasserFlowRate = 0;
	stromVerbrauchAktuell = 0;
	stromVerbrauchLetzteZapfung = 0;
}

tempControl::~tempControl() {
	/*
	 * nothing
	 */
}

void tempControl::begin() {
	oneWire30 = new OneWire(ONE_WIRE_BUS30);
	oneWire32 = new OneWire(ONE_WIRE_BUS32);
	oneWire34 = new OneWire(ONE_WIRE_BUS34);
	oneWire35 = new OneWire(ONE_WIRE_BUS35);
	oneWire36 = new OneWire(ONE_WIRE_BUS36);

	auslauf = new DS18B20(oneWire30);
	block = new DS18B20(oneWire32);
	gehaeuse = new DS18B20(oneWire34);
	zulauf = new DS18B20(oneWire35);
	kuehlwasser = new DS18B20(oneWire36);

	//Temperaturfuehler hochfahren
	auslauf->begin();
	gehaeuse->begin();
	block->begin();
	zulauf->begin();
	kuehlwasser->begin();

	// Fühlerauflösung einstellen
	auslauf->setResolution(11);
	gehaeuse->setResolution(8);
	block->setResolution(10);
	zulauf->setResolution(10);
	kuehlwasser->setResolution(8);
	//Schonmal Temperatur anfordern
	auslauf->requestTemperatures();
	gehaeuse->requestTemperatures();
	block->requestTemperatures();
	zulauf->requestTemperatures();
	kuehlwasser->requestTemperatures();

}

//DS18B20
void tempControl::requestSensors(void) {
	if (auslauf->isConversionComplete()) {
		hahnTemp = static_cast<int16_t>(auslauf->getTempC() * 100);
		auslauf->requestTemperatures();
	}

	if (gehaeuse->isConversionComplete()) {
		hausTemp = static_cast<int16_t>(gehaeuse->getTempC() * 100);
		gehaeuse->requestTemperatures();
	}
	if (zulauf->isConversionComplete()) {
		zulaufTemp = static_cast<int16_t>(zulauf->getTempC() * 100);
		zulauf->requestTemperatures();
	}
	if (block->isConversionComplete()) {
		blockTemp = static_cast<int16_t>(block->getTempC() * 100);
		block->requestTemperatures();
	}
	if (kuehlwasser->isConversionComplete()) {
		kuehlwasserTemp = static_cast<int16_t>(kuehlwasser->getTempC() * 100);
		kuehlwasser->requestTemperatures();
	}

}

//zTempControl i2C
void tempControl::holeDaten(void) {
	aTxBuffer[0] = JUST_COMMUNICATE;
	convertTxData();
	i2cTransfer();
	convertRxData();
}

void tempControl::sendeBefehl(uint8_t befehl, uint16_t daten) {
	aTxBuffer[0] = befehl;
	aTxBuffer[6] = lowByte(daten); //STM32 erwartet in Little Endian
	aTxBuffer[7] = highByte(daten);
	convertTxData();
	i2cTransfer();
	convertRxData();
}

void tempControl::convertTxData(void) {
	//TODO maybe eine schnellere Funktion schreiben
	/*   *  Der Temperaturregler erwartet immer folgende Daten vom Master:
	 *  - 1 Byte: was tun (0x01 - nothing, 0xF9-0xFF do something, switch something) aRxBuffer[0]
	 *  - 1 Byte: nothing (dann kann man evtl den pointer auf ein uint16_t array dingsen)
	 *  - 2 Bytes: Temperatur Einlauf -> wärmer -> mehr stoff geben aRxBuffer[1-2]
	 *  - 2 Bytes: Temperatur Kühlkreislauf -> zu hot: runterregeln! aRxBuffer [3-4]
	 *  - 2 Bytes: Sonstige Daten aRxBuffer [5-6]
	 */

	aTxBuffer[1] = 0x00;
	aTxBuffer[2] = lowByte(zulaufTemp);
	aTxBuffer[3] = highByte(zulaufTemp);
	aTxBuffer[4] = lowByte(kuehlwasserTemp);
	aTxBuffer[5] = highByte(kuehlwasserTemp);
}

void tempControl::convertRxData(void) {
	/*
	 * Der Temperaturregler schickt _immer_ folgendes zurück
	 *  - 2 Bytes: Temperatur Block Aussen INT16_T
	 *  - 2 Bytes: Temperatur Block Innen INT16_T
	 *  - 2 Bytes: 1 Byte Batteriestatus (0x01 high, 0x02 normal, 0x03 low, 0x04 ultralow), 1 Byte InVoltage *10
	 *  - 2 Bytes: Aktueller Stromverbrauch in W
	 *  - 2 Bytes: Stromverbrauch in Wh letzte Zapfung (zwischen 2 Zapfende)
	 *  - 2 Bytes: Flowrate Kühlwasser
	 *
	 *  LITTLE ENDIAN, MY FRIEND!
	 *
	 **  Die Daten werden LITTLE ENDIAN übertragen. also aus uint16_t 0xAABB wird:
	 *  Übertragung: [0]0xBB [1]0xAA

	 */
	blockAussenTemp = (aRxBuffer[1] << 8) + aRxBuffer[0];
	blockInnenTemp = (aRxBuffer[3] << 8) + aRxBuffer[2];
	batterieStatus = aRxBuffer[4];
	eingangsSpannung = aRxBuffer[5];
	stromVerbrauchAktuell = (aRxBuffer[7] << 8) + aRxBuffer[6];
	stromVerbrauchLetzteZapfung = (aRxBuffer[9] << 8) + aRxBuffer[8];
	kuehlWasserFlowRate = (aRxBuffer[11] << 8) + aRxBuffer[10];
}

void tempControl::i2cTransfer(void) {
	Wire.beginTransmission(TEMP_CONTROL_I2C_ADDR); // transmit to device #18
	for (uint8_t i = 0; i < TEMP_CONTROL_TX_BYTES; i++) {
		Wire.write(aTxBuffer[i]);        // send it
	}
	Wire.endTransmission();    // stop transmitting

	Wire.requestFrom(TEMP_CONTROL_I2C_ADDR, TEMP_CONTROL_RX_BYTES);
	uint8_t i = 0;
	while (Wire.available() && i < TEMP_CONTROL_RX_BYTES) {
		aRxBuffer[i] = Wire.read(); // receive a byte as character
		i++;
	}

}
