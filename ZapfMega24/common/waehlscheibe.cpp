/*
 * waehlscheibe.cpp
 *
 *  Created on: 29.06.2024
 *      Author: al
 */
#include "Arduino.h"
#include "waehlscheibe.h"
#include "gemein.h"
#include "stdint.h"
#include "audio.h"
#include "zPower.h"
#include "globalVariables.h"


/* Variables */

void beginWaehlscheibe(void) {
	//Wählscheibe
	pinMode(WSpuls, INPUT); // WSpuls auf Input (Interrupt)
	pinMode(WSready, INPUT);  //Wählscheibe Puls

}

uint8_t readWaehlscheibe(void) {
	for (uint8_t channel = 0; channel < 12; channel++) {
		power.setLed(channel, 1);
	//	wsLed.setPWM(channel, 0xFFF);
	}

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
			power.setLed(waehlZahl, 1);
			//wsLed.setPWM(waehlZahl, 0xFFF);
			if (waehlZahl > 1) {
				power.setLed(waehlZahl -1, 0);
				//wsLed.setPWM(waehlZahl - 1, GRUEN_LED_ABGEDUNKELT + 2000);
			}
//			if (waehlZahl > 2) {
//				power.setLed(waehlZahl -2, 0);
//				wsLed.setPWM(waehlZahl - 2, GRUEN_LED_ABGEDUNKELT);
//
//			}

		}
	}
	power.wsLedGrundbeleuchtung();
	power.setLed(waehlZahl, 1);
	//wsLed.setPWM(waehlZahl, 0xFFF);
	if (waehlZahl > 10) {
		/* not possible bei einer Wählscheibe */
		waehlZahl = 0;
	}

	return waehlZahl;
}

void oldWaehlscheibeFun(void) {
	sound.on();
	sound.mp3Play(22, 1);
	for (uint8_t channel = 0; channel < 12; channel++) {
		wsLed.setPWM(channel, 2047);
	}

	for (uint8_t dw = 0; dw < 2; dw++) { //mega Lightshow!!
		for (uint16_t i = 0; i < 11; i++) {
			delay(50);
			for (uint8_t x = 11; x > 0; x--) {
				delay(30);
				wsLed.setPWM(x + i, 0xFFF); //pwm.setPWM(x + i, 4096, 0);
				wsLed.setPWM(x + i + 1, 0x00); //pwm.setPWM(x + i + 1, 0, 4096);
			}
		}

		for (uint16_t i = 11; i > 0; i--) {
			delay(100 % i);
			for (uint8_t x = 0; x < 11; x++) {
				delay(50);
				wsLed.setPWM(x + i - 1, 0x00); //pwm.setPWM(x + i - 1, 0, 4096);
				wsLed.setPWM(x + i, 0xFFF); //pwm.setPWM(x + i, 4096, 0);
			}
		}
		delay(500);
	}
	power.wsLedGrundbeleuchtung();
}

