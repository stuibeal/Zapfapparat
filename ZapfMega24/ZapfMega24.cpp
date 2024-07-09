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
#include "zPower.h"
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
volatile uint8_t einsteller = 2;
uint8_t oldeinsteller = 2;

bool ebiModeBool = false; //#define ebiMode               0xF9      //    1 an, 0 aus       Temperatur auf 2°C, Hahn auf, Zapfmusik
bool beginZapfBool = false; // #define beginZapf             0xFA      //    Beginn das Zapfprogramm -> PID auf aggressiv
bool endZapfBool = false; //#define endZapf               0xFB      //    Data send : milliliter
bool kurzBevorZapfEndeBool = false; //#define kurzBevorZapfEnde     0xFC      //    sagt das wir kurz vor Ende sind → Valve schließen -> PID auf konservativ

// DREHENCODER
#define ENCODER_OPTIMIZE_INTERRUPTS
Encoder Dreher(ROTARY_DT_PIN, ROTARY_CLK_PIN); //PINS für Drehgeber

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
PCA9685 wsLed(WS_LED_ADDRESS);
zPower power;
bool debugflowBool = 0;

void setup(void) {
	power.beginPower(); /* STROM AN */
	pinMode(FLOW_SM6020, OUTPUT);
	oldTime = millis();
	digitalWrite(FLOW_SM6020, HIGH);

	flowmeter.initialise(); /* HIER i2c begin!*/
	beginWaehlscheibe(); /* WS und Tastenpins */

	ZD.beginn(&SD); /* Display mit Pointer zur SD starten */
	if (!SD.begin(SD_CS)) {  // nachschauen ob die SD-Karte drin und gut ist
		ZD.printInitText("SD Karte Error!");
		ZD.printInitText("Knopf 1: retry");
		ZD.printInitText("Knopf 2: egal");
		switch (errorLed()) {
		case 1:
			SD.begin(SD_CS);
			break;
		case 2:
			break;
		}
		return;   // don't do anything more if not
	} else {
		ZD.printInitText("SD Karte OPTIMAL!");
	}
	ZD.showBMP("/bmp/z-logo.bmp", 20, 20);

	drucker.initialise(); /* Thermodrucker */
	ZD.printInitText("Talondrucker bereit");
	temp.begin(); /* Temperaturcontrol uC - Wire sollte gestartet sein! */
	ZD.printInitText("Temperaturfühler bereit");

	//Rotary Encoder
	pinMode(ROTARY_SW_PIN, INPUT); // Drehgeberknopf auf Input
	attachInterrupt(digitalPinToInterrupt(ROTARY_SW_PIN),
			einstellerUmsteller_ISR, FALLING);
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
	ZD.printInitText("Ventilsteuerung bereit");

	//DCF RTC
	logbuch.initialise(&SD, &user, &temp, buf);
	ZD.printInitText("RTC DCF77 Uhrensohn");

	//FLOWMETER
	pinMode(FLOW_WINDOW, INPUT);    //Wenn durchfluss, dann true
	while (digitalRead(FLOW_WINDOW))	//Warten bis das Hochgefahren ist
	{
		delay(1);
	}
	ZD.printInitText("Flowmeter ifm SM6020 ready");

	//Make Windows 95 great again

	anfang();
	oldTime = millis();
	nachSchauZeit = millis();
	sound.bing();
}  //VOID SETUP

void anfang(void) {
	ZD.backgroundPicture();
	power.ledGrundbeleuchtung();
	userShow();
	ZD.showTastenFunktion("INFOSEITE", "SONDERFUNKTION");
}

void loop() {
	oldeinsteller = einsteller;
	sound.midiNextEvent();
	temp.holeDaten();
	drehgeber();

	ventil.check();

	//sprintf(buf, "Valvestate: %d old %d ist %d soll %d rt %lu vm %lu ", ventil.getValveState(),ventil.getOldState(), ventil.getValveProzent(), ventil.getValveProzentSoll(),ventil.getRunTime(), ventil.getValveMillis());
	switch (user.zapfStatus) {
	case user.zapfModus::zapfStandby:
		zapfStandbyProg();
		break;
	case user.zapfModus::zapfError:
		zapfErrorProg();
		break;
	case user.zapfModus::zapfBeginn:
		zapfBeginnProg();
		break;
	case user.zapfModus::amZapfen:
		amZapfenProg();
		break;
	case user.zapfModus::godZapfen:
		godZapfenProg();
		break;
	case user.zapfModus::kurzVorZapfEnde:
		kurzVorZapfEndeProg();
		break;
	case user.zapfModus::zapfEnde:
		zapfEndeProg();
		break;
	}
	//debugFlow();

} /*LOOP*/

