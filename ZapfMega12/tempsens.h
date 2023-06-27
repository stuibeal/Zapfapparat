/*
 * tempsens.h
 *
 *  Created on: 18.06.2023
 *      Author: alfred3
 */

#ifndef TEMPSENS_H_
#define TEMPSENS_H_
#include "Arduino.h"
#include <OneWire.h> //alte Version nehmen!!!!!!!
#include "DS18B20.h"  //DS18B20_RT vom Rob Tilaart
#include "gemein.h"


class tempsens {
public:
	tempsens();
	virtual ~tempsens();
	int16_t blockTemp;
	int16_t hausTemp;
	int16_t zulaufTemp;
	int16_t hahnTemp;
	int16_t kuehlwasserTemp;
	int16_t block1Temp;
	int16_t block2Temp;
	void begin();
	void request();
	void checkAndSet();

private:
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

	//OneWire oneWire30(uint8_t ONE_WIRE_BUS30);

	//OneWire oneWire32(uint8_t ONE_WIRE_BUS32);
	//OneWire oneWire34(uint8_t ONE_WIRE_BUS34);
	//OneWire oneWire35(uint8_t ONE_WIRE_BUS35);
	//OneWire oneWire36(uint8_t ONE_WIRE_BUS36);


	// DS18B20 Temperatursensoren
	//DS18B20 auslauf(OneWire &);
	//DS18B20 auslauf(const DS18B20 *);
	//DS18B20 block(const DS18B20 *);
	//DS18B20 gehaeuse(const DS18B20 *);
	//DS18B20 zulauf(const DS18B20 *);
	//DS18B20 kuehlwasser(const DS18B20 *);


};

#endif /* TEMPSENS_H_ */
