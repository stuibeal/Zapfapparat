// Do not remove the include below
#include "ZapfMega24.h"

#include "Arduino.h"
#include "gemein.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <Wire.h>
#include "Adafruit_Thermal.h"
#include <Adafruit_PWMServoDriver.h>   //PWM LED wählscheibe, VOR DEM DISPLAY includen!!!!!!!!!!!
#include "Adafruit_GFX.h"
#include "./zLibraries/MCUFRIEND_kbv/MCUFRIEND_kbv.h"
#include "zDisplay.h"
#include <SdFat.h>            // Use the SdFat library
#include "Encoder.h"  //für Drehencoder
#include "./zLibraries/RTC_DCF/DateTime.h"   // ELV RTC mit DCF
#include "./zLibraries/RTC_DCF/RealTimeClock_DCF.h"
#include "./zLibraries/zPrinter/zPrinter.h"
#include "benutzer.h"
#include "MD_MIDIFile.h"
#include "MD_YX5300.h"
#include "audio.h"
#include "zValve.h"
#include "zLog.h"
#include "zWireHelper.h"
#include "tempControl.h"

// Defines
#define USE_SDFAT
#define ENCODER_USE_INTERRUPTS  //damit die vom MEGA drin sind und er die als INT setzt
//#include <MD_cmdProcessor.h>   //weiss nicht ob wir den brauchen

#define DEBUGTO 3  //0 Nothing, 1 Serial, 2 Printer, 3 Display, 4 SD File
#if DEBUGTO == 3
#define DEBUGMSG(s) { ZD.printText(); ZD._tft.println(s); }
#endif
#ifndef DEBUG_A
#define DEBUG_A 1 //Debug Audio
#endif
// Defines aus

// Variablen
char buf[80];
bool oldFlowWindow;
bool flowWindow;

//Hier Variablen definieren
byte aktuellerTag = 1;  //dann gehts mit der Musik aus
unsigned int minTemp = 200;
unsigned int zielTemp = 200;
unsigned long auswahlZeit = 0;
int aktuellerModus = 0;
unsigned int hell;

unsigned long oldTime = millis();
unsigned long nachSchauZeit = 0;
unsigned long hellZeit = 0;
unsigned int hellCount = 0;
unsigned int dunkelCount = 0;
bool dunkelBool = false;

byte lichtan = LOW;

//Waehlscheibe
unsigned long kienmuehle = 0;  //Sondereingabe bei drücken der Taste2
byte Einsteller = 1; //Globale Variable für ISR, Start bei 1

bool ebiModeBool = false; //#define ebiMode               0xF9      //    1 an, 0 aus       Temperatur auf 2°C, Hahn auf, Zapfmusik
bool beginZapfBool = false; // #define beginZapf             0xFA      //    Beginn das Zapfprogramm -> PID auf aggressiv
bool endZapfBool = false; //#define endZapf               0xFB      //    Data send : milliliter
bool kurzBevorZapfEndeBool = false; //#define kurzBevorZapfEnde     0xFC      //    sagt das wir kurz vor Ende sind → Valve schließen -> PID auf konservativ

String dataOnSd = "";

bool debugMode = false;

// DREHENCODER
#define ENCODER_OPTIMIZE_INTERRUPTS
Encoder Dreher(ROTARY_DT_PIN, ROTARY_CLK_PIN); //PINS für Drehgeber

volatile int DreherKnopfStatus = 0; //Da wird der Statatus vom Drehgeberknopf gelesen
long oldPosition = 0; //Fuer Drehgeber

unsigned int tempAnzeigeZeit = millis(); //für zehnsekündige Temperaturanzeige

// Variablen aus
SdFat SD;  // SD-KARTE
zDisplay ZD;   // neues zDisplay Objekt
zWireHelper flowmeter;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
tempControl temp;	//Temperatursensorik
benutzer user;  //Benutzer
audio sound; 	//Audioobjekt
zValve ventil; // Ventilsteuerung, Druck, Reinigungspumpe
zPrinter drucker;
zLog logbuch;
MD_MIDIFile SMF;
MD_YX5300 mp3 = MD_YX5300(MP3Stream);

