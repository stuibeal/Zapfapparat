/*
 * zValve.h
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 *
 *      macht die Valve auf und zu
 *      checkt den Druck
 */

#ifndef ZVALVE_H_
#define ZVALVE_H_
#include "gemein.h"

#define VALVE_TEILER 91
#define PRESSURE_ZERO 115

class zValve
{
public:
  zValve ();
  virtual
  ~zValve ();
  void
  begin ();
  inline uint8_t
  getValveState ()
  {
    return state;
  }
  inline uint8_t
  getValveProzent ()
  {
    return valveProzentIst;
  }
  inline uint8_t
  getCleanPumpState ()
  {
    return cleanPumpState;
  }

  int
  getPressure ();
  void
  cleanPumpOn ();
  void
  cleanPumpOff ();
  void
  openValve ();
  void
  closeValve ();
  void
  setValveProzent (uint8_t prozent);
  void
  check();

  enum valveState
  {
    ZU, MACH_AUF, MACHT_AUF, ZWISCHENDRIN, GANZ_AUF, MACH_ZU, MACHT_ZU
  };

  enum pumpState
  {
    OFF, ON
  };

private:
  uint8_t state;
  uint8_t oldState;
  uint8_t cleanPumpState;
  unsigned long valveMillis;
  unsigned long runTime;
  int8_t valveProzentSoll;
  int8_t valveProzentIst;
  unsigned int druck;

  void open();
  void close();
  void off();


};

#endif /* ZVALVE_H_ */
