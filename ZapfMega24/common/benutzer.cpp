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
void benutzer::writeDataToEEPROM() {
	EEPROM.put(0, restMengeFass);
	EEPROM.put(2, gesamtMengeTag);
	EEPROM.put(4, gesamtMengeTotal);
	EEPROM.put(6, gesamtMengeTag);
	EEPROM.put(EEPROM_START_ADDR_BIERTGESAMT+aktuell*2, bierGesamt[aktuell] );
	EEPROM.put(EEPROM_START_ADRR_BIERTAG+aktuell*2, bierTag[aktuell]);
}

void benutzer::readDataFromEEPROM() {
	EEPROM.get(0, restMengeFass);
	EEPROM.get(2, gesamtMengeTag);
	EEPROM.get(4, gesamtMengeTotal);
	EEPROM.get(6, gesamtMengeTag);
	for (uint8_t x = 0; x< arrayGroesse; x += 2) {
		/* bei Adresse 100 starten für Userbier */
		EEPROM.get(EEPROM_START_ADDR_BIERTGESAMT+x*2, bierGesamt[x] );
		EEPROM.get(EEPROM_START_ADRR_BIERTAG+x*2, bierTag[x]);
	}
}


void benutzer::addBier() {
	writeDataToEEPROM();
	bierTag[aktuell] += zapfMenge;
	bierGesamt[aktuell] += zapfMenge;
	gesamtMengeTotal += zapfMenge;
	gesamtMengeTag += zapfMenge;
	restMengeFass -= zapfMenge;
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