void setup(void) {
	//Erstmal bei den anderen MCs den Strom an
	pinMode(otherMcOn, OUTPUT);
	digitalWrite(otherMcOn, HIGH);

	//Pins herrichten
	pinMode(helligkeitSensor, INPUT);
	pinMode(lcdBacklightPwm, OUTPUT); //LCD Display Hintergrundbeleuchtung
	analogWrite(lcdBacklightPwm, 128);

	// Tasten in der Front
	pinMode(TASTE1_PIN, INPUT);
	pinMode(TASTE2_PIN, INPUT);
	pinMode(TASTE1_LED, OUTPUT);
	pinMode(TASTE2_LED, OUTPUT);
	analogWrite(TASTE1_LED, 10);
	analogWrite(TASTE2_LED, 10);

	// i2c etc
	flowmeter.initialise();

	//TFT
	ZD.beginn(&SD);  //mit Pointer zur SD starten

	//SD
	if (!SD.begin(SD_CS)) {  // nachschauen ob die SD-Karte drin und gut ist
		ZD.println("SD Karte nicht vorhanden! Bitte richten!");
		/***Hier funktion programmieren:
		 ZD.println("Ohne SD Karte fortfahren: Z-Knopf drücken!");
		 ***/
		return;   // don't do anything more if not
	} else {
		ZD.println("SD Karte bassd - OPTIMAL!");
	}

	ZD.showBMP("/bmp/z-logo.bmp", 200, 0);

	//Printer
	drucker.initialise(&Serial2, &user, &buf[0]);

	//Temperaturfuehler
	temp.begin(); //Wire sollte konfiguriert sein!
	ZD.println("Temperaturfuehler hochgefahren...");

	//FLOWMETER
	pinMode(FLOW_SM6020, OUTPUT);
	digitalWrite(FLOW_SM6020, HIGH);
	pinMode(FLOW_WINDOW, INPUT);    //Wenn durchfluss, dann true
	ZD.println("Flowmeter ifm SM6020 ein");

	//Rotary Encoder
	pinMode(ROTARY_SW_PIN, INPUT); // Drehgeberknopf auf Input
	attachInterrupt(digitalPinToInterrupt(ROTARY_SW_PIN),
			Einstellerumsteller_ISR, FALLING); //ISR= interrupt service routine; alternativ: (digitalPinToInterrupt(pin), ISR, mode)
	/*External Interrupts: 2 (interrupt 0), 3 (interrupt 1), 18 (interrupt 5), 19 (interrupt 4), 20 (interrupt 3), and 21 (interrupt 2).
	 These pins can be configured to trigger an interrupt on a low value, a rising or falling edge, or a change in value. See the attachInterrupt() function for details.
	 Den Schalter hardwaremäßig entprellt: 320k pullup, 10k pulldown und signal-kondensator-ground -> kondensator lädt und entprellt.
	 ABER: SDA/SCL ist parallel zu PIN 20/21, brauch den Interrupt also selber. So sind nur pins 2,3,18,19 am MEGA frei.
	 */
	//Aktueller Einstellmodus (1=Temperatur in °C*100, 2=Zapfmenge in ml
	//Wählscheibe
	pinMode(WSpuls, INPUT); // WSpuls auf Input (Interrupt)
	pinMode(WSready, INPUT);  //Wählscheibe Puls

	sound.starte(&SD, &SMF, &mp3);
	ZD.println("Harte Musik bereit");

	//Altdaten auslesen (SD karte) nach Stromweg oder so...

	//PWM Treiber hochfahren
	pwm.begin();
	pwm.setPWMFreq(1000);  // Maximale Frequenz (1kHz) -> reicht für LED
	pwm.setPWM(0, 0, 16);   //helle LEDS abdunkeln grün
	pwm.setPWM(11, 0, 16);  //helle LEDS abdunkeln weiß
	for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++) {
		pwm.setPWM(pwmnum, 0, 64); //alles leicht einschalten
	}
	ZD.println("PWM Waehlscheibe ready...");

	//Valve
	ventil.begin();
	ventil.check();   //dann sollte das aufgehen
	ZD.println("Ventilsteuerung aktiviert");

	//DCF RTC
	logbuch.initialise(&SD, &user, &temp, &buf[0]);
	ZD.println("RTC DCF77 aktiviert");

	//Make Windows 95 great again
	anfang();
	oldTime = millis();
	nachSchauZeit = millis();

}  //VOID SETUP

