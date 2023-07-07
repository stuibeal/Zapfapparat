/*
 * tempsens.h
 *
 *  Created on: 18.06.2023
 *      Author: alfred3
 */

#ifndef TEMPSENS_H_
#define TEMPSENS_H_
#include "Arduino.h"
#include "Wire.h"
#include <OneWire.h> //alte Version nehmen!!!!!!!
#include "DS18B20.h"  //DS18B20_RT vom Rob Tilaart
#include "gemein.h"
// One Wire Temperatursensoren Pins
const uint8_t ONE_WIRE_BUS30 = 30;
const uint8_t ONE_WIRE_BUS32 = 32;
const uint8_t ONE_WIRE_BUS34 = 34;
const uint8_t ONE_WIRE_BUS35 = 35;
const uint8_t ONE_WIRE_BUS36 = 36;



class tempsens
{
public:
  tempsens ();
  virtual
  ~tempsens ();
  int16_t blockTemp;
  int16_t hausTemp;
  int16_t zulaufTemp;
  int16_t hahnTemp;
  int16_t kuehlwasserTemp;
  uint16_t getInVoltage();
  unsigned int getBlock1Temp();   //PT100 am Block aussen
  unsigned int getBlock2Temp();   //PT100 am Block innen
  void
  begin ();
  void
  request ();
  void
  requestBlock ();
  void
  requestVoltage();
  void
  checkAndSet ();

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

  unsigned int block1Temp;   //PT100 am Block aussen
  unsigned int block2Temp;   //PT100 am Block innen
  uint16_t inVoltage;
  unsigned int iDataGet(uint8_t befehl);


};

#endif /* TEMPSENS_H_ */
