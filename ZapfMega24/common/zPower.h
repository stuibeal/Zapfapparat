/*
 * zPower.h
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 *
 *      Checkt die Eingangsspannung
 *      Wenn zu gering -> Lampen etc aus
 *      Wenn wieder höher -> Lampen etc wieder an
 *      Schickt den uC in Sleepmode + weckt wieder alles auf
 *
 *
 */

#ifndef ZPOWER_H_
#define ZPOWER_H_
#include "gemein.h"
#include "Arduino.h"
#include "stdint.h"
#include "tempControl.h"

#define Z_SCH_LAMPE_PIN 5  // output for the Lampe
#define LICHT_SENSOR  A6

class zPower {
public:
	enum powerState {
	BATT_ULTRAHIGH, BATT_HIGH, BATT_NORMAL, BATT_LOW, BATT_ULTRALOW
	};

enum machineState {
	SLEEP, WAKE_UP, WORK, STANDBY, GO_SLEEP
};

zPower();
virtual
~zPower();
inline zPower::powerState getPowerState() {
	return bkPowerState;
}
inline zPower::machineState getMachineState() {
	return bkMachineState;
}
inline void setState(zPower::powerState state) {
	bkPowerState = state;
}

void begin(tempControl *pTemp);
void check();
void setLed(uint8_t offon);
void schLampeControl(uint8_t offon, uint16_t dimspeed);

private:
uint8_t inVoltage;
int helligkeit;
uint8_t lampenOutput;
powerState bkPowerState;
machineState bkMachineState;
static unsigned long millisSeitZapfEnde;
static unsigned long millisSeitLetztemCheck;
tempControl *_pTemp;

};

#endif /* ZPOWER_H_ */
