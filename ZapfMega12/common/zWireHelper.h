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

class zWireHelper
{
public:
  zWireHelper ();
  virtual
  ~zWireHelper ();
  void
  initialise ();
  void
  iBefehl (uint8_t empfaenger, uint8_t befehl);
  void
  iDataSend (byte empfaenger, byte befehl, unsigned int sendedaten);
  void
  i2cIntDataSend (byte empfaenger, byte befehl, unsigned int sendedaten);
  void
  flowDataSend (uint8_t befehl, uint16_t wert);
  void
  flowDataSend (uint8_t befehl, uint8_t option1, uint8_t option2);
  inline uint16_t
  getMilliliter ()
  {
    return zapfMillis;
  }

private:
  uint8_t aRxBuffer[3];
  uint8_t aTxBuffer[3];
  uint16_t zapfMillis;
};

#endif /* COMMON_ZWIREHELPER_H_ */