void waehlscheibe() {
	uint8_t zahlemann = 0;  //Per Wählscheibe ermittelte Zahl
	sound.setStandby(true); //dann checkt er nicht ob er ausschalten soll
	sound.on();
	DEBUGMSG(sound.debugmessage);
	auswahlZeit = millis();

	flowmeter.flowDataSend(LED_FUN_1, 10, 200);

	if (!digitalRead(TASTE2_PIN)) {
		//wenn man sich verwählt hat bei der Nummerneingabe wirds gelöscht
		kienmuehle = 0;

		ventil.openValve();
		analogWrite(TASTE2_LED, 10);
		//geh mal Zapfen
		temp.sendeBefehl(BEGIN_ZAPF, 0x0);
		//flowDataSend(LED_FUN_2, 1000,1000);

	} else {
		analogWrite(TASTE2_LED, 255);
		//hier eigentlich die Kienmühle funktion

	}
	sound.pruefe();
	DEBUGMSG(sound.debugmessage);

	for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++) {
		pwm.setPWM(pwmnum, 0, 256); //Licht AN
	}
	bool old_waehler2 = 1;
	bool waehler2 = digitalRead(WSpuls);
	unsigned long temptime = 0;

	while (digitalRead(WSready) == 1) {

		old_waehler2 = waehler2;
		waehler2 = digitalRead(WSpuls);


		if (waehler2 < old_waehler2) {
			temptime = millis();  //hier die Wählscheibe auslesen
		}

		if ((waehler2 > old_waehler2) && (millis() - temptime > 50)) { //wenn Signal wieder von 0V auf 5V geht und mehr als 50ms vergangen sind, eins hochzählen
			zahlemann++; //Wählscheibe (US): 60ms PULS 0V, 40ms Pause (5V), ánsonsten immer 5V
			temptime = millis();
			pwm.setPWM(zahlemann, 4096, 0);
			if (zahlemann > 1) {
				pwm.setPWM(zahlemann - 1, 0, 512);
			}
		}
	}

	flowmeter.flowDataSend(GET_ML, 0, 0);  //LEDFun ausschalten

	sound.pruefe();
	DEBUGMSG(sound.debugmessage);

	if (zahlemann > 0) {
		for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++) {
			pwm.setPWM(pwmnum, 0, 64); //Licht aus
		}
		pwm.setPWM(zahlemann, 4096, 0);

		//Ab hier werden die Userdaten angezeigt.
		if ((zahlemann < 11) && !digitalRead(TASTE2_PIN)) {
			if (zahlemann == 10) {
				zahlemann = 0; // user beim "nuller"
			}
			user.aktuell = zahlemann;
			flowmeter.flowDataSend(SET_USER_ML, user.menge());
			delay(500); //damit der Zeit hat
			flowmeter.flowDataSend(BEGIN_ZAPF, 0, 0);

			switch (user.getGodMode()) {
			case 1:
				sound.loadLoopMidi("d_runni2.mid");
				break;
			case 2:
				sound.loadLoopMidi("keen.mid");
				break;
			}

			if (user.temp() > minTemp) {
				zielTemp = user.temp();
			} else {
				zielTemp = minTemp;
			}

			userShow();  // Zeigt die Userdaten an
			auswahlZeit = millis();

			beginZapfBool = true;

		}

		/*!
		 * Addiert die gewählten Zahlen zu kienmuehle dazu
		 */
		if ((digitalRead(TASTE2_PIN)) > 0 && zahlemann > 0) {
			kienmuehle = kienmuehle * 10;  //das passt so, Alfred!
			if (zahlemann < 10) {
				kienmuehle += zahlemann;
			}
			sprintf(buf, "Nr: %9lu", kienmuehle);
			ZD.infoText(buf);
		}

	}
	//Erst am schluss das MIDI RESET aufheben!

	sound.setStandby(beginZapfBool); //wenn er nicht zapft, kein Standby!
	sound.pruefe();
	DEBUGMSG(sound.debugmessage);
}

