/*
 * tempsens.cpp
 *
 *  Created on: 18.06.2023
 *      Author: alfred3
 */

#include "tempsens.h"

tempsens::tempsens ()
{
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
  block1Temp = 0;
  block2Temp = 0;
  inVoltage = 0;
}

tempsens::~tempsens ()
{
  // TODO Auto-generated destructor stub
}

void
tempsens::begin ()
{
  oneWire30 = new OneWire (ONE_WIRE_BUS30);
  oneWire32 = new OneWire (ONE_WIRE_BUS32);
  oneWire34 = new OneWire (ONE_WIRE_BUS34);
  oneWire35 = new OneWire (ONE_WIRE_BUS35);
  oneWire36 = new OneWire (ONE_WIRE_BUS36);

  auslauf = new DS18B20 (oneWire30);
  block = new DS18B20 (oneWire32);
  gehaeuse = new DS18B20 (oneWire34);
  zulauf = new DS18B20 (oneWire35);
  kuehlwasser = new DS18B20 (oneWire36);

  //OneWire oneWire30(ONE_WIRE_BUS30);
  //DS18B20 auslauf(&oneWire30);

  //OneWire oneWire32(ONE_WIRE_BUS32);
  //OneWire oneWire34(ONE_WIRE_BUS34);
  //OneWire oneWire35(ONE_WIRE_BUS35);
  //OneWire oneWire36(ONE_WIRE_BUS36);

  // DS18B20 Temperatursensoren

  //DS18B20 block(&oneWire32);
  //DS18B20 gehaeuse(&oneWire34);
  //DS18B20 zulauf(&oneWire35);
  //DS18B20 kuehlwasser(&oneWire36);

  //Temperaturfuehler hochfahren
  auslauf->begin ();
  gehaeuse->begin ();
  block->begin ();
  zulauf->begin ();
  kuehlwasser->begin ();

  // Fühlerauflösung einstellen
  auslauf->setResolution (11);
  gehaeuse->setResolution (8);
  block->setResolution (10);
  zulauf->setResolution (10);
  kuehlwasser->setResolution (8);
  //Schonmal Temperatur anfordern
  auslauf->requestTemperatures ();
  gehaeuse->requestTemperatures ();
  block->requestTemperatures ();
  zulauf->requestTemperatures ();
  kuehlwasser->requestTemperatures ();

}

void
tempsens::request ()
{

  if (auslauf->isConversionComplete ())
    {
      hahnTemp = static_cast<int16_t> (auslauf->getTempC () * 100);
      auslauf->requestTemperatures ();
    }

  if (gehaeuse->isConversionComplete ())
    {
      hausTemp = static_cast<int16_t> (gehaeuse->getTempC () * 100);
      gehaeuse->requestTemperatures ();
    }
  if (zulauf->isConversionComplete ())
    {
      zulaufTemp = static_cast<int16_t> (zulauf->getTempC () * 100);
      zulauf->requestTemperatures ();
    }
  if (block->isConversionComplete ())
    {
      blockTemp = static_cast<int16_t> (block->getTempC () * 100);
      block->requestTemperatures ();
    }
  if (kuehlwasser->isConversionComplete ())
    {
      kuehlwasserTemp = static_cast<int16_t> (kuehlwasser->getTempC () * 100);
      kuehlwasser->requestTemperatures ();
    }

}

void
tempsens::checkAndSet ()
{
  //dunno
}

unsigned int
tempsens::getBlock1Temp ()
{
  block1Temp = iDataGet (GET_BLOCK_TEMP);
  return block1Temp;
}

unsigned int
tempsens::getBlock2Temp ()
{
  block2Temp = iDataGet (GET_AUSLAUF_TEMP);
  return block1Temp;
}

uint16_t
tempsens::getInVoltage ()
{
  inVoltage = iDataGet (GET_IN_VOLTAGE);
  return inVoltage;
}

unsigned int
tempsens::iDataGet (uint8_t befehl)
{
  const uint8_t howManyBytes = 2;
  uint8_t recieveByte[2];

  Wire.beginTransmission (TEMP_I2C_ADDR); // transmit to device #18
  Wire.write (befehl);        // hol mir die Millis
  Wire.endTransmission ();    // stop transmitting

  Wire.requestFrom (TEMP_I2C_ADDR, howManyBytes); // request 2 bytes from slave device
  while (Wire.available ())   // slave may send less than requested
    {
      recieveByte[0] = Wire.read (); // receive a byte as character
      recieveByte[1] = Wire.read (); // receive a byte as character
    }

  return (recieveByte[0] << 8) + recieveByte[1];
}

