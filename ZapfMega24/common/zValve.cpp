/*
 * zValve.cpp
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 */

#include "zValve.h"

zValve::zValve() {
	valveMillis = 0;
	oldState = ZU;
	state = MACH_AUF;
	runTime = 0;
	cleanPumpState = OFF;
	valveProzentSoll = 100;
	valveProzentIst = 0;
}

zValve::~zValve() {
	// Auto-generated destructor stub
}
void zValve::begin() {
	pinMode(VALVE_AUF_PIN, OUTPUT);
	digitalWrite(VALVE_AUF_PIN, LOW);
	pinMode(VALVE_ZU_PIN, OUTPUT);
	digitalWrite(VALVE_ZU_PIN, LOW);
	pinMode(CLEAN_PUMP_PIN, OUTPUT);
	digitalWrite(CLEAN_PUMP_PIN, LOW);
	pinMode(PRESSURE_SENS_PIN, INPUT);
	valveMillis = millis();
	openValve();
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
int zValve::getPressure() {
//	uint32_t druckWert = (analogRead(PRESSURE_SENS_PIN) - PRESSURE_ZERO)*100;
//	int druck = (int) (druckWert / 146); //dann hat man das ergebnis in atü * 100
//	if (druck < 0) {
//		druck=0;
//	}

	long int druckWert = analogRead(PRESSURE_SENS_PIN);
	long int druck = map(druckWert, 120, 730, 0, 500);
	if (druck < 0 ){
		druck = 0;
	}
	return (int)druck;
}

/* @brief  Hinterer Ausgang wird mit 12V versorgt für Reinigungspumpe
 * 	   Klinkenanschluss 6.3mm
 */
void zValve::cleanPumpOn() {
	cleanPumpState = ON;
	if (state != GANZ_AUF) {
		valveProzentSoll = 100;
		state = MACH_AUF;
		check();
	}
	digitalWrite(CLEAN_PUMP_PIN, cleanPumpState);
}

/** @brief  Hinterer Ausgang wird abgeschaltet
 * 	    Das Ventil soll(!) offen bleiben, damit das ganze dann
 * 	    evtl mit CO2 durchgeblasen werden kann.
 */
void zValve::cleanPumpOff() {
	cleanPumpState = OFF;
	digitalWrite(CLEAN_PUMP_PIN, cleanPumpState);
}

void zValve::openValve() {
	valveProzentSoll = 100;
	check();
}

void zValve::closeValve() {
	valveProzentSoll = 0;
	check();
}

void zValve::setValveProzent(uint8_t prozent) {
	valveProzentSoll = prozent;
	check();
}

void zValve::check() {
	if (valveProzentSoll != valveProzentIst || state == ZWISCHENDRIN) {

		if (valveProzentIst < valveProzentSoll
				&& state != MACHT_AUF) {
			state = MACH_AUF;
		}
		if (valveProzentIst > valveProzentSoll
				&& state !=MACHT_ZU) {
			state = MACH_ZU;
		}

		switch (state) {
		case MACH_AUF:
			open();
			valveMillis = millis();
			state = MACHT_AUF;
			break;
		case MACHT_AUF:

			runTime += (millis() - valveMillis);
			valveMillis = millis();
			valveProzentIst = (int8_t) (runTime / VALVE_TEILER);
			if (valveProzentIst >= valveProzentSoll) {
				off();
				state = ZWISCHENDRIN;
			}
			break;
		case ZWISCHENDRIN:
			if (valveProzentIst > 99) {
				valveProzentIst = 100;
				state = GANZ_AUF;
				runTime = VALVE_TEILER * 100;
			}
			if (valveProzentIst < 1) {
				valveProzentIst = 0;
				state = ZU;
				runTime = 0;
			}
			break;
		case MACH_ZU:
			close();
			valveMillis = millis();
			state = MACHT_ZU;
			break;
		case MACHT_ZU:
			runTime = runTime - (millis() - valveMillis);
			valveMillis = millis();
			valveProzentIst =  (int8_t) (runTime / VALVE_TEILER);
			if (valveProzentIst <= valveProzentSoll) {
				off();
				state = ZWISCHENDRIN;
			}
			break;
		default:
			delay(1);
			break;

		} /*switch*/

	} /*if*/

} /*void*/

void zValve::open() {
	digitalWrite(VALVE_ZU_PIN, LOW);
	digitalWrite(VALVE_AUF_PIN, HIGH);
}

void zValve::close() {
	digitalWrite(VALVE_AUF_PIN, LOW);
	digitalWrite(VALVE_ZU_PIN, HIGH);
}

void zValve::off() {
	digitalWrite(VALVE_ZU_PIN, LOW);
	digitalWrite(VALVE_AUF_PIN, LOW);
}