void waehlFunktionen() {
	switch (kienmuehle) {
	case 847: // UHRZEIT
		logbuch.getClockString();
		ZD.printText();
		ZD._tft.println(buf);
		break;
	case 463633: //GODOFF
		user.setGodMode(0);
		ZD.userShow(&user);
		break;
	case 43373:
		user.setGodMode(IDDQD);
		ZD.userShow(&user);
		break;
	case 5336:
		user.setGodMode(KEEN);
		ZD.userShow(&user);
		break;
	case 1275: //Die Telefonnummer der Kienmühle
		oldWaehlscheibeFun();
		break;
	case 9413: //Telefonnummer
		reinigungsprogramm();
		break;
	case 25326: //clean
		reinigungsprogramm();
		break;
	case 75337: //sleep
		// do power.goSleep();
		break;
	default:
		spezialprogramm(kienmuehle);
		break;
	}
	kienmuehle = 0;
}

void anfang(void) {
	//mp3.playTrack(3); //Brantl Edel Pils
	//ZD._tft.fillScreen(BLACK);

	ZD.showBMP("/bmp/back01.bmp", 0, 0);

	analogWrite(TASTE2_LED, 10);
	analogWrite(TASTE1_LED, 10);
	analogWrite(lcdBacklightPwm, 20);
	for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++) {
		pwm.setPWM(pwmnum, 0, 64); //alles leicht einschalten
	}
	pwm.setPWM(0, 0, 16);   //helle LEDS abdunkeln grün
	pwm.setPWM(11, 0, 16);  //helle LEDS abdunkeln weiß
	userShow();
	sound.bing();

}

void aufWachen(void) {
	// nothing
}

void einSchlafen(void) {
	//nothing
}

void tickMetronome(void)
// flash a LED to the beat
		{
	static uint32_t lastBeatTime = 0;
	static boolean inBeat = false;
	static uint8_t leuchtLampe = B00000001;
	uint16_t beatTime;

	beatTime = 60000 / sound._SMF->getTempo() / 4; // msec/beat = ((60sec/min)*(1000 ms/sec))/(beats/min)
	if (!inBeat) {
		if ((millis() - lastBeatTime) >= beatTime) {
			lastBeatTime = millis();

			inBeat = true;
			if (leuchtLampe & 1) {
				digitalWrite(TASTE1_LED, HIGH);
			} else {
				digitalWrite(TASTE2_LED, HIGH);
			}

			//ZD.setCursor(5, 30);
			//sprintf(buf, "leuchtLampe: %02x ", leuchtLampe);
			flowmeter.flowDataSend(LED_FUN_4, leuchtLampe, 0xFF);
			//flowDataSend(LED_FUN_1, 1, 70);
			//leuchtLampe << 1;
			leuchtLampe++;

		}
	} else {
		if ((millis() - lastBeatTime) >= 100) // keep the flash on for 100ms only
				{

			if (!(leuchtLampe & 1)) {
				analogWrite(TASTE1_LED, 20);
			} else {
				analogWrite(TASTE2_LED, 20);
			}

			inBeat = false;

		}
	}
}

void seltencheck(void) {
	//sprintf(buf, "hell: %d dunkelcount %d", hell, dunkelCount);
	//DEBUGMSG(buf);

	hell = analogRead(helligkeitSensor);

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

		for (int x = 255; x >= 0; x--) {
			for (uint8_t pwmnum = 0; pwmnum < 13; pwmnum++) {
				pwm.setPWM(pwmnum, 0, x); //Wählscheibe runterdimmen
			}
			delay(10);
		}
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

	temp.requestSensors();
	temp.holeDaten();

}
//ZD.print_val(zulauf.getTempC() * 100, 200, 170, 1, 1);

void belohnungsMusik() {
	if (user.tag() > 2000 && user.getMusik() == 0) {
		user.setMusik(1);
		sound.mp3Play(user.aktuell, 1);
	}
	if (user.tag() > 2500 && user.getMusik() == 1) {
		user.setMusik(2);
		sound.mp3Play(user.aktuell, 2);
	}
	if (user.tag() > 3000 && user.getMusik() == 2) {
		user.setMusik(3);
		sound.mp3Play(user.aktuell, 3);
	}
	if (user.tag() > 3500 && user.getMusik() == 3) {
		user.setMusik(0);
		sound.mp3Play(user.aktuell, 4);
		ZD.showBMP("/bmp/back02.bmp", 0, 0);
		userShow();
	}

}

