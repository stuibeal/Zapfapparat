// Do not remove the include below
#include "ZapfMega24.h"

#include "Arduino.h"
#include "gemein.h"
#include "globalVariables.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <Wire.h>
#include <SdFat.h>            // Use the SdFat library
#include "waehlscheibe.h"
#include "Adafruit_Thermal.h"
#include "Adafruit_GFX.h"
#include "./zLibraries/MCUFRIEND_kbv/MCUFRIEND_kbv.h"
#include "U8g2_for_Adafruit_GFX.h"
#include "zDisplay.h"
#include "Encoder.h"  //für Drehencoder
#include "./zLibraries/RTC_DCF/DateTime.h"   // ELV RTC mit DCF
#include "./zLibraries/RTC_DCF/RealTimeClock_DCF.h"
#include "./zLibraries/zPrinter/zPrinter.h"
#include "benutzer.h"
#include "MD_MIDIFile.h"
#include "MD_YX5300.h"
#include "tempControl.h"
#include "audio.h"
#include "zValve.h"
#include "zLog.h"
#include "zPower.h"
#include "zWireHelper.h"

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

//Hier Variablen definieren
unsigned long auswahlZeit = 0;

unsigned long oldTime = millis();
unsigned long nachSchauZeit = 0;

//Waehlscheibe
unsigned long kienmuehle = 0;  //Sondereingabe bei drücken der Taste2
byte Einsteller = 1; //Globale Variable für ISR, Start bei 1

bool ebiModeBool = false; //#define ebiMode               0xF9      //    1 an, 0 aus       Temperatur auf 2°C, Hahn auf, Zapfmusik
bool beginZapfBool = false; // #define beginZapf             0xFA      //    Beginn das Zapfprogramm -> PID auf aggressiv
bool endZapfBool = false; //#define endZapf               0xFB      //    Data send : milliliter
bool kurzBevorZapfEndeBool = false; //#define kurzBevorZapfEnde     0xFC      //    sagt das wir kurz vor Ende sind → Valve schließen -> PID auf konservativ

// DREHENCODER
#define ENCODER_OPTIMIZE_INTERRUPTS
Encoder Dreher(ROTARY_DT_PIN, ROTARY_CLK_PIN); //PINS für Drehgeber

volatile int DreherKnopfStatus = 0; //Da wird der Statatus vom Drehgeberknopf gelesen
long oldPosition = 0; //Fuer Drehgeber

unsigned int tempAnzeigeZeit = millis(); //für zehnsekündige Temperaturanzeige

// Variablen aus

// Globale Variablen erstmalig hier initialisieren
char buf[80];
audio sound; 	//Audioobjekt
benutzer user;  //Benutzer
tempControl temp;	//Temperatursensorik

// Variablen für hier
SdFat SD;  // SD-KARTE
zDisplay ZD;   // neues zDisplay Objekt
zWireHelper flowmeter;
zValve ventil; // Ventilsteuerung, Druck, Reinigungspumpe
zPrinter drucker;
zLog logbuch;
MD_MIDIFile SMF;
MD_YX5300 mp3 = MD_YX5300(MP3Stream);
zPower power;

void setup(void) {
	power.begin(); /* STROM AN */
	flowmeter.initialise(); /* HIER i2c begin!*/
	ZD.beginn(&SD); /* Display mit Pointer zur SD starten */

	beginWaehlscheibeLed(); /* WS und Tastenpins */
	ZD.printInitText("Wählscheibe ready");

	//SD
	if (!SD.begin(SD_CS)) {  // nachschauen ob die SD-Karte drin und gut ist
		ZD.printInitText("SD Karte Error!");
		ZD.printInitText("Knoppen drucken");
		ZD.printInitText("um ohne zu laden");
		errorLed();
		return;   // don't do anything more if not
	} else {
		ZD.printInitText("SD Karte OPTIMAL!");
	}
	ZD.showBMP("/bmp/z-logo.bmp", 20, 20);

	drucker.initialise(); /* Thermodrucker */

	temp.begin(); /* Temperaturcontrol uC - Wire sollte gestartet sein! */
	ZD.printInitText("Temperaturfühler...");

	//FLOWMETER
	pinMode(FLOW_SM6020, OUTPUT);
	digitalWrite(FLOW_SM6020, HIGH);
	pinMode(FLOW_WINDOW, INPUT);    //Wenn durchfluss, dann true
	ZD.printInitText("Flowmeter ifm SM6020");

	//Rotary Encoder
	pinMode(ROTARY_SW_PIN, INPUT); // Drehgeberknopf auf Input
	attachInterrupt(digitalPinToInterrupt(ROTARY_SW_PIN),
			Einstellerumsteller_ISR, FALLING);
	/*External Interrupts: 2 (interrupt 0), 3 (interrupt 1), 18 (interrupt 5), 19 (interrupt 4), 20 (interrupt 3), and 21 (interrupt 2).
	 These pins can be configured to trigger an interrupt on a low value, a rising or falling edge, or a change in value. See the attachInterrupt() function for details.
	 Den Schalter hardwaremäßig entprellt: 320k pullup, 10k pulldown und signal-kondensator-ground -> kondensator lädt und entprellt.
	 ABER: SDA/SCL ist parallel zu PIN 20/21, brauch den Interrupt also selber. So sind nur pins 2,3,18,19 am MEGA frei.
	 */

	sound.starte(&SD, &SMF, &mp3);
	ZD.printInitText("Harte Musik ok");

	//Altdaten auslesen (SD karte) nach Stromweg oder so...

	//Valve
	ventil.begin();
	ventil.check();   //dann sollte das aufgehen
	ZD.printInitText("Ventilsteuerung");

	//DCF RTC
	logbuch.initialise(&SD, &user, &temp, buf);
	ZD.printInitText("RTC DCF77");

	//Make Windows 95 great again
	anfang();
	oldTime = millis();
	nachSchauZeit = millis();

}  //VOID SETUP

