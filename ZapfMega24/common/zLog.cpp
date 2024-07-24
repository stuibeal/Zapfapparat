/*
 * zLog.cpp
 *
 *  Created on: 01.07.2023
 *      Author: Alfred3
 */

#include "zLog.h"

#include "globalVariables.h"

zLog::zLog() {
	DateTime dateTime = DateTime(0, 1, 1, SAMSDA, 0, 0, 0);
	_sd = nullptr;
	_user = nullptr;
	_temp = nullptr;
	_buf = nullptr;
	logState = 0;

}

zLog::~zLog() {
	// Auto-generated destructor stub
}

/**
 * Initialisiert die LOG Klasse. Sollte nach I2C und ca 1,5s nach Boot
 * kommen, da ansonsten der uC RTC_DCF noch nicht empfangen kann
 * @param psd	Pointer zur SD
 * @param puser	Pointer zu den Userdaten
 * @param ptemp Pointer zu den Temperaturdaten
 */
void zLog::initialise(SdFat *psd, benutzer *puser, tempControl *ptemp,
		char *buf) {
	_sd = psd;  //Save pointer
	_user = puser;
	_temp = ptemp;
	_buf = buf;
	RTC_DCF.begin();
	RTC_DCF.enableDCF77Reception();
	RTC_DCF.enableDCF77LED();   //später ausschalten in der nacht!)
	//RTC_DCF.setDateTime(&dateTime); //Damit irgendwas drin is
}



void zLog::getClockString(void) {
	RTC_DCF.getDateTime(&dateTime);
	sprintf(_buf, "Es is %02u:%02u:%02u am %02u.%02u.%02u", dateTime.getHour(),
			dateTime.getMinute(), dateTime.getSecond(), dateTime.getDay(),
			dateTime.getMonth(), dateTime.getYear());
}
void zLog::getClockBarcode(void) {
	RTC_DCF.getDateTime(&dateTime);
	sprintf(buf, "%02u%02u%02u%02u%02u%02u",  dateTime.getDay(),
			dateTime.getMonth(), dateTime.getYear(),dateTime.getHour(),
			dateTime.getMinute(), dateTime.getSecond());
}

uint8_t zLog::getWochadog(void) {
	RTC_DCF.getDateTime(&dateTime);
	return dateTime.getWeekday();

}


void zLog::setDcfLed(bool onoff){
	if(onoff){
		RTC_DCF.enableDCF77LED();
	} else {
		RTC_DCF.disableDCF77LED();
	}
}

/* Name:			dataLogger
 * Beschreibung:	Diese Funktion schreibt die aktuellen Daten auf die SD Karte
 *
 */
void zLog::logAfterZapf(void) {
	// TBD: other files müssen alle zu sein
	// jeden Tag ein File, ein gesamtfile

	/*
	 //Zeit einlesen
	 RTC_DCF.getDateTime (&dateTime);

	 char fileBuf[20] = "";
	 String dataString = "";
	 sprintf (fileBuf, "LOG_%02u%02u%2u.csv", dateTime.getDay (),
	 dateTime.getMonth (), dateTime.getYear ());

	 if (!SD.exists (fileBuf))
	 {
	 //Wenn Datei noch nicht vorhanden, Kopfzeile schreiben!
	 dataString = "Datum Zeit, ";
	 for (uint8_t x = 0; x < 10; x++)
	 {
	 dataString += user.username[x];
	 dataString += ", ";
	 }
	 dataString += "Gesamtmenge, ";
	 dataString += "Batterie-Volt, ";
	 dataString += "Helligkeit";

	 File dataFile = SD.open (fileBuf, FILE_WRITE);
	 // if the file is available, write to it:
	 if (dataFile)
	 {
	 dataFile.println (dataString);
	 dataFile.close ();
	 }
	 // if the file isn't open, pop up an error:
	 else
	 {
	 //Serial.println("konnte z-log.csv nicht öffnen");
	 }
	 }

	 // Daten schreiben
	 char timeBuf[20] = "00.00.00 00:00:00, ";
	 sprintf (timeBuf, "%02u.%02u.%02u %02u:%02u:%02u, ", dateTime.getDay (),
	 dateTime.getMonth (), dateTime.getYear (), dateTime.getHour (),
	 dateTime.getMinute (), dateTime.getSecond ());
	 File dataFile = SD.open (fileBuf, FILE_WRITE);

	 dataString = timeBuf;

	 for (uint8_t x = 0; x < 11; x++)
	 {
	 dataString += String (user.tag ());
	 dataString += ",";
	 }
	 dataString += String (user.gesamtMengeTag);
	 dataString += ",";
	 dataString += String (inVoltage);
	 dataString += ",";
	 dataString += String (hell);

	 // if the file is available, write to it:
	 if (dataFile)
	 {
	 dataFile.println (dataString);
	 dataFile.close ();
	 }
	 // if the file isn't open, pop up an error:
	 else
	 {
	 //Serial.println("konnte z-log.csv nicht öffnen");
	 }
	 */
}