void zapfStandbyProg(void) {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		ZD.showTastenFunktion("INFOSEITE", "SONDERFUNKTION");
	}
	dauerCheck();
	if (user.aktuell == 0 && ventil.getValveProzent() > 50) {
		if (digitalRead(FLOW_WINDOW)) {
			user.zapfStatus = user.zapfModus::zapfError;
		}
	}

}

void debugFlow() {
	static uint16_t wieoft = 0;
	wieoft++;
	uint32_t t = 0;
	while (debugflowBool && t < 100) {
		t++;
		sprintf(buf, "wieoft %u state: %d ventil %d window %d t %lu", wieoft,
				user.zapfStatus, ventil.getValveProzent(),
				digitalRead(FLOW_WINDOW), t);
		ZD.infoText(buf);
	}
}

void zapfErrorProg() {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		ZD.infoText("HEY DU HONK! ERST BENUTZER WÄHLEN!");
		ventil.closeValve();
		while (ventil.getValveProzent() > 0) {
			ventil.check();
		}
		user.zapfStatus = user.zapfModus::zapfStandby;
		debugflowBool = 1;

	}

}

void zapfBeginnProg(void) {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		ventil.openValve();
		temp.sendeBefehl(BEGIN_ZAPF, 0x0);
		flowmeter.flowDataSend(SET_USER_ML, user.menge());
		sound.godModeSound(user.getGodMode());
		delay(500); //damit der Zeit hat

		flowmeter.flowDataSend(BEGIN_ZAPF, 0, 0);
		userShow();  // Zeigt die Userdaten an
		auswahlZeit = millis();
		ZD.infoText("BITTE ZAPFHAHN BETÄTIGEN");
	}

	dauerCheck();
	if (millis() - auswahlZeit > WARTE_ZEIT) {
		user.zapfStatus = user.zapfModus::zapfStandby;
		user.aktuell = 0;
		ZD.userShow(&user);
	}
	if (digitalRead(FLOW_WINDOW)) {
		user.zapfStatus = user.zapfModus::amZapfen;
	}

}
void amZapfenProg(void) {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		ZD.showTastenFunktion("ZAPFABBRUCH", "ABBRUCH+SET ML");
		sound.setStandby(1);
		sound.pruefe();
		if (user.getGodMode() > 0) {
			user.zapfStatus = user.zapfModus::godZapfen;
		}

	}
	flowmeter.flowDataSend(GET_ML, 0, 0);

	// Nachschaun ob er fertig ist und dann bingen und zamschreim
	if (flowmeter.getMilliliter() >= user.menge()) {
	}
// Nachschaun ob er eventuell zu lang braucht und nix zapft
	if (((millis() - auswahlZeit) > 10000) && (flowmeter.getMilliliter() < 5)) {
		beginZapfBool = false;
		sound.setStandby(0);
		temp.sendeBefehl(END_ZAPF, 0x0);
		ventil.check();
		power.ledGrundbeleuchtung();
	}
	if ((user.menge() - flowmeter.getMilliliter()) < 30) {
		temp.sendeBefehl(KURZ_VOR_ZAPFENDE, 0x0);
		ventil.check();
	}

	if (digitalRead(TASTE1_PIN)) {
		user.zapfStatus = user.zapfModus::zapfEnde;
		power.tastenLed(1, 255);
	}

}
void godZapfenProg(void) {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
	}

	/*MIDI nach Zapfhahn*/
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
		sound.tickMetronome();
	}
	if (digitalRead(TASTE1_PIN)) {
		user.zapfStatus = user.zapfModus::zapfEnde;
		power.tastenLed(1, 255);
	}

}