//Infoknopf
void infoseite(void) {
	analogWrite(TASTE1_LED, 10);
	sound.loadSingleMidi("SKYFALL.MID");
	sound._SMF->pause(false);
	ZD.infoscreen(&temp, &user);

	//ZD.setFont(&FreeSans9pt7b);

	for (int x = 10; x < 256; x++) {
		analogWrite(TASTE1_LED, x);
		delay(10);
	}
	while (digitalRead(TASTE1_PIN)) {
	}
	for (int x = 255; x > 11; x--) {
		analogWrite(TASTE1_LED, x);
		delay(10);
	}
	anfang();
	userShow();

}

void loop() {
	byte oldeinsteller = Einsteller;
	sound.midiNextEvent();

	Drehgeber();

	//Valve Control
	ventil.check();

	//Wenn die Wählscheibe betätigt wird
	if (digitalRead(WSready)) {
		waehlscheibe();
	}

	//Wenn was rumgestellt wird
	if (oldeinsteller != Einsteller) {
		UserDataShow();
	}

	//Wenn jemand an den Tasten rumspielt
	if (digitalRead(TASTE1_PIN)) {
		infoseite();
	}

	// Wenn Nummer Fertig und Taste losgelassen
	if (!digitalRead(TASTE2_PIN) && kienmuehle > 0) {
		analogWrite(TASTE2_LED, 20);
		waehlFunktionen();
	}

	if (user.getGodMode() == 0)  //sound._SMF.->isPaused () ||
			{

		if ((millis() - oldTime) > 1000) {
			oldTime = millis();
			anzeigeAmHauptScreen();
			sound.pruefe();
			if (DEBUG_A) {
				DEBUGMSG(sound.debugmessage);
			}

			//RTC_DCF.getDateTime(&dateTime);
			//printClock();
		}

		if ((millis() - nachSchauZeit) > 10000 && !beginZapfBool) {
			seltencheck();
			nachSchauZeit = millis();

		}
	}

	if (beginZapfBool) {
		if (user.getGodMode() > 0) {
			oldFlowWindow = flowWindow;
			flowWindow = digitalRead(FLOW_WINDOW);
			if (oldFlowWindow == true && flowWindow == false) {
				sound._SMF->pause(true);
			}
			if (oldFlowWindow == false && flowWindow == true) {
				sound._SMF->pause(false);
			}
			if (!sound._SMF->isPaused()) {
				tickMetronome();
			}
		}

		flowmeter.flowDataSend(GET_ML, 0, 0); // aktuelle ml vom Flow uC abfragen

		// Nachschaun ob er fertig ist und dann bingen und zamschreim
		if (flowmeter.getMilliliter() >= user.menge() || digitalRead(TASTE2_PIN)) {
			if (user.getGodMode() == 1) {
				ZD.showBMP("/god/11.bmp", 300, 50);
			}
			sound._SMF->close();
			sound.midiSilence();
			ventil.check();
			sound.bing();

			//Sollte er abgebrochen haben:
			if (flowmeter.getMilliliter() < user.menge()) {
				drucker.printerErrorZapfEnde(flowmeter.getMilliliter());
			}

			user.addBier(flowmeter.getFreshZapfMillis());

			drucker.printerZapfEnde(flowmeter.getMilliliter());
			UserDataShow();
			beginZapfBool = false;
			sound.setStandby(beginZapfBool);
			dataLogger();
			belohnungsMusik();

		}

		// Nachschaun ob er eventuell zu lang braucht und nix zapft
		if (((millis() - auswahlZeit) > 10000) && (flowmeter.getMilliliter() < 5)) {
			beginZapfBool = false;
			sound.setStandby(beginZapfBool);
			temp.sendeBefehl(END_ZAPF, 0x0);
			ventil.check();
			for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++) {
				pwm.setPWM(pwmnum, 0, 64); //alles leicht einschalten
			}
		}

		if ((user.menge() - flowmeter.getMilliliter()) < 30) {
			temp.sendeBefehl(KURZ_VOR_ZAPFENDE, 0x0);
			ventil.check();
		}
	}

	//Check Knöpfe (Benutzer) -> Display up -> Zapfprogramm

	//Zwischendurch mal was protokollieren (alle 5 minuten oder so)
}

