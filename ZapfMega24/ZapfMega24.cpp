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
#include "Encoder.h"  //für Drehencoder
#include "./zLibraries/RTC_DCF/DateTime.h"   // ELV RTC mit DCF
#include "./zLibraries/RTC_DCF/RealTimeClock_DCF.h"
#include "./zLibraries/zPrinter/zPrinter.h"
#include "benutzer.h"
#include "MD_MIDIFile.h"
#include "MD_YX5300.h"
#include "tempControl.h"
#include "audio.h"
#include "common/zLog.h"
#include "zDisplay.h"
#include "zValve.h"
#include "zWireHelper.h"

// Defines
#define USE_SDFAT
#define ENCODER_USE_INTERRUPTS  //damit die vom MEGA drin sind und er die als INT setzt
//#include <MD_cmdProcessor.h>   //weiss nicht ob wir den brauchen

#define DEBUGTO 3  //0 Nothing, 1 Serial, 2 Printer, 3 Display, 4 SD File
#if DEBUGTO == 3
#define DEBUGMSG(s) { ZD.infoText(1, s); }
#endif
#ifndef DEBUG_A
#define DEBUG_A 1 //Debug Audio
#endif
// Defines aus

//Hier Variablen definieren
unsigned long auswahlZeit = 0;
unsigned long oldTime = millis();
unsigned long nachSchauZeit = 0;

//Tastentexte
const char tt_zapfabbruch[] = "ZAPFABBRUCH";
const char tt_zapfabbruchUndSet[] = "ABBRUCH+SET ML";
const char tt_talon[] = "TALON DRUCKEN";
const char tt_sonder[] = "SONDERFUNKTION";
const char tt_fertig[] = "BIN FERTIG";
const char tt_info[] = "INFOSEITE";
const char tt_bittezapfen[] = "BITTE ZAPFHAHN BETÄTIGEN";
const char tt_abbruch[] = "ABBRUCH";

//Waehlscheibe
uint32_t kienmuehle = 0;  //Sondereingabe bei drücken der Taste2
volatile uint8_t einsteller = 2;
uint8_t oldeinsteller = 2;

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

void setup(void) {
	power.beginPower(); /* STROM AN */
	pinMode(FLOW_SM6020, OUTPUT);
	oldTime = millis();
	digitalWrite(FLOW_SM6020, HIGH);

	flowmeter.initialise(); /* HIER i2c begin!*/
	beginWaehlscheibe(); /* WS und Tastenpins */

	ZD.beginn(&SD); /* Display mit Pointer zur SD starten */
	if (!SD.begin(SD_CS)) {  // nachschauen ob die SD-Karte drin und gut ist
		ZD.printInitText(F("SD Karte Error!"));
		ZD.printInitText(F("Knopf 1: retry"));
		ZD.printInitText(F("Knopf 2: egal"));
		switch (errorLed()) {
		case 1:
			SD.begin(SD_CS);
			break;
		case 2:
			break;
		}
		return;   // don't do anything more if not
	} else {
		ZD.printInitText(F("SD Karte OPTIMAL!"));
	}
	ZD.showBMP(F("/bmp/z-logo.bmp"), 20, 20);

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
	ZD.printInitText(F("Harte Musik ok"));

	//Valve
	ventil.begin();
	ventil.check();   //dann sollte das aufgehen
	ZD.printInitText(F("Ventilsteuerung bereit"));

	//DCF RTC
	logbuch.initialise();
	ZD.printInitText(F("RTC DCF77 Uhrensohn"));

	temp.begin(); /* Temperaturcontrol uC - Wire sollte gestartet sein! */
	ZD.printInitText(F("Temperaturfühler bereit"));

	//FLOWMETER
	pinMode(FLOW_WINDOW, INPUT);    //Wenn durchfluss, dann true
	while (digitalRead(FLOW_WINDOW))	//Warten bis das Hochgefahren ist
	{
		delay(1);
	}
	ZD.printInitText(F("Flowmeter ifm SM6020 ready"));

	user.readDataFromEEPROM();
	ZD.printInitText(F("Daten vom EEPROM gelesen"));

	ZD.printInitText(F("Talondrucker starten..."));
	drucker.initialise(); /* Thermodrucker */

	logbuch.logSystemMsg(F("Zapfapparat hochgefahren"));

	anfang();
	oldTime = millis();
	nachSchauZeit = millis();
	sound.bing();
}  //VOID SETUP