void kurzVorZapfEndeProg(void) {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		ZD.infoText("ZAPFVORGANG FAST FERTIG!");
		power.setWhiteLed(0xFFF);
	}

}
void zapfEndeProg(void) {
	/* Einmalige Ausführung */
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		sound.bing();
		ZD.showTastenFunktion("", "TALON DRUCKEN");
		auswahlZeit = millis();
		ZD.showUserGod2Pic();
		sound._SMF->close();
		sound.midiSilence();
		ventil.check();

		//Sollte er abgebrochen haben:
		uint16_t zapfMenge = flowmeter.getMilliliter()
				+ flowmeter.getFreshZapfMillis();
		user.addBier(zapfMenge); //alte ml dazurechnen
		flowmeter.flowDataSend(END_ZAPF, 0); //damit die Zapfmillis wieder auf null sind

		ZD.showAllUserData();
		beginZapfBool = false;
		sound.setStandby(beginZapfBool);
		logbuch.logAfterZapf();
		belohnungsMusik();
	}

	/*Ab hier Dauerausführung*/
	if (readTaste(2)) {
		drucker.printerZapfEnde(user.zapfMenge);
	}

	dauerCheck();
	warteZeitCheck();
}

void dauerCheck(void) {
	if (digitalRead(WSready)) {
		waehlscheibe();
	}
	//Wenn was rumgestellt wird
	if (oldeinsteller != einsteller) {
		ZD.showAllUserData();
	}
	//Wenn jemand an den Tasten rumspielt
	if (digitalRead(TASTE1_PIN)) {
		infoseite();
	}
	// alle 200ms anzeige aktualisieren
	if (((millis() - oldTime) > 200)) { // && user.getGodMode() == 0) {
		oldTime = millis();
		anzeigeAmHauptScreen();
		sound.pruefe();
		if (DEBUG_A) {
			DEBUGMSG(sound.debugmessage);
		}
	}

	//alle 10s mal alles nachkucken
	if ((millis() - nachSchauZeit) > 10000 && !beginZapfBool && !sound.isOn()) {
		seltencheck();
		nachSchauZeit = millis();
	}
	// Wenn Nummer Fertig und Taste losgelassen
	if (!digitalRead(TASTE2_PIN) && kienmuehle > 0) {
		power.tastenLed(2, TASTEN_LED_NORMAL);
		waehlFunktionen();
		power.ledGrundbeleuchtung();
		power.setLed(user.aktuell, 0xFFF);
	}

}

void warteZeitCheck() {
	if (millis() - auswahlZeit > WARTE_ZEIT) {
		user.aktuell = 0;
		power.ledGrundbeleuchtung();
		user.zapfStatus = user.zapfModus::zapfStandby;
		ZD.userShow(&user);
	}

}
uint8_t readTaste(uint8_t taste) {
	uint8_t tastendruck = 0;
	uint8_t tastenPin = TASTE1_PIN;
	switch (taste) {
	case 1:
		tastenPin = TASTE1_PIN;
		break;
	case 2:
		tastenPin = TASTE2_PIN;
	}
	if (digitalRead(tastenPin)) {
		while (digitalRead(tastenPin)) {
			power.tastenLed(taste, 255);
		}
		power.tastenLed(taste, TASTEN_LED_NORMAL);
		tastendruck = 1;
	}
	return tastendruck;
}

