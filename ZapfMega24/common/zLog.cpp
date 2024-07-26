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
	DateTime dateTime;
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
void zLog::initialise() {
	RTC_DCF.begin();
	RTC_DCF.enableDCF77Reception();
	RTC_DCF.enableDCF77LED();   //später ausschalten in der nacht!)
	dateTime = DateTime(0, 1, 1, SAMSDA, 0, 0, 0);
	//RTC_DCF.setDateTime(&dateTime); //Damit irgendwas drin is
}

void zLog::getClockString(void) {
	RTC_DCF.getDateTime(&dateTime);
	sprintf_P(buf, PSTR("Es is %02u:%02u:%02u am %02u.%02u.%02u"), dateTime.getHour(),
			dateTime.getMinute(), dateTime.getSecond(), dateTime.getDay(),
			dateTime.getMonth(), dateTime.getYear());
}
void zLog::getClockBarcode(void) {
	RTC_DCF.getDateTime(&dateTime);
	sprintf_P(buf, PSTR("%02u%02u%02u%02u%02u%02u"), dateTime.getDay(),
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

void zLog::writeData(File *file, uint16_t data, bool komma) {
	switch (komma) {
	case KOMMA:
		sprintf_P(buf, PSTR("%d,%d; "), data / 100, data % 100);
		break;
	case GANZZAHL:
		sprintf_P(buf, PSTR("%d; "), data);
		break;
	}
	file->print(buf);
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
	//sound._SMF->close();
	RTC_DCF.getDateTime(&dateTime);
	SD.chdir("/");
	sprintf_P(buf, PSTR("/log/LOG_%02u%02u%2u.csv"), dateTime.getDay(), dateTime.getMonth(),
			dateTime.getYear());

	//Wenn Datei noch nicht vorhanden, Kopfzeile schreiben!
	if (!SD.exists(buf)) {
		FsFile logZapfFile = SD.open(buf, FILE_WRITE);
		fileStatus = logZapfFile;
		if (fileStatus) {
			logZapfFile.print(F("Zeit; "));
			for (uint8_t x = 0; x < user.arrayGroesse; x++) {
				logZapfFile.print(user.userN[x]);
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
	if (SD.exists(buf)) {
		FsFile logZapfFile = SD.open(buf, FILE_WRITE);
		fileStatus = logZapfFile;
		if (fileStatus) {
			sprintf_P(buf, PSTR("%02u:%02u:%02u; "), dateTime.getHour(),
					dateTime.getMinute(), dateTime.getSecond());
			logZapfFile.print(buf);
			for (uint8_t x = 0; x < user.arrayGroesse; x++) {
				logZapfFile.print(user.bierTag[x]);
				logZapfFile.print(F("; "));
			}
			writeData(&logZapfFile, user.gesamtMengeTotal, GANZZAHL);
			writeData(&logZapfFile, user.gesamtMengeTag, GANZZAHL);
			writeData(&logZapfFile, user.restMengeFass, GANZZAHL);
			writeData(&logZapfFile, power.getInVoltage()*10, KOMMA);
			writeData(&logZapfFile, temp.getStromVerbrauchLetzteZapfung(), GANZZAHL);
			writeData(&logZapfFile, temp.getKuehlWasserTemp(), KOMMA);
			writeData(&logZapfFile, power.getHelligkeit(), GANZZAHL);
			logZapfFile.println(F(""));
			logZapfFile.close();
		} else {
			logZapfFile.close();
			return fileStatus;
		}
	}
	return fileStatus;
}

bool zLog::logSystemMsg(const __FlashStringHelper *sysMsg) {
	//sound._SMF->close();

	bool fileStatus = 1;
	RTC_DCF.getDateTime(&dateTime);
	SD.chdir("/");
	sprintf_P(buf, PSTR("/log/syslog_%02u%02u%2u.log"), dateTime.getDay(), dateTime.getMonth(),
			dateTime.getYear());
	FsFile systemLogFile = SD.open(buf, FILE_WRITE);
	fileStatus = systemLogFile;
	if (fileStatus) {
		char timeBuf[22]="";
		RTC_DCF.getDateTime(&dateTime);
		sprintf_P(timeBuf, PSTR("%02u.%02u.%02u %02u:%02u:%02u - "), dateTime.getDay(),
				dateTime.getMonth(), dateTime.getYear(), dateTime.getHour(),
				dateTime.getMinute(), dateTime.getSecond());
		systemLogFile.print(timeBuf);
		systemLogFile.println(sysMsg);
	}
	systemLogFile.close();
	return fileStatus;
}

bool zLog::logSystemMsg(const char *sysMsg) {
	//sound._SMF->close();

	bool fileStatus = 1;
	RTC_DCF.getDateTime(&dateTime);
	SD.chdir("/");
	sprintf_P(buf, PSTR("/log/syslog_%02u%02u%2u.log"), dateTime.getDay(), dateTime.getMonth(),
			dateTime.getYear());
	FsFile systemLogFile = SD.open(buf, FILE_WRITE);
	fileStatus = systemLogFile;
	if (fileStatus) {
		char timeBuf[22]="";
		sprintf_P(timeBuf, PSTR("%02u.%02u.%02u %02u:%02u:%02u - "), dateTime.getDay(),
				dateTime.getMonth(), dateTime.getYear(), dateTime.getHour(),
				dateTime.getMinute(), dateTime.getSecond());
		systemLogFile.print(timeBuf);
		systemLogFile.println(sysMsg);
	}
	systemLogFile.close();
	return fileStatus;
}