void anfang(void) {
	ZD.backgroundPicture();
	power.ledGrundbeleuchtung();
	userShow();
	ZD.showTastenFunktion(tt_sonder, tt_info);
}

void loop() {
	oldeinsteller = einsteller;
	sound.midiNextEvent();
	drehgeber();
	ventil.check();

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
} /*LOOP*/

void zapfStandbyProg(void) {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		ZD.showTastenFunktion(tt_sonder, tt_info);
		power.ledGrundbeleuchtung();
	}
	dauerCheck();
	if (user.aktuell == 0 && ventil.getValveProzent() > 50) {
		if (digitalRead(FLOW_WINDOW)) {
			user.zapfStatus = user.zapfModus::zapfError;
		}
	}
}

void zapfErrorProg() {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		logbuch.logSystemMsg(F("Fehlzapfung"));
		ZD.infoText(1, F("HEY DU HONK! ERST BENUTZER WÄHLEN!"));
		sound.on();
		ventil.closeValve();
		sound.check();
		sound.mp3Play(29, 3);
		while (ventil.getValveProzent() > 0) {
			ventil.check();
		}
		while (digitalRead(FLOW_WINDOW)) {
			delay(1);
		}
		flowmeter.flowDataSend(END_ZAPF, 0);
		user.zapfMenge = flowmeter.getFreshZapfMillis();
		ZD.infoText(1, F("Zapfhahn gesperrt mein Herr! Wählen!"));
		user.zapfStatus = user.zapfModus::zapfStandby;
	}
}
void preZapf(uint8_t nummer) {
	sound.setStandby(true); //dann checkt er nicht ob er ausschalten soll
	kienmuehle = 0; /*wenn man sich verwählt hat bei der Nummerneingabe wirds gelöscht*/
	user.aktuell = nummer;
	user.oldZapfStatus = user.zapfModus::zapfStandby;
	user.zapfStatus = user.zapfModus::zapfBeginn;
	sprintf_P(buf, PSTR("User Nr %d: %s angemeldet"), user.aktuell,
			user.getName());
	logbuch.logSystemMsg(buf);
}

void zapfBeginnProg(void) {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		/* sollte er fehlgezapft haben und dann gewählt werden die ihm draufgeschrieben*/
		if (user.oldZapfStatus == user.zapfModus::zapfError) {
			user.addBier();
			ZD.showAllUserData();
			user.zapfMenge = flowmeter.getFreshZapfMillis(); //wenn nix zwischengezapft wurde sollts 0 sein
			ZD.infoText(1, F("Restbier der Fehlzapfung aufgerechnet!"));

		}
		/* sollte er am Nulluser rumgespielt haben kriegt er die Einstellung */
		if (user.checkNullUser()) {
			//die PIENE kann die Temperatur verstellen
			if (user.aktuell == 19) {
				temp.sendeBefehl(SET_TEMPERATUR, user.getSollTemperatur());
			}
			ZD.showAllUserData();
		}

		ventil.openValve();
		temp.sendeBefehl(BEGIN_ZAPF, 0x0);
		flowmeter.flowDataSend(SET_USER_ML, user.getMenge());
		sound.godModeSound(user.getGodMode());
		temp.sendeBefehl(BEGIN_ZAPF, 0x0);
		userShow();  // Zeigt die Userdaten an
		while (sound.pruefe() != sound.AUDIO_STANDBY) {
		}
		auswahlZeit = millis();
		ZD.infoText(1, F("BITTE ZAPFHAHN BETÄTIGEN"));
	}

	dauerCheck();
	if (millis() - auswahlZeit > WARTE_ZEIT) {
		user.zapfStatus = user.zapfModus::zapfStandby;
		user.aktuell = 0;
		ZD.userShow();
		sound.setStandby(0);
		temp.sendeBefehl(END_ZAPF, 0x0);
		ventil.check();
		power.ledGrundbeleuchtung();
	}

	/*Hier wird dann gekuckt ob der zum zapfen beginnt. gott lobe den flowmeter!*/
	if (digitalRead(FLOW_WINDOW)) {
		user.zapfStatus = user.zapfModus::amZapfen;
		auswahlZeit = millis();
	}
}

void amZapfenProg(void) {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		ZD.showTastenFunktion(tt_zapfabbruch, tt_zapfabbruchUndSet);
		if (user.getGodMode() > 0) {
			user.zapfStatus = user.zapfModus::godZapfen;
		}
		flowCheckWhileZapfing(1);
	}
	checkWhileZapfing();
	flowCheckWhileZapfing(0);
