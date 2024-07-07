/*
 * benutzer.cpp
 *
 *  Created on: 18.06.2023
 *      Author: alfred3
 */

#include "benutzer.h"

benutzer::benutzer() {
	gesamtMengeTotal = 0;
    gesamtMengeTag = 0;

	for(uint8_t x = 0; x < 11; x++) {
		bierTemp[x] = 300;
		bierMenge[x] = 333;
		bierTag[x] = 0;
		bierGesamt[x] = 0;
		godMode[x] = false;
		musik[x] = 0;

	}
	zapfStatus= zapfStandby;
	restMengeFass = 30000;



}

benutzer::~benutzer() {
	// TODO Auto-generated destructor stub
}


uint16_t benutzer::menge() {
	return bierMenge[aktuell];
}

uint16_t benutzer::temp() {
	return bierTemp[aktuell];
}

uint16_t benutzer::gesamt() {
	return bierGesamt[aktuell];
}

uint16_t benutzer::tag() {
	return bierTag[aktuell];
}

void benutzer::addBier(uint16_t zapfmenge)  {
	bierTag[aktuell] += zapfmenge;
	bierGesamt[aktuell] += zapfmenge;
	gesamtMengeTotal += zapfmenge;
	gesamtMengeTag += zapfmenge;
}

String benutzer::getName()  {
	return username[aktuell];
}

uint8_t benutzer::getMusik()  {
	return musik[aktuell];
}

void benutzer::setMusik(uint8_t musikvar)  {
	musik[aktuell] = musikvar;
}

void benutzer::setMenge(uint16_t mengevar)  {
	bierMenge[aktuell] = mengevar;
}

void benutzer::setTemp(uint16_t tempvar)  {
	bierTemp[aktuell] = tempvar;
}

void benutzer::setGodMode(uint8_t godmodetype)  {
	godMode[aktuell] = godmodetype;
}

uint8_t benutzer::getGodMode()  {
	return godMode[aktuell];
}
