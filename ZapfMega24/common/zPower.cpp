/*
 * zPower.cpp
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 */

#include "zPower.h"
#include "gemein.h"
#include "globalVariables.h"
#include "Adafruit_PWMServoDriver.h"



zPower::zPower ()
{
	inVoltage = 120;
	helligkeit =0 ;
	bkPowerState = BATT_NORMAL;
	bkMachineState = WAKE_UP;
	millisSeitZapfEnde = 0;
	millisSeitLetztemCheck = 0;
	lampenOutput = 0;
	autoLightBool = 1;
}

zPower::~zPower ()
{
  /* Auto-generated destructor stub */
}

void zPower::begin() {
	pinMode(OTHER_MC_PIN, OUTPUT);
	digitalWrite(OTHER_MC_PIN, HIGH);

	pinMode(Z_SCH_LAMPE_PIN, OUTPUT);
	pinMode(LICHT_SENSOR_PIN, INPUT);
	pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
	analogWrite(LCD_BACKLIGHT_PIN, 128);
}

void zPower::check() {
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
   helligkeit = digitalRead(LICHT_SENSOR_PIN);
}

void zPower::setLed(uint8_t offon) {

}
void schLampeControl(uint8_t offon, uint16_t dimspeed) {

}

void zapfLichtControl(uint8_t offon, uint16_t dimspeed) {

}
void autoLight(uint8_t offon) {

}

void goSleep(void) {
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