void userShow(void) {
	Einsteller = 2; //Wieder bei mL beginnen beim Drehknebel
	ZD.userShow(&user);
	UserDataShow();
}

void anzeigeAmHauptScreen(void) {
	//DEBUGMSG("vor transmitBlocktemp");
	//ZD.print_val2 (temp.getBlock1Temp (), 20, 100, 3, 1);
	//temp.holeDaten();
	//ZD.printVal(temp.getBlockAussenTemp(), 25, 100, WHITE, ZDUNKELGRUEN, &FETT,	KOMMA);
	//DEBUGMSG("vor transmitauslauf");
	//DEBUGMSG("vor transmitauslauf");
	ZD.print_val2(temp.getBlockAussenTemp(), 20, 125, 1, 1);
	ZD.print_val2((int)flowmeter.getFreshZapfMillis(), 20, 150, 1, 0);
	//ZD.printText();
	ZD.print_val2((int)ventil.getPressure(), 20, 175,1,0);

}

/* Name:			dataLogger
 * Beschreibung:	Diese Funktion schreibt die aktuellen Daten auf die SD Karte
 *
 */
void dataLogger(void) {
	// TBD: other files müssen alle zu sein
	// jeden Tag ein File, ein gesamtfile

	/*
	 //Zeit einlesen
	 RTC_DCF.getDateTime (&dateTime);

	 char fileBuf[20] = "";
	 String dataString = "";
	 sprintf (fileBuf, "LOG_%02u%02u%2u.csv", dateTime.getDay (),
	 dateTime.getMonth (), dateTime.getYear ());

	 if (!SD.exists (fileBuf))
	 {
	 //Wenn Datei noch nicht vorhanden, Kopfzeile schreiben!
	 dataString = "Datum Zeit, ";
	 for (uint8_t x = 0; x < 10; x++)
	 {
	 dataString += user.username[x];
	 dataString += ", ";
	 }
	 dataString += "Gesamtmenge, ";
	 dataString += "Batterie-Volt, ";
	 dataString += "Helligkeit";

	 File dataFile = SD.open (fileBuf, FILE_WRITE);
	 // if the file is available, write to it:
	 if (dataFile)
	 {
	 dataFile.println (dataString);
	 dataFile.close ();
	 }
	 // if the file isn't open, pop up an error:
	 else
	 {
	 //Serial.println("konnte z-log.csv nicht öffnen");
	 }
	 }

	 // Daten schreiben
	 char timeBuf[20] = "00.00.00 00:00:00, ";
	 sprintf (timeBuf, "%02u.%02u.%02u %02u:%02u:%02u, ", dateTime.getDay (),
	 dateTime.getMonth (), dateTime.getYear (), dateTime.getHour (),
	 dateTime.getMinute (), dateTime.getSecond ());
	 File dataFile = SD.open (fileBuf, FILE_WRITE);

	 dataString = timeBuf;

	 for (uint8_t x = 0; x < 11; x++)
	 {
	 dataString += String (user.tag ());
	 dataString += ",";
	 }
	 dataString += String (user.gesamtMengeTag);
	 dataString += ",";
	 dataString += String (inVoltage);
	 dataString += ",";
	 dataString += String (hell);

	 // if the file is available, write to it:
	 if (dataFile)
	 {
	 dataFile.println (dataString);
	 dataFile.close ();
	 }
	 // if the file isn't open, pop up an error:
	 else
	 {
	 //Serial.println("konnte z-log.csv nicht öffnen");
	 }
	 */
}

void UserDataShow() {
	int x = 400;
	int y = 210;
	switch (Einsteller) {
	case 1:
		ZD.print_val(user.temp(), x, y, 1, 1);
		ZD.print_val(user.menge(), x, y + 30, 0, 0);
		ZD.print_val(user.tag() / 5, x, y + 60, 0, 1);
		ZD.print_val(user.gesamtMengeTag / 10, x, y + 90, 0, 1);
		break;
	case 2:
		ZD.print_val(user.temp(), x, y, 0, 1);
		ZD.print_val(user.menge(), x, y + 30, 1, 0);
		ZD.print_val(user.tag() / 5, x, y + 60, 0, 1);
		ZD.print_val(user.gesamtMengeTag / 10, x, y + 90, 0, 1);
		break;
	}
}

