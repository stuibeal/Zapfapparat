/*
 * benutzer.h
 *
 *  Created on: 18.06.2023
 *      Author: alfred3
 */

#ifndef BENUTZER_H_
#define BENUTZER_H_
#include <Arduino.h>
#include <stdint.h>
#include <string.h>

//const uint8_t arrayGroesse = 20;
#define arrayGroesse 20   //0+19!!!!
#define EEPROM_START_ADDR_BIERTAG 100
#define EEPROM_START_ADDR_BIERGESAMT 200

class benutzer {
public:
	benutzer();
	virtual ~benutzer();
	uint8_t aktuell = 0;
	const char *userN[arrayGroesse] ={"Bitte wählen","Christoph","Mädels","Barracuda","Ebi","Maex","Didi","Beda","Basi","Al","Burschen","Alois","Stefan M.L.","Framo","Marina","Anne","Pia","Italia","Maria","Piene"};
	uint16_t bierTemp[arrayGroesse];
	uint16_t bierMenge[arrayGroesse];
	uint16_t bierTag[arrayGroesse];
	uint16_t bierGesamt[arrayGroesse];
	uint16_t zapfMenge;
	uint16_t lastZapfMenge;
	uint16_t gesamtMengeTotal;
	uint16_t gesamtMengeTag;
	uint16_t restMengeFass; /*Fass sollte unter 65,535l haben*/
	uint16_t gesamt();
	String getName();
	void cleanEEPROM(void);
	void writeDataToEEPROM(void);
	void readDataFromEEPROM(void);
	void addBier(void);
	uint8_t musik[arrayGroesse];
	uint8_t getMusik();
	void setMusik(uint8_t musikvar);
	void setMenge(uint16_t mengevar);
	void setTemp(uint16_t tempvar);
	uint8_t checkNullUser(void);

	inline void setGodMode(uint8_t wasfuergott) {
		godMode[aktuell] = wasfuergott;
	}
	inline uint8_t getGodMode(void) {
		return godMode[aktuell];
	}
	inline uint16_t getSollTemperatur(void) {
		return bierTemp[aktuell];
	}
	inline uint16_t getMenge(void) {
		return bierMenge[aktuell];
	}
	inline uint16_t getBierTag(void) {
		return bierTag[aktuell];
	}
	inline uint16_t getBierGesamt(void) {
		return bierGesamt[aktuell];
	}
	inline uint16_t getRestMengeFass(void) {
		return restMengeFass;
	}

	enum zapfModus {
		zapfStandby,
		zapfError,
		zapfBeginn,
		amZapfen,
		godZapfen,
		kurzVorZapfEnde,
		zapfEnde
	};
	zapfModus zapfStatus;
	zapfModus oldZapfStatus;

private:
	uint16_t oldBierGesamt[arrayGroesse];
	uint8_t godMode[arrayGroesse];

};

#endif /* BENUTZER_H_ */
