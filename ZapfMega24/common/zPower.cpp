/*
 * zPower.cpp
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 */

#include "zPower.h"

#include "../zDisplay.h"
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
	lightIsOn = 0;
	oldLightIsOn = 0;
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
	analogWrite(LCD_BACKLIGHT_PIN, 0);
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
		if (helligkeit > 100) {
			helligkeit = 100;
		}
		autoLight();
		lcdAutoLight();
	}

}

void zPower::tastenLed(uint8_t taste, uint8_t helligkeit) {
	if (bkPowerState <= powerState::BATT_NORMAL) {
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

void zPower::setLed(uint8_t ledNr, bool on) {
	uint16_t pwm = 0;
	if (on) {
		pwm = map(helligkeit, 1, 100, 10, 0xFFF);
	} else {
		pwm = map(helligkeit, 1, 100, 3, 400);
	}

	wsLed.setPWM(ledNr, pwm);

}

void zPower::setAllWSLed(uint16_t helligkeit) {
	for (int i = 0; i < 12; i++) {
		wsLed.setPWM(i, helligkeit);
	}
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
		setLed(channel, 0);
	}
	//weiß
	setWhiteLed(WEISS_LED_ABGEDUNKELT);
}

void zPower::schLampeControl(uint8_t offon) {
	if (bkPowerState <= powerState::BATT_NORMAL) {
		digitalWrite(Z_SCH_LAMPE_PIN, offon);
	} else {
		digitalWrite(Z_SCH_LAMPE_PIN, 0);
	}
}

void zPower::zapfLichtControl(uint8_t pwmValue) {
	if (bkPowerState <= powerState::BATT_NORMAL) {
		flowmeter.flowDataSend(LED_FUN_4, 0b11111111, pwmValue);
	} else {
		flowmeter.flowDataSend(LED_FUN_4, 0b11111111, 0);
	}
}

void zPower::autoLight(void) {
	if (autoLightBool) {
		uint8_t ledBrightness = LAMPE_AN - helligkeit + 10;
		oldLightIsOn = lightIsOn;
		if (helligkeit < LAMPE_AN) {
			lightIsOn = true;
		}

		if (helligkeit > LAMPE_AUS) {
			lightIsOn = false;
		}

		if (oldLightIsOn != lightIsOn) {
			if (lightIsOn) {
				schLampeControl(1);
				zapfLichtControl(ledBrightness);
			} else {
				schLampeControl(0);
				zapfLichtControl(0);
			}
		}
	}
}

void zPower::lcdAutoLight(void) {
	if (autoLightBool) {
		if (helligkeit > 100) {
			analogWrite(LCD_BACKLIGHT_PIN, 0);
		} else if (helligkeit > 20 && helligkeit < 101) {
			analogWrite(LCD_BACKLIGHT_PIN, 200 - helligkeit * 2);
		} else {
			analogWrite(LCD_BACKLIGHT_PIN, 240);
		}

	} else {
		analogWrite(LCD_BACKLIGHT_PIN, 127);
	}
}

void zPower::setBackLight(void) {
	flowmeter.flowDataSend(GET_ML, 0, 0);
	delay(30);
	uint8_t ledBrightness = 0;
	if (lightIsOn) {
		ledBrightness = 10;
	}
	flowmeter.flowDataSend(DIM_LED_TO_WERT, 127, ledBrightness); //LEDFun ausschalten
}
void zPower::dimLight(uint16_t lichtpin, uint16_t anfang, uint16_t ende,
		uint16_t delaytime) {
	if (anfang < ende) {
		for (uint16_t x = anfang; x < ende + 1; x++) {
			if (lichtpin > 0) {
				if (lichtpin == 1) {
					zapfLichtControl(x);
				} else {
					analogWrite(lichtpin, x);
				}
			} else {
				setAllWSLed(x * 4);
			}
			delay(delaytime);
		}
	} else {
		for (uint16_t x = anfang; x > ende - 1; x--) {
			if (lichtpin > 0) {
				if (lichtpin == 1) {
					zapfLichtControl(x);
				} else {
					analogWrite(lichtpin, x);
				}
			} else {
				setAllWSLed(x * 4);
			}
			delay(delaytime);
		}
		if (lichtpin > 0) {
			analogWrite(lichtpin, ende);
		} else {
			setAllWSLed(ende);
		}
	}

}

void zPower::goSleep(void) {
	digitalWrite(Z_SCH_LAMPE_PIN, 1);
	sound.setStandby(1);
	sound.mp3ClearPlaylist();
	sound.playStop();
	sound.on();
	while (sound.pruefe() != sound.AUDIO_STANDBY) {
	}
	ZD.infoText(buf);
	ZD.fillScreen(BLACK);
	ZD.showBMP(F("/bmp/DOOM02.bmp"), 80, 60); //320x200
	switch (logbuch.getWochadog()) {
	case logbuch.MODA:
		sound.mp3AddToPlaylist(23, 4); //USA
		sound.mp3AddToPlaylist(28, 1); //Anne 10 Mark
		sound.mp3AddToPlaylist(23, 1); //Gute nach Freunde
		break;
	case logbuch.ERDA:
		sound.mp3AddToPlaylist(23, 5); //Luxemburg
		sound.mp3AddToPlaylist(28, 2); //Anne de Henn
		sound.mp3AddToPlaylist(23, 1); //Gute nach Freunde
		break;
	case logbuch.MIGGA:
		sound.mp3AddToPlaylist(23, 6); //Deutschlandlied
		sound.mp3AddToPlaylist(29, 8); //berger unanagenehmer teil
		sound.mp3AddToPlaylist(23, 1); //Gute nach Freunde
		break;
	case logbuch.PFINSDA:
		sound.mp3AddToPlaylist(23, 9); //DDR
		sound.mp3AddToPlaylist(23, 1); //Gute nach Freunde
		sound.mp3AddToPlaylist(23, 7); //Grossvater
		break;
	case logbuch.FREIDA:
		sound.mp3AddToPlaylist(23, 2); //Bayernhymne
		sound.mp3AddToPlaylist(23, 1); //Gute nach Freunde
		break;
	case logbuch.SAMSDA:
		sound.mp3AddToPlaylist(23, 2); //Bayernhymne
		sound.mp3AddToPlaylist(28, 3); //Anne De weiber
		sound.mp3AddToPlaylist(23, 1); //Gute nach Freunde
		break;
	case logbuch.SUNDA:
		sound.mp3AddToPlaylist(23, 3); //EUROPA
		sound.mp3AddToPlaylist(29, 1); //Berger da fällt mir im moment nix ein
		sound.mp3AddToPlaylist(23, 1); //Gute nach Freunde
		break;
	}
	dimLight(LCD_BACKLIGHT_PIN, 255, 0, 20);
	temp.sendeBefehl(ZAPFEN_STREICH, 0);
	dimLight(TASTE1_LED, TASTEN_LED_NORMAL, 255, 30);
	dimLight(TASTE2_LED, TASTEN_LED_NORMAL, 255, 30);
	digitalWrite(FLOW_SM6020, 0);
	dimLight(0, GRUEN_LED_ABGEDUNKELT / 4, 255, 3);
	drucker.schaltAus();
	dimLight(1, 0, 255, 15); //0: WS Led 1: Zapflicht
	ZD.showBMP(F("/bmp/DOOM02b.bmp"), 80, 60); //320x200

	bool playNotGNF = 1;
	do {
		sound.pruefe();
		if (sound.getPlFolder() == 23 && sound.getPlSong() == 1) {
			playNotGNF = 0;
		}
		//ZD.infoText(buf);
		delay(100);

	} while (playNotGNF);
	ZD.infoText(F("Gute Nacht, Freunde!"));
	digitalWrite(Z_SCH_LAMPE_PIN, 0);
	user.clearDayUserData();
	user.writeDataToEEPROM();
	ventil.closeValve();
	do {
		ventil.check();
		sprintf(buf, "%d", ventil.getValveProzent());
		ZD.infoText(buf);
	} while (ventil.getValveProzent() > 0);

	sprintf(buf, "%d", ventil.getValveProzent());
	ZD.infoText(buf);
	delay(2000);

	logbuch.disableDCF77LED();
	dimLight(LCD_BACKLIGHT_PIN, 0, 255, 30);
	analogWrite(LCD_BACKLIGHT_PIN, 255);
	dimLight(TASTE1_LED, 255, 0, 60);
	dimLight(TASTE2_LED, 255, 0, 60);
	dimLight(0, 255, 0, 30);
	dimLight(1, 255, 0, 120); //0: WS Led 1: Zapflicht
	zapfLichtControl(0);
	flowmeter.flowDataSend(ZAPFEN_STREICH, 0);

	do {
		sound.pruefe();
		delay(200);

	} while (sound.getPlaylistSize() > 0);
	digitalWrite(OTHER_MC_PIN, 0);
	sound.off();
	sound.setStandby(0);
	sound.mp3D.playTheList = 0;
	while (sound.pruefe() != sound.AUDIO_OFF) {
	}
	sound.stromAus();
	autoLightBool = 0;
	uint8_t sleeping = 1;
	do {
		check();
		oldeinsteller = einsteller;
		if (helligkeit > 20) {
			sleeping = 0;
		}
		if (oldeinsteller != einsteller) {
			sleeping = 0;
		}
		if (digitalRead(TASTE2_PIN) || digitalRead(TASTE1_PIN)) {
			sleeping = 0;
		}
	} while (sleeping);
	digitalWrite(OTHER_MC_PIN, 1);
	digitalWrite(FLOW_SM6020, 1);
	logbuch.enableDCF77LED();
	autoLightBool = 1;
	sound.stromAn();

	delay(1000);
	analogWrite(LCD_BACKLIGHT_PIN, 0); //0: voll hell
	anfang();
	ZD.infoText(F("Lust auf ein Frühbierchen?"));

//	//Daten noch loggen
//
//	while (dunkelBool == true) {
//		hell = analogRead(helligkeitSensor);
//		if (hell > 200) {
//			hellCount++;
//			delay(10000);
//		} else {
//			hellCount = 0;
//		}
//		if (digitalRead(TASTE1_PIN) == HIGH) {
//			dunkelBool = false;
//			delay(100);
//			anfang();
//		}
//		if (hellCount > 9) {
//			dunkelBool = false;
//			digitalWrite(AUDIO_AMP, HIGH);
//			digitalWrite(otherMcOn, HIGH);
//			delay(3000);   //pause machen damit die auch alle hochkommen
//			anfang();
//		}
//	}
//}

}
