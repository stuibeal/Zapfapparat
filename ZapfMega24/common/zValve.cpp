/*
 * zValve.cpp
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 */

#include "zValve.h"

zValve::zValve ()
{
  valveMillis = 0;
  oldState = ZU;
  state = MACH_AUF;
  runTime = 0;
  cleanPumpState = OFF;
  druck = 0;
  valveProzentSoll = 100;
  valveProzentIst = 0;
}

zValve::~zValve ()
{
  // Auto-generated destructor stub
}
void
zValve::begin ()
{
  pinMode (VALVE_AUF_PIN, OUTPUT);
  digitalWrite (VALVE_AUF_PIN, LOW);
  pinMode (VALVE_ZU_PIN, OUTPUT);
  digitalWrite (VALVE_ZU_PIN, LOW);
  pinMode (CLEAN_PUMP_PIN, OUTPUT);
  digitalWrite (CLEAN_PUMP_PIN, LOW);
  pinMode (PRESSURE_SENS_PIN, INPUT);
  valveMillis = millis();
}
/*
 * Druck
 * atue		wert
 * 0	120
 * 1	270
 * 1,2	290
 * 1,4	321
 * 1,6	350
 * 1,8	380
 * 2	405
 * 2,2	440
 * 2,4	470
 * 2,5	480
 * 3	552
 */
/**
 * @brief  Liest den Drucksensor aus und gibt das Ergebnis zurück
 * @return Druck in ATÜ * 100
 */
int
zValve::getPressure ()
{
  druck = analogRead (PRESSURE_SENS_PIN) - PRESSURE_ZERO;
  druck = druck / 1.46; //dann hat man das ergebnis in atü * 100
  return druck;
}

/* @brief  Hinterer Ausgang wird mit 12V versorgt für Reinigungspumpe
 * 	   Klinkenanschluss 6.3mm
 */
void
zValve::cleanPumpOn ()
{
  cleanPumpState = ON;
  if (state != GANZ_AUF) {
      valveProzentSoll = 100;
      state = MACH_AUF;
      check();
  }
  digitalWrite (CLEAN_PUMP_PIN, cleanPumpState);
}

/** @brief  Hinterer Ausgang wird abgeschaltet
 * 	    Das Ventil soll(!) offen bleiben, damit das ganze dann
 * 	    evtl mit CO2 durchgeblasen werden kann.
 */
void
zValve::cleanPumpOff ()
{
  cleanPumpState = OFF;
  digitalWrite (CLEAN_PUMP_PIN, cleanPumpState);
}

void
zValve::openValve ()
{
  valveProzentSoll = 100;
  state = MACH_AUF;
  check ();
}

void
zValve::closeValve ()
{
  valveProzentSoll = 0;
  state = MACH_ZU;
  check ();
}

void
zValve::setValveProzent (uint8_t prozent)
{
  valveProzentSoll = prozent;
  if (valveProzentIst < valveProzentSoll)
    {
      state = MACH_AUF;
    }
  else if (valveProzentIst > valveProzentSoll)
    {
      state = MACH_ZU;
    }
  check ();
}

void
zValve::check ()
{
  if (valveProzentSoll != valveProzentIst)
    {
      while (oldState != state)
	{
	  oldState = state;
	  switch (state)
	    {
	    case ZU:
	      runTime = 0; //setzt die Runtime wieder zurück auf 0 -> 0%
	      valveProzentIst = 0;
	      break;
	    case MACH_AUF:
	      open ();
	      valveMillis = millis ();
	      state++;
	      break;
	    case MACHT_AUF:
	      // braucht 9s von auf nach zu. teilen wir durch etwas mehr für sicher auf/zu.
	      runTime += millis () - valveMillis;
	      valveMillis = millis ();
	      valveProzentIst = int8_t (runTime / VALVE_TEILER);
	      if (valveProzentIst >= valveProzentSoll)
		{
		  off ();
		  state = ZWISCHENDRIN;
		}
	      break;
	    case ZWISCHENDRIN:
	      if (valveProzentIst >= 100)
		{
		  state = GANZ_AUF;
		}
	      if (valveProzentIst <= 0)
		{
		  state = ZU;
		}
	      break;
	    case GANZ_AUF:
	      valveProzentIst = 100;
	      runTime = 0;
	      break;
	    case MACH_ZU:
	      close ();
	      valveMillis = millis ();
	      state++;
	      break;
	    case MACHT_ZU:
	      // braucht 9s von auf nach zu. teilen wir durch etwas mehr für sicher auf/zu.
	      runTime += millis () - valveMillis;
	      valveMillis = millis ();
	      valveProzentIst = 100 - int8_t (runTime / VALVE_TEILER);
	      if (valveProzentIst <= valveProzentSoll)
		{
		  off ();
		  state = ZWISCHENDRIN;
		}
	      break;
	    }
	}

    }
}

void
zValve::open ()
{
  digitalWrite (VALVE_ZU_PIN, LOW);
  digitalWrite (VALVE_AUF_PIN, HIGH);
}

void
zValve::close ()
{
  digitalWrite (VALVE_AUF_PIN, LOW);
  digitalWrite (VALVE_ZU_PIN, HIGH);
}

void
zValve::off ()
{
  digitalWrite (VALVE_ZU_PIN, LOW);
  digitalWrite (VALVE_AUF_PIN, LOW);
}
