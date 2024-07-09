/*
 * zPower.cpp
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 */

#include "zPower.h"
#include "Arduino.h"
#include "gemein.h"
#include "globalVariables.h"
#include "PCA9685.h"
#include "Wire.h"

zPower::zPower() {
	inVoltage = 120;
	helligkeit = 0;
	bkPowerState = BATT_NORMAL;
	bkMachineState = WAKE_UP;
	millisSeitZapfEnde = 0;
	millisSeitLetztemCheck = 0;
	autoLightBool = 1;
	zSchLampeStatus = 0;
	bkLichtStatus = 0;
}

zPower::~zPower() {
	/* Auto-generated destructor stub */
}

void zPower::beginPower() {
	//I2C
	Wire.begin(); // Master of the universe
	Wire.setClock(400000); // I2C in FastMode 400kHz

	wsLed.begin();
	wsLed.setFrequency(200, 0);

	pinMode(OTHER_MC_PIN, OUTPUT);
	digitalWrite(OTHER_MC_PIN, HIGH);
	pinMode(TASTE1_PIN, INPUT);
	pinMode(TASTE2_PIN, INPUT);
	pinMode(TASTE1_LED, OUTPUT);
	pinMode(TASTE2_LED, OUTPUT);
	pinMode(Z_SCH_LAMPE_PIN, OUTPUT);
	pinMode(LICHT_SENSOR_PIN, INPUT);
	pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
	analogWrite(LCD_BACKLIGHT_PIN, 128);
	ledGrundbeleuchtung();
}

void zPower::check() {
	if (millis() - millisSeitLetztemCheck > 1000) {
		millisSeitLetztemCheck = millis();
		uint16_t oldHelligkeit = helligkeit;
		switch (temp.getBatterieStatus()) {
		case 0x00:
			bkPowerState = BATT_ULTRAHIGH;
			break;
		case 0x01:
			bkPowerState = BATT_HIGH;
			break;
		case 0x02:
			bkPowerState = BATT_NORMAL;
			break;
		case 0x03:
			bkPowerState = BATT_LOW;
			break;
		case 0x04:
			bkPowerState = BATT_ULTRALOW;
			break;
		}
		uint16_t newHelligkeit = analogRead(LICHT_SENSOR_PIN);
		helligkeit = (oldHelligkeit + newHelligkeit) / 8;
	}
}

void zPower::tastenLed(uint8_t taste, uint8_t helligkeit) {
	if (bkPowerState >= powerState::BATT_NORMAL) {
		switch (taste) {
		case 0:
			analogWrite(TASTE1_LED, helligkeit);
			analogWrite(TASTE2_LED, helligkeit);
			break;
		case 1:
			analogWrite(TASTE1_LED, helligkeit);
			break;
		case 2:
			analogWrite(TASTE2_LED, helligkeit);
			break;
		}
	} else {
		digitalWrite(TASTE1_LED, 0);
		digitalWrite(TASTE2_LED, 0);
	}
}

void zPower::setLed(uint8_t ledNr, uint16_t pwm) {
	wsLed.setPWM(ledNr, pwm);

}

void zPower::setWhiteLed(uint16_t helligkeit) {
	wsLed.setPWM(0, helligkeit);
	wsLed.setPWM(11, helligkeit);

}

void zPower::ledGrundbeleuchtung() {
	wsLedGrundbeleuchtung();
	tastenLed(0, TASTEN_LED_NORMAL);
}

void zPower::wsLedGrundbeleuchtung() {
	//grün
	for (uint8_t channel = 1; channel < 11; channel++) {
		wsLed.setPWM(channel, GRUEN_LED_ABGEDUNKELT);
	}
	//weiß
	wsLed.setPWM(0, WEISS_LED_ABGEDUNKELT);
	wsLed.setPWM(11, WEISS_LED_ABGEDUNKELT);
}

void zPower::schLampeControl(uint8_t offon) {
	if (bkPowerState >= powerState::BATT_NORMAL) {
		digitalWrite(Z_SCH_LAMPE_PIN, offon);
	} else {
		digitalWrite(Z_SCH_LAMPE_PIN, 0);
	}
}

void zPower::zapfLichtControl(uint8_t pwmValue) {
	if (bkPowerState >= powerState::BATT_NORMAL) {
		flowmeter.flowDataSend(LED_FUN_4, 0b11111111, pwmValue);
	} else {
		flowmeter.flowDataSend(LED_FUN_4, 0b11111111, 0);
	}
}

void zPower::autoLight(uint8_t offon) {
	autoLightBool = offon;
}

void zPower::goSleep(void) {
	/*
	 * hell = 200;

	 //Hier checken ob was gespielt wird, ansonsten audio aus
	 //audio.check();

	 if (hell < 6) {
	 dunkelCount++;
	 } else {
	 dunkelCount = 0;
	 }

	 if (dunkelCount > 9) {
	 //Nachtprogramm
	 DEBUGMSG("Nachtprogramm")
	 dunkelBool = true;

	 sound.mp3Play(12, 1); //Gute NAcht Freunde

	 for (int x = 0; x < 256; x++) {
	 analogWrite(lcdBacklightPwm, x);
	 delay(50);
	 }

	 for (int x = 255; x >= 0; x--) {
	 analogWrite(TASTE1_LED, x);
	 delay(50);
	 }

	 for (int x = 255; x >= 0; x--) {
	 analogWrite(TASTE2_LED, x);
	 delay(50);
	 }

	 //		for (int x = 255; x >= 0; x--) {
	 //			for (uint8_t pwmnum = 0; pwmnum < 13; pwmnum++) {
	 //				pwm.setPWM(pwmnum, 0, x); //Wählscheibe runterdimmen
	 //			}
	 //			delay(10);
	 //		}

	 flowmeter.flowDataSend(END_ZAPF, 0, 0);
	 flowmeter.flowDataSend(ZAPFEN_STREICH, 0, 0);
	 temp.sendeBefehl(ZAPFEN_STREICH, 0x0);
	 user.gesamtMengeTag = 0;
	 aktuellerTag++;
	 ventil.closeValve();
	 for (int x = 0; x <= 11; x++) {
	 ventil.check();
	 delay(1000);
	 }

	 delay(130000); //dann ist gute nach Freunde aus
	 sound.mp3Play(12, aktuellerTag); //lied 2-7
	 delay(240000);

	 digitalWrite(AUDIO_AMP, LOW);
	 digitalWrite(otherMcOn, LOW);
	 //Daten noch loggen

	 //Userdaten noch löschen
	 for (int x = 0; x <= 10; x++) {
	 user.bierTag[x] = 0;
	 }

	 while (dunkelBool == true) {
	 hell = analogRead(helligkeitSensor);
	 if (hell > 200) {
	 hellCount++;
	 delay(10000);
	 } else {
	 hellCount = 0;
	 }
	 if (digitalRead(TASTE1_PIN) == HIGH) {
	 dunkelBool = false;
	 delay(100);
	 anfang();
	 }
	 if (hellCount > 9) {
	 dunkelBool = false;
	 digitalWrite(AUDIO_AMP, HIGH);
	 digitalWrite(otherMcOn, HIGH);
	 delay(3000);   //pause machen damit die auch alle hochkommen
	 anfang();
	 }
	 }
	 }
	 */
}
