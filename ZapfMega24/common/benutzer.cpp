/*
 * benutzer.cpp
 *
 *  Created on: 18.06.2023
 *      Author: alfred3
 */

#include "benutzer.h"
#include "gemein.h"
#include "globalVariables.h"
#include "EEPROM.h"

benutzer::benutzer() {
	gesamtMengeTotal = 0;
	gesamtMengeTag = 0;
	zapfStatus = zapfStandby;
	oldZapfStatus = zapfStandby;
	restMengeFass = 30000;
	zapfMenge = 0;
	lastZapfMenge = 0;
	for (uint8_t x = 0; x < arrayGroesse; x++) {
		bierTemp[x] = STANDARD_TEMP;
		bierMenge[x] = STANDARD_MENGE;
		bierTag[x] = 0;
		bierGesamt[x] = 0;
		godMode[x] = false;
		musik[x] = 0;
	}
}

benutzer::~benutzer() {
}

uint16_t benutzer::gesamt() {
	return bierGesamt[aktuell];
}

/**
 *  schreibt die Daten ins EEPROM (bei Stromausfall gesichert)
 */

void benutzer::cleanEEPROM() {
	for (uint16_t x = 2; x < 400; x++) {
		EEPROM.write(x, 0);
	}
}

void benutzer::writeDataToEEPROM() {
	eeprom_write_word((uint16_t*) 0, restMengeFass);
	eeprom_write_word((uint16_t*) 2, gesamtMengeTag);
	eeprom_write_word((uint16_t*) 4, gesamtMengeTotal);
	eeprom_write_word((uint16_t*) EEPROM_START_ADDR_BIERGESAMT + (aktuell * 2),
			bierGesamt[aktuell]);
	eeprom_write_word((uint16_t*) EEPROM_START_ADDR_BIERTAG + (aktuell * 2),
			bierTag[aktuell]);
	eeprom_write_byte((uint8_t*) EEPROM_START_ADDR_MUSIK + aktuell,
			musik[aktuell]);
}

void benutzer::readDataFromEEPROM() {
	restMengeFass = eeprom_read_word((uint16_t*) 0);
	gesamtMengeTag = eeprom_read_word((uint16_t*) 2);
	gesamtMengeTotal = eeprom_read_word((uint16_t*) 4);
	for (uint8_t x = 0; x < arrayGroesse; x++) {
		/* bei Adresse 100 starten für Userbier */
		bierGesamt[x] = eeprom_read_word(
				(uint16_t*) EEPROM_START_ADDR_BIERGESAMT + (x * 2));
		bierTag[x] = eeprom_read_word(
				(uint16_t*) EEPROM_START_ADDR_BIERTAG + (x * 2));
		musik[x] = eeprom_read_byte(
				(uint8_t*) EEPROM_START_ADDR_MUSIK + aktuell);
	}

}

void benutzer::addBier() {
	bierTag[aktuell] += zapfMenge;
	bierGesamt[aktuell] += zapfMenge;
	gesamtMengeTotal += zapfMenge;
	gesamtMengeTag += zapfMenge;
	restMengeFass -= zapfMenge;
	flowmeter.flowDataSend(END_ZAPF, 0); //damit die Zapfmillis wieder auf null sind
	lastZapfMenge = zapfMenge;
	zapfMenge = 0;
	writeDataToEEPROM();
}

String benutzer::getName() {
	return userN[aktuell];
}

uint8_t benutzer::getMusik() {
	return musik[aktuell];
}

void benutzer::setMusik(uint8_t musikvar) {
	musik[aktuell] = musikvar;
}

void benutzer::setMenge(uint16_t mengevar) {
	if (mengevar < MIN_MENGE) {
		mengevar = MIN_MENGE;
	}
	bierMenge[aktuell] = mengevar;
}

void benutzer::setTemp(uint16_t tempvar) {
	if (tempvar < MIN_TEMP) {
		tempvar = MIN_TEMP;
	}
	bierTemp[aktuell] = tempvar;
}

uint8_t benutzer::checkNullUser() {
	uint8_t yesno = 0;
	if (bierTemp[0] != STANDARD_TEMP) {
		bierTemp[aktuell] = bierTemp[0];
		bierTemp[0] = STANDARD_TEMP;
		yesno = 1;
	}
	if (bierMenge[0] != STANDARD_MENGE) {
		bierMenge[aktuell] = bierMenge[0];
		bierMenge[0] = STANDARD_MENGE;
		yesno = 1;
	}
	return yesno;
}

void benutzer::clearDayUserData() {
	gesamtMengeTag = 0;
	//Userdaten noch löschen
	for (int x = 0; x < arrayGroesse; x++) {
		user.bierTag[x] = 0;
		user.musik[x] = 0;
	}

}

void benutzer::clearAllUserData() {
	cleanEEPROM();
	gesamtMengeTag = 0;
	gesamtMengeTotal = 0;
	restMengeFass = 30000;
	writeDataToEEPROM();
}
