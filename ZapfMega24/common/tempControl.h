/*
 * tempControl.h
 *
 *  Created on: 26.06.2024
 *      Author: stuibeal
 *
 *  - Kommunikation mit dem zTempControl STM32 uC
 *  - DS18B20 Sensoren am Arduino
 *
 */

#ifndef COMMON_TEMPCONTROL_H_
#define COMMON_TEMPCONTROL_H_

#include "Arduino.h"
#include "string.h"
#include "Wire.h"
#include "gemein.h"
#include "stdint.h"
#include <OneWire.h> //alte Version nehmen!!!!!!!
#include "DS18B20.h"  //DS18B20_RT vom Rob Tilaart

// One Wire Temperatursensoren Pins
#define ONE_WIRE_BUS30  30
#define ONE_WIRE_BUS32  32
#define ONE_WIRE_BUS34  34
#define ONE_WIRE_BUS35  35
#define ONE_WIRE_BUS36  36

//zTempControl uC
#define TEMP_CONTROL_I2C_ADDR 0x13
#define TEMP_CONTROL_TX_BYTES 8
#define TEMP_CONTROL_RX_BYTES 12
#define JUST_COMMUNICATE 0x01
#define SET_TEMPERATUR 0x02


//#define BATT_ULTRAHIGH 0x0
//#define BATT_HIGH 0x01
//#define BATT_NORMAL 0x02
//#define BATT_LOW 0x03
//#define BATT_ULTRALOW 0x04



class tempControl {
public:
	tempControl();
	virtual ~tempControl();
	void
	begin();

	//DS18B20
	void
	requestSensors(void);

	//zTempControl i2C
	void
	holeDaten(void);
	/**
	 * @brief 			Sendet Befehlt an den zTempControl STM32 uC
	 * @param befehl	Was für ein Befehl uint8_t
	 * @param daten		Eventuelle Datenübergabe in uint16_t
	 */
	void
	sendeBefehl(uint8_t befehl, uint16_t daten);

	//Datenausgabe
	inline int16_t getBlockInnenTemp(void) {
		return blockInnenTemp;
	}
	;
	inline int16_t getBlockAussenTemp(void) {
		return blockAussenTemp;
	}
	;
	inline uint8_t getBatterieStatus(void) {
		return batterieStatus;
	}
	;
	inline uint16_t getKuehlWasserFlowRate(void) {
		return kuehlWasserFlowRate;
	}
	;
	inline uint16_t getStromVerbrauchAktuell(void) {
		return stromVerbrauchAktuell;
	}
	;
	inline uint16_t getStromVerbrauchLetzteZapfung(void) {
		return stromVerbrauchLetzteZapfung;
	}
	;
	inline int16_t getDSblockTemp(void) {
		return blockTemp;
	}
	;
	inline int16_t getHausTemp(void) {
		return hausTemp;
	}
	;
	inline int16_t getZulaufTemp(void) {
		return zulaufTemp;
	}
	;
	inline int16_t getHahnTemp(void) {
		return hahnTemp;
	}
	;
	inline int16_t getKuehlWasserTemp(void) {
		return kuehlwasserTemp;
	}
	;

private:
	//Daten vom zTempControl uC
	int16_t blockAussenTemp;
	int16_t blockInnenTemp;  //kann auch minus sein du honk
	uint8_t batterieStatus;
	uint8_t eingangsSpannung;
	uint16_t kuehlWasserFlowRate;
	uint8_t aRxBuffer[TEMP_CONTROL_RX_BYTES];
	uint8_t aTxBuffer[TEMP_CONTROL_TX_BYTES];
	uint16_t stromVerbrauchAktuell;
	uint16_t stromVerbrauchLetzteZapfung;
	void
	convertTxData(void);
	void
	convertRxData(void);
	void
	i2cTransfer(void);

	//Daten von den DS18B20 Sensoren am Arduino
	OneWire *oneWire30;
	OneWire *oneWire32;
	OneWire *oneWire34;
	OneWire *oneWire35;
	OneWire *oneWire36;

	DS18B20 *auslauf;
	DS18B20 *block;
	DS18B20 *gehaeuse;
	DS18B20 *zulauf;
	DS18B20 *kuehlwasser;

	int16_t blockTemp;
	int16_t hausTemp;
	int16_t zulaufTemp;
	int16_t hahnTemp;
	int16_t kuehlwasserTemp;

};

#endif /* COMMON_TEMPCONTROL_H_ */
