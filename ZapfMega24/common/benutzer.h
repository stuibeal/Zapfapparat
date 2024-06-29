/*
 * benutzer.h
 *
 *  Created on: 18.06.2023
 *      Author: alfred3
 */

#ifndef BENUTZER_H_
#define BENUTZER_H_
#include "Arduino.h"
#include <stdint.h>
#include <string.h>

const uint8_t arrayGroesse = 15;

class benutzer {
public:
	benutzer();
	virtual ~benutzer();
	uint8_t aktuell = 1;
    String username[arrayGroesse] =
    		{ "Honk", "Christoph", "NO-Y", "DJ R. Kentn", "Ebi", "Maex", "Didi", "Beda", "Basi", "Al" };
    uint16_t gesamtMengeTotal;
    uint16_t gesamtMengeTag;
    uint16_t bierTemp[arrayGroesse];
    uint16_t bierMenge[arrayGroesse];
    uint16_t bierTag[arrayGroesse];
    uint16_t bierGesamt[arrayGroesse];
    uint8_t godMode[arrayGroesse];
    uint16_t menge();
    uint16_t temp();
    uint16_t gesamt();
    uint16_t tag();
    String getName();
    void addBier(uint16_t zapfmenge);
    uint8_t musik[arrayGroesse];
    uint8_t getMusik();
    void setMusik(uint8_t musikvar);
    void setMenge(uint16_t mengevar);
    void setTemp(uint16_t tempvar);
    void setGodMode(uint8_t wasfuergott);
    uint8_t getGodMode();

private:
	uint16_t oldBierGesamt[arrayGroesse];
};

#endif /* BENUTZER_H_ */