void Drehgeber() {
	int x = 400;
	int y = 210;

	long newPosition = Dreher.read();
	if (newPosition != oldPosition) {
		switch (Einsteller) {
		case 1:
			user.setTemp(user.temp() + (newPosition - oldPosition));
			if ((user.temp() < minTemp)) {
				user.setTemp(minTemp);
			}
			ZD.print_val(user.temp(), x, y, 1, 1);
			break;
		case 2:
			user.setMenge(user.menge() + (newPosition - oldPosition));
			if (user.menge() <= 20) {
				user.setMenge(20);
			}
			ZD.print_val(user.menge(), x, y + 30, 1, 0);
			break;
		case 3:
			if (newPosition < oldPosition) {
			} else {
			}
			ZD.print_val(user.tag(), x, y + 60, 2, 0);
			break;
		}

		oldPosition = newPosition;

	}
}

void Einstellerumsteller_ISR() { //Interruptroutine, hier den Drehgeberknopf abfragen
	DreherKnopfStatus = digitalRead(ROTARY_SW_PIN);
	Einsteller++;
	if (Einsteller > 2) {
		Einsteller = 1;
	}
	UserDataShow();
}

void oldWaehlscheibeFun(void) {
	sound.mp3Play(11, 1); //Magnum
	pwm.setPWM(0, 0, 2048);   //Grüne LED an
	for (uint8_t dw = 0; dw < 2; dw++) { //mega Lightshow!!
		for (uint16_t i = 0; i < 11; i++) {
			delay(50);
			for (uint8_t x = 11; x > 0; x--) {
				delay(30);
				pwm.setPWM(x + i, 4096, 0);
				pwm.setPWM(x + i + 1, 0, 4096);
			}
		}

		for (uint16_t i = 11; i > 0; i--) {
			delay(100 % i);
			for (uint8_t x = 0; x < 11; x++) {
				delay(50);
				pwm.setPWM(x + i - 1, 0, 4096);
				pwm.setPWM(x + i, 4096, 0);
			}
		}
		delay(500);
	}
	pwm.setPWM(11, 0, 16);  //helle LEDS abdunkeln weiß
	pwm.setPWM(0, 0, 16);   //helle LEDS abdunkeln grün

}

void reinigungsprogramm(void) {
	//hier braces damit die variablen allein hier sind
	ZD.printText();
	ZD._tft.println("        REINIGUNGSPROGRAMM");
	temp.sendeBefehl(ZAPFEN_STREICH, 0x0);
	ventil.cleanPumpOn();
	delay(500); //zum taste loslassen!
	unsigned long waitingTime = millis();
	int sekunden = 0;
	while (!digitalRead(TASTE1_PIN)) {
		if (millis() - waitingTime >= 1000) {
			waitingTime = millis();
			sekunden++;
			ZD.printText();
			ZD._tft.println(sekunden);
		}

	}
	ventil.cleanPumpOff();
	ZD.printText();
	ZD._tft.println("ENDE REINIGUNGSPROGRAMM      ");

}

void spezialprogramm(uint32_t input) {
	uint16_t varSet = 0;
	uint16_t varContent = 0;
	if (input > 999999 && input < 10000000) {
		varSet = input / 100000; //2 x Zahl VarSet
		varContent = input % 100000;  //5x varContent
	} else if (input > 99999 && input < 1000000) {
		varSet = input / 10000; //zwei Zahl Varset
		varContent = input % 10000;  //4x Content (00-9999)
	} else if (input > 9999 && input < 100000) {
		varSet = input / 1000;  //2x zahl Varset
		varContent = input % 1000;  //drei Zahlen Content
	} else if (input > 999 && input < 10000) {
		varSet = input / 100;  //2x zahl Varset
		varContent = input % 100;  //zwei  Zahlen Content
	} else if (input > 99 && input < 1000) {
		varSet = input / 100;  //1x zahl Varset
		varContent = input % 100;  //zwei  Zahlen Content
	}
	switch (varSet) {
	case 0:
		//play dööh dööh dööh
		break;
	case 9: //Mediaplayer
		sound.mp3Play(11, varContent);
		break;
	case 99: // auch Mediaplayer
		sound.mp3Play(11, varContent);
		break;
	}
}
