/*
 * waehlscheibe.c
 *
 *  Created on: 29.06.2024
 *      Author: al
 */
#include "Arduino.h"
#include "waehlscheibe.h"
#include "gemein.h"
#include <PCA9685_LED_DRIVER.h>
#include "stdint.h"
#include "globalVariables.h"


/* Class Constructor */
PCA9685 wsLed = (WS_LED_ADDRESS);

/* Variables */
uint16_t WSpwmVal[12];

void beginWaehlscheibeLed(void) {
	wsLed.begin(WS_LED_FREQUENCY);
	wsLedGrundbeleuchtung();
}

void wsLedGrundbeleuchtung() {
	wsLed.setPWM(WSpwmVal, sizeof(WSpwmVal));
	for (uint8_t channel = 0; channel < 12; channel++) {
		WSpwmVal[channel] = 20;
	}
	wsLed.update();

}

uint8_t readWaehlscheibe(void) {
	for (uint8_t channel = 0; channel < 12; channel++) {
		WSpwmVal[channel] = 0xFFF;
	}
	wsLed.update();

	bool old_waehler2 = 1;
	bool waehler2 = digitalRead(WSpuls);
	unsigned long temptime = 0;

	uint8_t waehlZahl = 0;  //Per Wählscheibe ermittelte Zahl

	while (digitalRead(WSready) == 1) {

		old_waehler2 = waehler2;
		waehler2 = digitalRead(WSpuls);

		if (waehler2 < old_waehler2) {
			temptime = millis();  //hier die Wählscheibe auslesen
		}

		if ((waehler2 > old_waehler2) && (millis() - temptime > 50)) { //wenn Signal wieder von 0V auf 5V geht und mehr als 50ms vergangen sind, eins hochzählen
			waehlZahl++; //Wählscheibe (US): 60ms PULS 0V, 40ms Pause (5V), ánsonsten immer 5V
			temptime = millis();
			wsLed.setPWM(waehlZahl, 0xFFF);
			if (waehlZahl > 1) {
				wsLed.setPWM(waehlZahl - 1, 1000);
			}
			wsLed.update();
		}
	}
	wsLedGrundbeleuchtung();
	wsLed.setPWM(waehlZahl, 0xFFF);
	wsLed.update();

	return waehlZahl;
}

void oldWaehlscheibeFun(void) {
	sound.mp3Play(11, 1);
	for (uint8_t channel = 0; channel < 12; channel++) {
		WSpwmVal[channel] = 2047;
	}
	wsLed.update();

	for (uint8_t dw = 0; dw < 2; dw++) { //mega Lightshow!!
		for (uint16_t i = 0; i < 11; i++) {
			delay(50);
			for (uint8_t x = 11; x > 0; x--) {
				delay(30);
				WSpwmVal[x + i] = 0xFFF; //pwm.setPWM(x + i, 4096, 0);
				WSpwmVal[x + i + 1] = 0x00; //pwm.setPWM(x + i + 1, 0, 4096);
				wsLed.update();
			}
		}

		for (uint16_t i = 11; i > 0; i--) {
			delay(100 % i);
			for (uint8_t x = 0; x < 11; x++) {
				delay(50);
				WSpwmVal[x + i - 1] = 0x00; //pwm.setPWM(x + i - 1, 0, 4096);
				WSpwmVal[x + i] = 0xFFF; //pwm.setPWM(x + i, 4096, 0);
				wsLed.update();
			}
		}
		delay(500);
	}
	wsLedGrundbeleuchtung();
}

