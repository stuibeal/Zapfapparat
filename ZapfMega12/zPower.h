/*
 * zPower.h
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 *
 *      Checkt die Eingangsspannung
 *      Wenn zu gering -> Lampen etc aus
 *      Wenn wieder höher -> Lampen etc wieder an
 *      Schickt den uC in Sleepmode + weckt wieder alles auf
 *
 *
 */

#ifndef ZPOWER_H_
#define ZPOWER_H_
#include "gemein.h"
#include "Wire.h"

class zPower
{
public:
  zPower ();
  virtual
  ~zPower ();
  inline uint8_t
  getState ()
  {
    return powerState;
  }
  inline void
  setState (uint8_t state)
  {
    powerState = state;
  }
  enum states
  {
    NORMAL, LOW_POWER, GEH_SCHLAFEN, SCHLAEFT, WACH_AUF
  };
  void check();
  void setLed(uint8_t offon);



private:
  int lastVoltage;
  int helligkeit;
  uint8_t powerState;
  static unsigned long millisSeitZapfEnde;
  static unsigned long millisSeitLetztemCheck;

};

#endif /* ZPOWER_H_ */
