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
  getPowerState ()
  {
    return powerState;
  }
  inline uint8_t
  getMachineState ()
  {
    return machineState;
  }
  inline void
  setState (uint8_t state)
  {
    powerState = state;
  }

  void check();
  void setLed(uint8_t offon);




private:
  enum powerState
    {
      TOO_LOW_POWER, LOW_POWER, MID_POWER, HIGH_POWER, TOO_HIGH_POWER
    };
    enum machineState
    {
      SLEEP, WAKE_UP, WORK, STANDBY, GO_SLEEP
    };

  int lastVoltage;
  int helligkeit;
  uint8_t powerState;
  uint8_t machineState;
  static unsigned long millisSeitZapfEnde;
  static unsigned long millisSeitLetztemCheck;



};

#endif /* ZPOWER_H_ */