// sollte er abbrechen
	if (readTaste(1)) {
		drucker.printerErrorZapfEnde();
		user.zapfStatus = user.zapfModus::zapfEnde;
	}
	if (readTaste(2)) {
		user.zapfStatus = user.zapfModus::zapfEnde;
		user.setMenge(user.zapfMenge);
	}
}

void godZapfenProg(void) {
	static uint8_t oldFlowWindow;
	static uint8_t flowWindow;
	oldFlowWindow = flowWindow;
	flowWindow = digitalRead(FLOW_WINDOW);

	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		sound.on();
		sound.mp3Pause();
		oldFlowWindow = false;
		flowWindow = true;
		sound._SMF->pause(false);
	}

	/*MIDI nach Zapfhahn*/
	if (oldFlowWindow == true && flowWindow == false) {
		sound._SMF->pause(true);
	}
	if (oldFlowWindow == false && flowWindow == true) {
		sound._SMF->pause(false);
	}
	if (!sound._SMF->isPaused()) {
		sound.tickMetronome();
	}
	if (readTaste(1)) {
		user.zapfStatus = user.zapfModus::zapfEnde;
	}
	/*GOD braucht den grünen Balken mehr denn je*/
	user.zapfMenge = flowmeter.getFreshZapfMillis();
	ZD.showBalken();

	// Nachschaun ob er fertig ist und dann bingen und zamschreim
	if (user.zapfMenge >= user.getMenge()) {
		user.zapfStatus = user.zapfModus::zapfEnde;
		sound.mp3Pause();
	}

}

void kurzVorZapfEndeProg(void) {
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		ZD.infoText(1, F("ZAPFVORGANG FAST FERTIG!"));
		power.setLed(0, 1);
		temp.sendeBefehl(KURZ_VOR_ZAPFENDE, 0x0);
		power.tastenLed(0, 255);

	}
	checkWhileZapfing();
	flowCheckWhileZapfing(0);
	if (user.zapfMenge >= user.getMenge()) {
		user.zapfStatus = user.zapfModus::zapfEnde;
	}

}
void zapfEndeProg(void) {
	/* Einmalige Ausführung */
	if (user.oldZapfStatus != user.zapfStatus) {
		user.oldZapfStatus = user.zapfStatus;
		sound.bing();
		ZD.showTastenFunktion(tt_fertig, tt_talon);
		auswahlZeit = millis();
		ZD.showUserGod2Pic();
		sound._SMF->close();
		sound.midiSilence();
		ventil.check();
		/*Sollte der noch weiterzapfen*/
		while (digitalRead(FLOW_WINDOW)) {
			checkWhileZapfing();
		}
		user.addBier();
		ZD.showAllUserData();
		if (!logbuch.logAfterZapf()) {
			ZD.infoText(1, F("Irgendwas mit dem Logfile."));
		}
		belohnungsMusik();
		sound.setStandby(0);
		power.setBackLight();
	}

	/*Ab hier Dauerausführung*/

	while (digitalRead(FLOW_WINDOW)) {
		// wenn der doch noch weiterzapft!
		checkWhileZapfing();
	}

	if (readTaste(1)) {
		backToNull();
	}

	if (readTaste(2)) {
		temp.holeDaten();
		drucker.printerZapfEnde();
		backToNull();
	}

	dauerCheck();
	warteZeitCheck();
}

void flowCheckWhileZapfing(uint8_t reset) {
	static uint16_t lastFlow;
	uint16_t actualFlow;
	static uint8_t sekundenOhneFlow;
	if (reset) {
		lastFlow = 0;
		actualFlow = 1;
		sekundenOhneFlow = 0;
		auswahlZeit = millis();
	} else {

		// Nachkucken ob er im kurz vor Zapfende ist
		if (millis() - auswahlZeit > 999) {
			auswahlZeit = millis();
			actualFlow = flowmeter.getMilliliter() - lastFlow;
			if (user.zapfMenge + actualFlow > user.getMenge()) {
				user.zapfStatus = user.zapfModus::kurzVorZapfEnde;
			}
			if (actualFlow == 0) {
				sekundenOhneFlow++;
			} else {
				sekundenOhneFlow = 0;
			}
			lastFlow = flowmeter.getMilliliter();
		}

		// Nachschaun ob er eventuell zu lang braucht und nix zapft
		if (sekundenOhneFlow > 20) {
			sekundenOhneFlow = 0;
			sound.mp3Play(29, 2); //Kackalafax
			delay(5000);
			user.zapfStatus = user.zapfModus::zapfEnde;
		}
	}
}

