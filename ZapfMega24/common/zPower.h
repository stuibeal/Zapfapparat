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
#include "Arduino.h"
#include "stdint.h"

#include "PCA9685.h"
#include "tempControl.h"

#define Z_SCH_LAMPE_PIN 5  // output for the Lampe
#define LICHT_SENSOR_PIN  A6
#define LCD_BACKLIGHT_PIN 4   //Hintergrundbeleuchtung Display
#define OTHER_MC_PIN	  A5
#define LAMPE_AN 10  //wenn helligkeit kleiner als wert
#define LAMPE_AUS 19 //wenn helligkeit größer als wert
#define WS_LED_ADDRESS     0x40
#define WS_LED_FREQUENCY   400     //min 24Hz, max 1524Hz
#define GRUEN_LED_ABGEDUNKELT 40
#define WEISS_LED_ABGEDUNKELT 20
#define TASTE1_LED 6
#define TASTE2_LED 8
#define TASTEN_LED_NORMAL 10 //grundbeleuchtung der Tasten

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
	inline void setAutoLight(bool offon) {
		autoLightBool = offon;
	}
	inline uint8_t getHelligkeit() {
		return helligkeit;
	}
	inline uint8_t getAutoLightBool() {
		return autoLightBool;
	}
	inline uint8_t getInVoltage() {
		return inVoltage;
	}

	void beginPower();
	void check();
	void tastenLed(uint8_t taste, uint8_t helligkeit);
	void setLed(uint8_t ledNr, bool on);
	void setWhiteLed(uint16_t helligkeit);
	void ledGrundbeleuchtung(void);
	void wsLedGrundbeleuchtung(void);
	void schLampeControl(uint8_t offon);
	void zapfLichtControl(uint8_t pwmValue);
	void autoLight(void);
	void lcdAutoLight(void);
	void setBackLight(void);
	void goSleep(void);

private:

	void dimLightHelper(uint16_t pin, uint8_t wert);
	void dimLight(uint16_t lichtpin, uint8_t anfang, uint8_t ende,
			uint16_t delaytime);
	void setAllWSLed(uint16_t helligkeit);
	uint8_t inVoltage;
	uint16_t helligkeit;
	uint8_t autoLightBool;
	uint8_t zSchLampeStatus;
	uint8_t bkLichtStatus;
	powerState bkPowerState;
	machineState bkMachineState;
	unsigned long millisSeitZapfEnde;
	unsigned long millisSeitLetztemCheck;
	bool oldLightIsOn;
	bool lightIsOn;

};

#endif /* ZPOWER_H_ */
