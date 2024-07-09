/*
 * zValve.h
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 *
 *      macht die Valve auf und zu
 *      checkt den Druck
 */

#ifndef ZVALVE_H_
#define ZVALVE_H_
#include "gemein.h"
#include <Arduino.h>

#define VALVE_TEILER 91
#define PRESSURE_ZERO 115
#define PRESSURE_SENS_PIN    A7

#define VALVE_AUF_PIN          42  //mach auf den Hahn, geht hardwaremäßig nur wenn valveZu aus
#define VALVE_ZU_PIN           44  //mach zu den Hahn, geht hardwaremäßig nur wenn valveAuf aus
#define CLEAN_PUMP_PIN           5   // PWM outputpin Pumpe -> Reinigung!


#define ZU 1
#define MACH_AUF 2
#define MACHT_AUF 3
#define ZWISCHENDRIN 4
#define GANZ_AUF 5
#define MACH_ZU 6
#define MACHT_ZU 7

#define OFF 0
#define ON 1
//	enum valveState {
//		ZU, MACH_AUF, MACHT_AUF, ZWISCHENDRIN, GANZ_AUF, MACH_ZU, MACHT_ZU
//	};
//
//	enum pumpState {
//		OFF, ON
//	};
//


//Old Things, good Things. Brauch ma nimmer.
#define oldValve          49  //Outputpin Magnetventil

class zValve {
public:
	zValve();
	virtual
	~zValve();
	void
	begin();
	inline uint8_t getValveState() {
		return state;
	}
	inline int8_t getValveProzent() {
		return valveProzentIst;
	}
	inline int8_t getValveProzentSoll() {
		return valveProzentSoll;
	}
	inline uint8_t getOldState() {
		return oldState;
	}

	inline uint8_t getCleanPumpState() {
		return cleanPumpState;
	}
	inline unsigned long getRunTime() {
		return runTime;
	}
	inline unsigned long getValveMillis() {
		return valveMillis;
	}

	int
	getPressure();
	void
	cleanPumpOn();
	void
	cleanPumpOff();
	void
	openValve();
	void
	closeValve();
	void
	setValveProzent(uint8_t prozent);
	void
	check();


private:
	uint8_t state;
	uint8_t oldState;
	uint8_t cleanPumpState;
	unsigned long valveMillis;
	unsigned long runTime;
	int8_t valveProzentSoll;
	int8_t valveProzentIst;

	void open();
	void close();
	void off();

};

#endif /* ZVALVE_H_ */