void checkWhileZapfing() {
	if (((millis() - oldTime) > 100)) {
		static uint8_t counter = 0;
		counter++;
		oldTime = millis();
		user.zapfMenge = flowmeter.getFreshZapfMillis();
		if (counter > 4) {
			counter = 0;
			temp.holeDaten();
			sound.pruefe();
			showZapfapparatData();
			if (DEBUG_A) {
				ZD.infoText(1, buf);
			}
		} else {
			ZD.print_val3((int) user.zapfMenge, 17, 170, GANZZAHL);
		}
	}
}

void checkImmer() {
	if (((millis() - oldTime) > 500)) {
		power.check();
		temp.holeDaten();
		flowmeter.flowDataSend(GET_ML, 0, 0);
		oldTime = millis();
		showZapfapparatData();
		sound.pruefe();
		if (DEBUG_A) {
			ZD.infoText(1, buf);
		}
	}
}

void dauerCheck(void) {
	checkImmer();

	ZD.infoCheck();

	if (temp.getBlockInnenTemp() < (int16_t) user.bierTemp[19]) {
		power.setLed(11, 1);
	}
	if (temp.getBlockAussenTemp() + 5 < (int16_t) user.bierTemp[19]) {
		power.setLed(0, 1);
	}

	if (digitalRead(WSready)) {
		waehlscheibe();
	}

	//Wenn was rumgestellt wird
	if (oldeinsteller != einsteller) {
		ZD.showAllUserData();
	}
	//Wenn jemand an den Tasten rumspielt
	if (digitalRead(TASTE2_PIN) && user.zapfStatus != user.zapfEnde) {
		infoseite();
	}

	//alle 10s mal alles nachkucken
	if ((millis() - nachSchauZeit) > 10000
			&& user.zapfStatus == user.zapfModus::zapfStandby
			&& !sound.isOn()) {
		seltencheck();
		nachSchauZeit = millis();
	}
	// Sonderfunktionsinfoanzeige
	if (digitalRead(TASTE1_PIN)) {
		power.tastenLed(1, 255);
		if (kienmuehle < 10) {
			showSpezialProgrammInfo(kienmuehle);

		}
	}

	// Wenn Nummer Fertig und Taste losgelassen
	if (!digitalRead(TASTE1_PIN) && kienmuehle > 0) {
		power.tastenLed(2, TASTEN_LED_NORMAL);
		waehlFunktionen();
		ZD.backGroundUserData();
		ZD.showAllUserData();
		power.ledGrundbeleuchtung();
		power.setLed(user.aktuell, 0xFFF);
	}

}

void backToNull() {
	sprintf_P(buf, PSTR("User Nr %d: %s Zapfung beendet"), user.aktuell,
			user.getName());
	logbuch.logSystemMsg(buf);
	sprintf_P(buf,
			PSTR("User Nr %d: Zapfmenge %d, LastZapfmenge %d, Eingestellt %d "),
			user.aktuell, user.zapfMenge, user.lastZapfMenge, user.getMenge());
	logbuch.logSystemMsg(buf);
	if (user.zapfMenge > 0) {
		user.addBier();
	}
	temp.sendeBefehl(END_ZAPF, 0x0);
	user.aktuell = 0;
	power.ledGrundbeleuchtung();
	user.zapfStatus = user.zapfModus::zapfStandby;
	ZD.userShow();
}