void waehlscheibe() {
	sound.setStandby(true); //dann checkt er nicht ob er ausschalten soll
	sound.on();
	auswahlZeit = millis();
	flowmeter.flowDataSend(LED_FUN_1, 10, 200); /*LEDFUN ein*/
	uint8_t zahlemann = readWaehlscheibe(); /*Wählscheibe auslesen*/
	flowmeter.flowDataSend(GET_ML, 0, 0);  //LEDFun ausschalten
	sound.pruefe();

	if (zahlemann == 0) {
		user.aktuell = 0;
		ZD.userShow(&user);
	}

	if (zahlemann > 0) {
		switch (digitalRead(TASTE2_PIN)) {
		case 0: /* Wenn nicht, Zapfprogramm starten */
			kienmuehle = 0; /*wenn man sich verwählt hat bei der Nummerneingabe wirds gelöscht*/
			user.aktuell = zahlemann;
			user.zapfStatus = user.zapfModus::zapfBeginn;
			beginZapfBool = true;

			break;
		case 1: /* Wenn doch, Nummerneingabe starten */
			power.tastenLed(2, 255);
			if (zahlemann == 10) {
				zahlemann = 0;
			}
			kienmuehle = kienmuehle * 10;  //das passt so, Alfred!
			if (zahlemann < 10) {
				kienmuehle += zahlemann;
			}
			sprintf(buf, "Nr: %9lu", kienmuehle);
			ZD.infoText(buf);
			break;
		}
	}
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
	case 624686:
		user.setGodMode(MAGNUM);
		ZD.userShow(&user);
		break;
	case 62249837:
		user.setGodMode(MACGYVER);
		ZD.userShow(&user);
		break;
	case 64264:
		user.setGodMode(MIAMI);
		ZD.userShow(&user);
		break;
	case 73463353:
		user.setGodMode(SEINFELD);
		ZD.userShow(&user);
		break;
	case 253:
		user.setGodMode(ALF);
		ZD.userShow(&user);
		break;
	case 2658:
		user.setGodMode(COLT);
		ZD.userShow(&user);
		break;
	case 3688:
		user.setGodMode(DOTT);
		ZD.userShow(&user);
		break;
	case 4639:
		user.setGodMode(INDY);
		ZD.userShow(&user);
		break;
	case 5824:
		user.setGodMode(JUBI);
		ZD.userShow(&user);
		break;
	case 8100:
		ventil.openValve();
		break;
	case 80:
		ventil.closeValve();
		break;
	case 1275: //Die Telefonnummer der Kienmühle
		oldWaehlscheibeFun();
		break;
	case 54248: /* LICHT*/
		flowmeter.flowDataSend(LED_FUN_4, 0b11111111, 128);
		digitalWrite(Z_SCH_LAMPE_PIN, 1);
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

uint8_t errorLed() {
	uint8_t tasteGedrueckt = 0;
	while (!digitalRead(TASTE1_PIN) && !digitalRead(TASTE2_PIN)) {
		digitalWrite(TASTE1_LED, HIGH);
		digitalWrite(TASTE2_LED, LOW);
		delay(200);
		digitalWrite(TASTE1_LED, LOW);
		digitalWrite(TASTE2_LED, HIGH);
	}
	if (digitalRead(TASTE1_PIN)) {
		tasteGedrueckt = 1;
	}
	if (digitalRead(TASTE2_PIN)) {
		tasteGedrueckt = 2;
	}
	power.ledGrundbeleuchtung();
	return tasteGedrueckt;
}

void userShow(void) {
	einsteller = 2; //Wieder bei mL beginnen beim Drehknebel
	ZD.userShow(&user);
}

void anzeigeAmHauptScreen(void) {
	ZD.print_val3(temp.getBlockAussenTemp(), 20, 125, KOMMA);
	ZD.print_val3((int) flowmeter.getMilliliter(), 20, 150, GANZZAHL);
	ZD.print_val3((int) ventil.getPressure(), 20, 175, KOMMA);
}

void drehgeber() {
	static long oldPosition = 0;
	long newPosition = Dreher.read();
	if (newPosition != oldPosition) {
		switch (einsteller) {
		case 1:
			user.setTemp(user.temp() + (newPosition - oldPosition));
			ZD.showSingleUserData(1);
			break;
		case 2:
			user.setMenge(user.menge() + (newPosition - oldPosition));
			ZD.showSingleUserData(2);
			break;
		}
		oldPosition = newPosition;
	}
}

void einstellerUmsteller_ISR() { //Interruptroutine, hier den Drehgeberknopf abfragen
	einsteller++;
	if (einsteller > 2) {
		einsteller = 1;
	}
}

void reinigungsprogramm(void) {
//hier braces damit die variablen allein hier sind
	ZD.infoText("Reinigungsprogramm");
	temp.sendeBefehl(ZAPFEN_STREICH, 0x0);
	ventil.cleanPumpOn();
	while (digitalRead(TASTE1_PIN)) {
	}
	delay(20); //zum taste loslassen!
	unsigned long waitingTime = millis();
	int sekunden = 0;
	power.tastenLed(1, 255);
	while (!digitalRead(TASTE1_PIN)) {
		if (millis() - waitingTime >= 1000) {
			waitingTime = millis();
			sekunden++;
			sprintf(buf, "Reinigt seit %d Sekunden", sekunden);
			ZD.infoText(buf);
		}

	}
	ventil.cleanPumpOff();
	ZD.infoText("Ender Geländer");

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
	case 3: // F für Fass
		if (varContent < 65) {
			user.restMengeFass = varContent * 1000;
			ZD.showAllUserData();
		} else {
			ZD.infoText("Fassgröße über 65l nicht möglich!");
		}
		break;
	case 8: //Ventil
		if (varContent > 100) {
			varContent = 100;
		}
		ventil.setValveProzent(varContent);

		break;
	case 9: //Mediaplayer
		sound.mp3Play(11, varContent);
		break;
	case 99: // auch Mediaplayer
		sound.mp3Play(11, varContent);
		break;
	}
}