void waehlscheibe() {
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

	uint8_t zahlemann = readWaehlscheibe();

	flowmeter.flowDataSend(GET_ML, 0, 0);  //LEDFun ausschalten

	sound.pruefe();
	DEBUGMSG(sound.debugmessage);

	if (zahlemann > 0) {

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
	wsLedGrundbeleuchtung();
	userShow();
	sound.bing();
}

void aufWachen(void) {
	// nothing
}

void einSchlafen(void) {
	//nothingvoid g

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
	temp.requestSensors();

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

void godModeZapfMidi() {
	if (user.getGodMode() > 0) {
		static uint8_t oldFlowWindow;
		static uint8_t flowWindow;
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
}

void beginnZapfProgramm() {
	godModeZapfMidi();
	flowmeter.flowDataSend(GET_ML, 0, 0);
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
		uint16_t zapfMenge = flowmeter.getMilliliter()
				+ flowmeter.getFreshZapfMillis();
		user.addBier(zapfMenge); //alte ml dazurechnen
		drucker.printerZapfEnde(zapfMenge);
		flowmeter.flowDataSend(END_ZAPF, 0); //damit die Zapfmillis wieder auf null sind

		UserDataShow();
		beginZapfBool = false;
		sound.setStandby(beginZapfBool);
		logbuch.logAfterZapf();
		belohnungsMusik();
	}
	// Nachschaun ob er eventuell zu lang braucht und nix zapft
	if (((millis() - auswahlZeit) > 10000) && (flowmeter.getMilliliter() < 5)) {
		beginZapfBool = false;
		sound.setStandby(beginZapfBool);
		temp.sendeBefehl(END_ZAPF, 0x0);
		ventil.check();
		wsLedGrundbeleuchtung();
	}
	if ((user.menge() - flowmeter.getMilliliter()) < 30) {
		temp.sendeBefehl(KURZ_VOR_ZAPFENDE, 0x0);
		ventil.check();
	}
}

void loop() {
	byte oldeinsteller = Einsteller;
	sound.midiNextEvent();
	temp.holeDaten();
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

	/*
	 * Hier nur checken wenn kein Godmode weil sonst Midi zu langsam spielt
	 * ansonsten jede Sekunde mal Daten aktualisieren
	 */
	if (((millis() - oldTime) > 200) && user.getGodMode() == 0) {
		oldTime = millis();
		anzeigeAmHauptScreen();
		sound.pruefe();
		if (DEBUG_A) {
			DEBUGMSG(sound.debugmessage);
		}
	}

	if ((millis() - nachSchauZeit) > 10000 && !beginZapfBool && !sound.isOn()) {
		seltencheck();
		nachSchauZeit = millis();
	}

	if (beginZapfBool) {
		beginnZapfProgramm();
	}

	//Check Knöpfe (Benutzer) -> Display up -> Zapfprogramm

	//Zwischendurch mal was protokollieren (alle 5 minuten oder so)
}

void errorLed() {
	while (!digitalRead(TASTE1_PIN) && !digitalRead(TASTE2_PIN)) {
		digitalWrite(TASTE1_LED, HIGH);
		digitalWrite(TASTE2_LED, LOW);
		delay(200);
		digitalWrite(TASTE1_LED, LOW);
		digitalWrite(TASTE2_LED, HIGH);
	}
	analogWrite(TASTE1_LED, TASTEN_LED_NORMAL);
	analogWrite(TASTE2_LED, TASTEN_LED_NORMAL);
}

void userShow(void) {
	Einsteller = 2; //Wieder bei mL beginnen beim Drehknebel
	ZD.userShow(&user);
	UserDataShow();
}

void anzeigeAmHauptScreen(void) {
	ZD.print_val3(temp.getBlockAussenTemp(), 20, 125, KOMMA);
	ZD.print_val3((int) flowmeter.getMilliliter(), 20, 150, GANZZAHL);
	//ZD.printText();
	ZD.print_val3((int) ventil.getPressure(), 20, 175, KOMMA);

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
			if ((user.temp() < MIN_TEMP)) {
				user.setTemp(MIN_TEMP);
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
