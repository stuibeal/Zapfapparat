/*
 * zLog.cpp
 *
 *  Created on: 01.07.2023
 *      Author: Alfred3
 */

#include "zLog.h"
#include "SdFat.h"
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
	sprintf(buf, "%02u%02u%02u%02u%02u%02u", dateTime.getDay(),
			dateTime.getMonth(), dateTime.getYear(), dateTime.getHour(),
			dateTime.getMinute(), dateTime.getSecond());
}

uint8_t zLog::getWochadog(void) {
	RTC_DCF.getDateTime(&dateTime);
	return dateTime.getWeekday();

}

void zLog::setDcfLed(bool onoff) {
	if (onoff) {
		RTC_DCF.enableDCF77LED();
	} else {
		RTC_DCF.disableDCF77LED();
	}
}

void zLog::writeDataInBuf(uint16_t data, bool komma) {
	switch(komma) {
	case KOMMA:
		sprintf(buf, "%d,%d; ", data/100, data%100);
		break;
	case GANZZAHL:
		sprintf(buf, "%d; ", data);
		break;

	}

}

/* Name:			dataLogger
 * Beschreibung:	Diese Funktion schreibt die aktuellen Daten auf die SD Karte
 *
 */
bool zLog::logAfterZapf(void) {
	// TBD: other files müssen alle zu sein
	// jeden Tag ein File für Zapfdaten
	bool fileStatus = 1;
	//Zeit einlesen
	RTC_DCF.getDateTime(&dateTime);
	sprintf(buf, "LOG_%02u%02u%2u.csv", dateTime.getDay(), dateTime.getMonth(),
			dateTime.getYear());

	//Wenn Datei noch nicht vorhanden, Kopfzeile schreiben!
	if (!_sd->exists(buf)) {
		FsFile logZapfFile = _sd->open(buf, FILE_WRITE);
		fileStatus = logZapfFile;
		if (fileStatus) {
			logZapfFile.print(F("Zeit; "));
			for (uint8_t x = 0; x < _user->arrayGroesse; x++) {
				logZapfFile.print(_user->userN[x]);
				logZapfFile.print(F("; "));
			}
			logZapfFile.print(F("Gesamtmenge; "));
			logZapfFile.print(F("Tagesmenge; "));
			logZapfFile.print(F("Fassrest; "));
			logZapfFile.print(F("Spannung; "));
			logZapfFile.print(F("Stromverbrauch Wh; "));
			logZapfFile.print(F("KühlwasserTemp; "));
			logZapfFile.println(F("Helligkeit"));
			logZapfFile.close();
		} else {
			logZapfFile.close();
			return fileStatus;
		}
	}

	//Wenn es die Datei schon gibt, Daten anfügen
	if (_sd->exists(buf)) {
		FsFile logZapfFile = _sd->open(buf, FILE_WRITE);
		fileStatus = logZapfFile;
		if (fileStatus) {
			sprintf(buf, "%02u:%02u:%02u; ", dateTime.getHour(),
					dateTime.getMinute(), dateTime.getSecond());
			logZapfFile.print(buf);
			for (uint8_t x = 0; x < _user->arrayGroesse; x++) {
				logZapfFile.print(_user->bierTag[x]);
				logZapfFile.print(F("; "));
			}
			writeDataInBuf(_user->gesamtMengeTotal, GANZZAHL);
			logZapfFile.print(buf);
			writeDataInBuf(_user->gesamtMengeTag, GANZZAHL);
			logZapfFile.print(buf);
			writeDataInBuf(_user->restMengeFass, GANZZAHL);
			logZapfFile.print(buf);
			writeDataInBuf(power.getInVoltage(), KOMMA);
			logZapfFile.print(buf);
			writeDataInBuf(temp.getStromVerbrauchLetzteZapfung(), GANZZAHL);
			logZapfFile.print(buf);
			writeDataInBuf(temp.getKuehlWasserTemp(), KOMMA);
			logZapfFile.print(buf);
			writeDataInBuf(power.getHelligkeit(), GANZZAHL);
			logZapfFile.println(buf);
			logZapfFile.close();
		} else {
			logZapfFile.close();
			return fileStatus;
		}
	}
	return fileStatus;
}

/*
 // Daten schreiben
 char timeBuf[20] = "00.00.00 00:00:00, ";
 sprintf(timeBuf, "%02u.%02u.%02u %02u:%02u:%02u, ", dateTime.getDay(),
 dateTime.getMonth(), dateTime.getYear(), dateTime.getHour(),
 dateTime.getMinute(), dateTime.getSecond());
 File dataFile = SD.open(fileBuf, FILE_WRITE);

 dataString = timeBuf;

 for (uint8_t x = 0; x < 11; x++) {
 dataString += String(user.tag());
 dataString += ",";
 }
 dataString += String(user.gesamtMengeTag);
 dataString += ",";
 dataString += String(inVoltage);
 dataString += ",";
 dataString += String(hell);

 // if the file is available, write to it:
 if (dataFile) {
 dataFile.println(dataString);
 dataFile.close();
 }
 // if the file isn't open, pop up an error:
 else {
 //Serial.println("konnte z-log.csv nicht öffnen");
 }
 */