void warteZeitCheck() {
	if (millis() - auswahlZeit > WARTE_ZEIT) {
		backToNull();
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
	auswahlZeit = millis();
	flowmeter.flowDataSend(LED_FUN_1, 10, 200); /*LEDFUN ein*/
	sound.setStandby(1);
	uint8_t zahlemann = readWaehlscheibe(); /*Wählscheibe auslesen*/
	power.setBackLight();
	sound.pruefe();

	/* bei leichtem Antippen der Wählscheibe, keine Wählung */
	if (zahlemann == 0) {
		user.aktuell = 0;
		ZD.userShow();
		power.ledGrundbeleuchtung();
		sound.setStandby(0);
	}

	if (zahlemann > 0) {
		switch (digitalRead(TASTE1_PIN)) {
		case 0: /* Wenn nicht, Zapfprogramm starten */
			preZapf(zahlemann);
			break;
		case 1: /* Wenn doch, Nummerneingabe starten */
			sound.setStandby(0);
			power.tastenLed(1, 255);
			if (zahlemann == 10) {
				zahlemann = 0;
			}
			kienmuehle = kienmuehle * 10;  //das passt so, Alfred!
			if (zahlemann < 10) {
				kienmuehle += zahlemann;
			}
			sprintf_P(buf, PSTR("Nummer gwählt: %9lu"), kienmuehle);
			ZD.infoText(0, buf);
			break;
		}
	}
}

void waehlFunktionen() {
	logbuch.logSystemMsg(buf); // hier sollte noch die Nr drinstehen
	switch (kienmuehle) {
	case 847: // UHRZEIT
		logbuch.getClockString();
		ZD.infoText(1, buf);
		break;
	case GODOFF_NR: //GODOFF
		user.setGodMode(0);
		ZD.userShow();
		break;
	case IDDQD_NR:
		user.setGodMode(IDDQD);
		ZD.userShow();
		break;
	case KEEN_NR:
		user.setGodMode(KEEN);
		ZD.userShow();
		break;
	case MAGNUM_NR:
		user.setGodMode(MAGNUM);
		ZD.userShow();
		break;
	case MACGYVER_NR:
		user.setGodMode(MACGYVER);
		ZD.userShow();
		break;
	case MIAMI_NR:
		user.setGodMode(MIAMI);
		ZD.userShow();
		break;
	case SEINFELD_NR:
		user.setGodMode(SEINFELD);
		ZD.userShow();
		break;
	case ALF_NR:
		user.setGodMode(ALF);
		ZD.userShow();
		break;
	case COLT_NR:
		user.setGodMode(COLT);
		ZD.userShow();
		break;
	case DOTT_NR:
		user.setGodMode(DOTT);
		ZD.userShow();
		break;
	case INDY_NR:
		user.setGodMode(INDY);
		ZD.userShow();
		break;
	case JUBI_NR:
		user.setGodMode(JUBI);
		ZD.userShow();
		break;
	case GIANNA_NR:
		user.setGodMode(GIANNA);
		ZD.userShow();
		break;
	case LIGABUE_NR:
		user.setGodMode(LIGABUE);
		ZD.userShow();
		break;
	case IDKFA_NR:
		user.setGodMode(IDKFA);
		ZD.userShow();
		break;
	case IDCLEV_NR:
		user.setGodMode(IDCLEV);
		ZD.userShow();
		break;
	case GLORIA_NR:
		user.setGodMode(GLORIA);
		ZD.userShow();
		break;
	case VAGABONDO_NR:
		user.setGodMode(VAGABONDO);
		ZD.userShow();
		break;
	case 1275: //Die Telefonnummer der Kienmühle
		oldWaehlscheibeFun();
		break;
	case 337766: //EEPROM
		user.cleanEEPROM();
		ZD.infoText(1, F("EEPROM gelöscht. Reset jetzt!"));
		break;
	case 983633865: //zuendfunk
		sound.mp3AddToPlaylist(30, 2);
		ZD.infoText(1, F("Zündfunk: Fernreiseguppe"));
		break;
	case 937825425: //westblick
		sound.mp3AddToPlaylist(30, 1);
		ZD.infoText(1, F("WDR Westblick"));
		break;
	case 33549: //feliz
		sound.mp3AddToPlaylist(1, 2);
		ZD.infoText(1, F("Feliz Navidad!"));
		break;

	default:
		spezialprogramm(kienmuehle);
		break;
	}
	kienmuehle = 0;
}

void seltencheck(void) {
	temp.requestSensors();
	power.check();
}

void belohnungsMusik() {
	if (user.getBierTag() > 2000 && user.getMusik() == 0) {
		user.setMusik(1);
		delay(1000); //for the bing
		sound.mp3PlayAndWait(29, 1); //fällt mir im moment nix ein
		sound.mp3Play(user.aktuell, 1);
	}
	if (user.getBierTag() > 2500 && user.getMusik() == 1) {
		user.setMusik(2);
		delay(1000); //for the bing
		sound.mp3PlayAndWait(29, 10); //wir nähern uns dem originalpackerl
		sound.mp3Play(user.aktuell, 2);
	}
	if (user.getBierTag() > 3000 && user.getMusik() == 2) {
		user.setMusik(3);
		delay(1000); //for the bing
		sound.mp3PlayAndWait(29, 5); //originalpackerl
		sound.mp3Play(user.aktuell, 3);
	}
	if (user.getBierTag() > 3500 && user.getMusik() == 3) {
		user.setMusik(0);
		delay(1000); //for the bing
		sound.mp3PlayAndWait(29, 4); //na
		sound.mp3Play(user.aktuell, 4);
	}

}

//Infoknopf
void infoseite(void) {
	power.tastenLed(2, 255);
	ZD.infoscreen();
	if (!sound.mp3D.playTheList) {
		if (user.aktuell == 0) {
			sound.mp3Play(20, 5); //ebi du bist die vier
		}
		if (user.aktuell == 5) {
			sound.mp3Play(20, 7); //no soup for you
		}
	}
	logbuch.logSystemMsg(F("Infoseite aufgerufen"));
	while (readTaste(2)) {
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
		delay(200);
	}
	if (digitalRead(TASTE1_PIN)) {
		tasteGedrueckt = 1;
	}
	if (digitalRead(TASTE2_PIN)) {
		tasteGedrueckt = 2;
	}
	digitalWrite(TASTE1_LED, TASTEN_LED_NORMAL);
	digitalWrite(TASTE2_LED, TASTEN_LED_NORMAL);
	return tasteGedrueckt;
}

void userShow(void) {
	einsteller = 2; //Wieder bei mL beginnen beim Drehknebel
	ZD.userShow();
}

void showZapfapparatData(void) {
	ZD.print_val3(temp.getBlockInnenTemp(), 17, 82, KOMMA);
	ZD.print_val3((int) user.zapfMenge, 17, 170, GANZZAHL);
	ZD.print_val3((int) ventil.getPressure(), 17, 260, KOMMA);
}

void drehgeber() {
	static long oldPosition = 0;
	long newPosition = Dreher.read();

	if (newPosition != oldPosition) {
		switch (einsteller) {
		case 1:
			user.setTemp(
					user.getSollTemperatur() + (newPosition - oldPosition));
			ZD.showSingleUserData(1);
			break;
		case 2:
			user.setMenge(user.getMenge() + (newPosition - oldPosition));
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
	temp.sendeBefehl(ZAPFEN_STREICH, 0x0);
	ventil.cleanPumpOn();
	unsigned long waitingTime = millis();
	int sekunden = 0;
	while (!digitalRead(TASTE2_PIN)) {
		if (millis() - waitingTime >= 1000) {
			waitingTime = millis();
			sekunden++;
			sprintf_P(buf, PSTR("Reinigt seit %d Sekunden"), sekunden);
			ZD.showTastenFunktion(buf, tt_fertig);
		}
	}
	ventil.cleanPumpOff();
	ZD.infoText(1, F("Reinigung abgeschlossen"));
	temp.sendeBefehl(WACH_AUF, 0);
}

void showSpezialProgrammInfo(uint8_t programmNummer) {
	static uint8_t oldProgrammNummer = 255;
	if (programmNummer != oldProgrammNummer) {
		oldProgrammNummer = programmNummer;
		switch (programmNummer) {
		case 0: //infoscreen anfang
			ZD.printProgrammInfo(F("Spezialauswahl"));
			ZD.printProgrammInfoZeilen(1, 1, F("1 Benutzer"));
			ZD.printProgrammInfoZeilen(2, 1, F("2 Apparat"));
			ZD.printProgrammInfoZeilen(3, 1, F("3 Fass"));
			ZD.printProgrammInfoZeilen(4, 1, F("4 Info"));
			ZD.printProgrammInfoZeilen(5, 1, F("5 Licht"));
			ZD.printProgrammInfoZeilen(1, 2, F("6 MIDI"));
			ZD.printProgrammInfoZeilen(2, 2, F("7 Schlafen"));
			ZD.printProgrammInfoZeilen(3, 2, F("8 Ventil"));
			ZD.printProgrammInfoZeilen(4, 2, F("9 MP3"));
			break;
		case 1: //USER 11-19
			ZD.printProgrammInfo(F("Benutzer 11-19"));
			ZD.printProgrammInfoZeilen(1, 1, F("Jetzt bitte zweite"));
			ZD.printProgrammInfoZeilen(2, 1, F("Zahl für die Benutzer"));
			ZD.printProgrammInfoZeilen(3, 1, F("11-19 wählen."));
			ZD.printProgrammInfoZeilen(4, 1, F("Beispielhaft die 8"));
			ZD.printProgrammInfoZeilen(5, 1, F("für Nutzer 18"));
			break;
		case 2: // ABC APPARAT
			ZD.printProgrammInfo(F("Apparat"));
			ZD.printProgrammInfoZeilen(1, 1, F("LEAN: Reinigung"));
			ZD.printProgrammInfoZeilen(2, 1, F("UF00: Daten löschen"));
			ZD.printProgrammInfoZeilen(4, 1, F("VORSICHT!"));
			ZD.printProgrammInfoZeilen(5, 1, F("WEG  IST WEG."));
			break;
		case 3: // DEF FASS
			ZD.printProgrammInfo(F("Fasswechsel"));
			ZD.printProgrammInfoZeilen(1, 1, F("Jetzt zweistellig"));
			ZD.printProgrammInfoZeilen(2, 1, F("die Faßgröße wählen"));
			ZD.printProgrammInfoZeilen(3, 1, F("MIN: 01, MAX: 65"));
			ZD.printProgrammInfoZeilen(4, 1, F("Dann wird der"));
			ZD.printProgrammInfoZeilen(5, 1, F("Restbierzähler rückgesetzt."));
			break;
		case 4: // GHI  INFO
			ZD.printProgrammInfo(F("Information"));
			sprintf_P(buf, PSTR("Playlist: %2d/%2d"), sound.getPlaylistPlace(),
					sound.getPlaylistSize());
			ZD.printProgrammInfoZeilen(1, 1, buf);
			sprintf_P(buf, PSTR("Playing: F:%2d S:%2d"), sound.getPlFolder(),
					sound.getPlSong());
			ZD.printProgrammInfoZeilen(2, 1, buf);
			sprintf_P(buf, PSTR("YX5300 Status: %d"), sound.mp3D.lastMp3Status);
			ZD.printProgrammInfoZeilen(3, 1, buf);
			sprintf_P(buf, PSTR("Audio Status: %d"), sound.pruefe());
			ZD.printProgrammInfoZeilen(4, 1, buf);
			sprintf_P(buf, PSTR("MP3 Status: %d"), sound.mp3D.playStatus);
			ZD.printProgrammInfoZeilen(5, 1, buf);
			sprintf_P(buf, PSTR("Audio Standby: %d"), sound.mp3D.standby);
			ZD.printProgrammInfoZeilen(6, 1, buf);
			break;
		case 5: //LKJ LICHT
			ZD.printProgrammInfo(F("Lichtprogramm"));
			ZD.printProgrammInfoZeilen(1, 1, F("ICHT: An")); //4248
			ZD.printProgrammInfoZeilen(2, 1, F("2 Auto aus"));
			ZD.printProgrammInfoZeilen(3, 1, F("3 Auto an"));
			ZD.printProgrammInfoZeilen(4, 1, F("4 Licht aus"));
			ZD.printProgrammInfoZeilen(1, 2, F("6 SchLampe an"));
			ZD.printProgrammInfoZeilen(2, 2, F("7 SchLampe aus"));

			break;
		case 6: //MNO MIDI
			ZD.printProgrammInfo(F("MIDI Steuerung"));
			break;
		case 7: //PQRS Schlafen
			ZD.printProgrammInfo(F("Schlafprogramm"));
			ZD.printProgrammInfoZeilen(1, 1, F("LEEP: Zapfenstreich")); //5337
			ZD.printProgrammInfoZeilen(2, 1, F(""));
			break;
		case 8: //TUV VENTIL
			ZD.printProgrammInfo(F("Ventilsteuerung"));
			ZD.printProgrammInfoZeilen(1, 1, F("Wert 0 - 100"));
			ZD.printProgrammInfoZeilen(2, 1, F("wählen um Ventil"));
			ZD.printProgrammInfoZeilen(3, 1, F("einzustellen."));
			ZD.printProgrammInfoZeilen(4, 1, F("NICHT HERUMSPIELEN!"));
			break;
		case 9: //WXY
			ZD.printProgrammInfo(F("MP3 Player"));
			ZD.printProgrammInfoZeilen(1, 1, F("1 Vor"));
			ZD.printProgrammInfoZeilen(1, 2, F("3 Zurück"));
			ZD.printProgrammInfoZeilen(2, 1, F("2 Pause"));
			ZD.printProgrammInfoZeilen(2, 2, F("9 Reset PL"));
			ZD.printProgrammInfoZeilen(3, 1, F("Zur Playlist hinzufügen:"));
			ZD.printProgrammInfoZeilen(4, 1, F("1 Zahl Ordner, 2 Song"));
			ZD.printProgrammInfoZeilen(5, 1, F("Zufallsplaylist erstellen"));
			ZD.printProgrammInfoZeilen(6, 1, F("1 wählen + 1 Zahl Ordner"));
			break;
		default:
			break;
		}
	}
}

void spezialprogramm(uint32_t input) {
	uint16_t varSet = 0;
	uint16_t varContent = 0;
	if (input > 9999 && input < 100000) {
		varSet = input / 10000;  //1x Varset
		varContent = input % 10000;  //4x Content
	} else if (input > 999 && input < 10000) {
		varSet = input / 1000;  //1x Varset
		varContent = input % 1000;  //1x Content
	} else if (input > 99 && input < 1000) {
		varSet = input / 100;  //1x Varset
		varContent = input % 100;  //1x Content
	} else if (input > 19 && input < 100) {
		varSet = input / 10; //1x zahl Varset
		varContent = input % 10; // 1x Content
	} else if (input > 10 && input < 20) {
		preZapf(input);
	}

	switch (varSet) {
	case 2: //Apparat
		switch (varContent) {
		case 5326: // lean
			reinigungsprogramm();
			logbuch.logSystemMsg(F("CLEAN: Reinigungsprogramm gestartet"));
			break;
		case 8300: // a ufnull
			user.clearAllUserData();
			logbuch.logSystemMsg(F("AUFNULL: Alle Daten gelöscht"));
		}
		break;

	case 3:
		// F für Fass
		if (varContent < 65) {
			user.restMengeFass = varContent * 1000;
			ZD.showAllUserData();
			sprintf_P(buf, PSTR("Neues Fass mit %d Litern installiert"),
					varContent);
			ZD.infoText(1, buf);
			logbuch.logSystemMsg(buf);
		} else {
			ZD.infoText(1, F("Fassgröße über 65l nicht möglich!"));
		}
		break;

	case 5:
		// LICHT
		switch (varContent) {
		case 4248: /* LICHT*/
			flowmeter.flowDataSend(LED_FUN_4, 0b11111111, 0xFF);
			digitalWrite(Z_SCH_LAMPE_PIN, 1);
			break;
		case 2: //auto aus
			power.setAutoLight(0);
			break;
		case 3: //auto an
			power.setAutoLight(1);
			break;
		case 4: // Licht aus
			break;
		case 6: // SchLampe an
			power.schLampeControl(1);
			break;
		case 7: // SchLampe aus
			power.schLampeControl(0);
			break;
		}
		break;
	case 7: //sleep
		switch (varContent) {
		case 5337: //LEEP
			power.goSleep();
			logbuch.logSystemMsg(F("SLEEP: Schlafprogramm initialisiert"));
			break;
		}
		break;

	case 8:
		//Ventil
		if (varContent > 100) {
			varContent = 100;
		}
		ventil.setValveProzent(varContent);
		ventil.check();
		break;
	case 9:
		//Mediaplayer
		switch (varContent) {
		case 1:
			sound.mp3NextSongOnPlaylist();
			break;
		case 2:
			sound.mp3Pause();
			sprintf_P(buf, PSTR("status %d"), sound.mp3D.playStatus);
			ZD.infoText(1, buf);
			break;
		case 3:
			sound.mp3PreviousSongOnPlaylist();
			break;
		case 9:
			sound.mp3ClearPlaylist();
			break;
		default:
			if (varContent > 100) {
				uint8_t folder = 1;
				uint8_t song = 1;
				folder = 30 + (varContent / 100);
				song = varContent % 100;
				sound.mp3AddToPlaylist(folder, song);
				sprintf_P(buf,
						PSTR("Lied %d in Ordner %d zu Playlist hinzugefügt"),
						20 + varContent, sound.mp3D.songsInPlayList);
				ZD.infoText(1, buf);
				logbuch.logSystemMsg(buf);
			} else if (varContent > 10 && varContent < 20) {
				sound.mp3FillShufflePlaylist(20 + varContent);
				sprintf_P(buf, PSTR("Playlist Ordner %d, %d Files"),
						20 + varContent, sound.mp3D.songsInPlayList);
				ZD.infoText(1, buf);
				logbuch.logSystemMsg(buf);
				delay(2000);
			} else {
				ZD.infoText(1, F("Kannst Du irgendwas?"));
			}
			delay(1);
			break;
		}
		break;

	default:
		ZD.infoText(1, F("Auswahl nicht möglich"));
		kienmuehle = 0;
		break;
	}
}
