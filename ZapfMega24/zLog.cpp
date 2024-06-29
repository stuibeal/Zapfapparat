/*
 * zLog.cpp
 *
 *  Created on: 01.07.2023
 *      Author: Alfred3
 */

#include "zLog.h"

zLog::zLog ()
{
  DateTime dateTime = DateTime (0, 1, 1, SAMSDA, 0, 0, 0);
  _sd = nullptr;
  _user = nullptr;
  _temp = nullptr;
  _buf = nullptr;
  logState = 0;

}

zLog::~zLog ()
{
  // Auto-generated destructor stub
}

/**
 * Initialisiert die LOG Klasse. Sollte nach I2C und ca 1,5s nach Boot
 * kommen, da ansonsten der uC RTC_DCF noch nicht empfangen kann
 * @param psd	Pointer zur SD
 * @param puser	Pointer zu den Userdaten
 * @param ptemp Pointer zu den Temperaturdaten
 */
void
zLog::initialise (SdFat *psd, benutzer *puser, tempControl *ptemp, char *buf)
{
  _sd = psd;  //Save pointer
  _user = puser;
  _temp = ptemp;
  _buf = buf;
  RTC_DCF.begin ();
  RTC_DCF.enableDCF77Reception ();
  RTC_DCF.enableDCF77LED ();   //später ausschalten in der nacht!)
  RTC_DCF.setDateTime (&dateTime); //Damit irgendwas drin is
}

void
zLog::getClockString (void)
{
  RTC_DCF.getDateTime (&dateTime);
  sprintf (_buf, "Es is %02u:%02u:%02u am %02u.%02u.%02u", dateTime.getHour (),
	   dateTime.getMinute (), dateTime.getSecond (), dateTime.getDay (),
	   dateTime.getMonth (), dateTime.getYear ());
}

