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
#define LICHT_SENSOR_PIN  A6
#define LCD_BACKLIGHT_PIN 4   //Hintergrundbeleuchtung Display
#define OTHER_MC_PIN	  A5
#define LAMPE_ON 💡
#define LAMPE_AUS 🚫

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
inline void setPowerState(zPower::powerState state) {
	bkPowerState = state;
}

void begin();
void check();
void setLed(uint8_t offon);
void schLampeControl(uint8_t offon, uint16_t dimspeed);
void zapfLichtControl(uint8_t offon, uint16_t dimspeed);
void autoLight(uint8_t offon);
void goSleep(void);

private:
uint8_t inVoltage;
int helligkeit;
uint8_t autoLightBool;
uint8_t lampenOutput;
powerState bkPowerState;
machineState bkMachineState;
unsigned long millisSeitZapfEnde;
unsigned long millisSeitLetztemCheck;

};

#endif /* ZPOWER_H_ */
