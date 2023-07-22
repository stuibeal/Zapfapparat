/*
 * ZapfMega12.h
 *
 *  Created on: 29.05.2023
 *      Author: alfred3
 *
 *      TODO: write Zapfdata to EEPROM after Zapfung
 *      TODO: Debug/logging class (tbd) on of call 33284 for DEBUG
 *      TODO: audio!
 *      TODO: logging before MIDI AUDIO! (sd!!!)
 *
 *
 */

#ifndef ZAPFMEGA12_H_
#define ZAPFMEGA12_H_

// Includes
#include "Arduino.h"
#include "gemein.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <Wire.h>
#include "Adafruit_Thermal.h"
#include "zDisplay.h"
#include <SdFat.h>            // Use the SdFat library
#include <Encoder.h>  //für Drehencoder
#include <Adafruit_PWMServoDriver.h>   //PWM LED wählscheibe
#include "./zLibraries/RTC_DCF/DateTime.h"   // ELV RTC mit DCF
#include "./zLibraries/RTC_DCF/RealTimeClock_DCF.h"
#include "./zLibraries/zPrinter/zPrinter.h"
#include "benutzer.h"
#include "tempsens.h"
#include "audio.h"
#include "zValve.h"
#include "zLog.h"
#include "zWireHelper.h"


// Includes aus

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



// Funktionen
void setup(void) ;
void waehlscheibe() ;
void waehlFunktionen() ;
void anfang(void) ;
void aufWachen(void) ;
void einSchlafen(void) ;
void tickMetronome(void)  		;
void seltencheck(void) ;
void belohnungsMusik() ;
void infoseite(void) ;
void loop() ;
void userShow(void) ;
void anzeigeAmHauptScreen(void) ;
void dataLogger(void) ;
void UserDataShow() ;
void Drehgeber() ;
void Einstellerumsteller_ISR() ;
void oldWaehlscheibeFun(void);
void reinigungsprogramm(void);
void spezialprogramm(uint16_t input);

// Variablen
char buf[80];
bool oldFlowWindow;
bool flowWindow;

SdFat SD;  // SD-KARTE
zDisplay ZD;   // neues zDisplay Objekt
zWireHelper drahthilfe;


//Hier Variablen definieren
byte aktuellerTag = 1;  //dann gehts mit der Musik aus
unsigned int minTemp = 200;
unsigned int zielTemp = 200;
unsigned int totalMilliLitres = 0;
volatile byte WSpulseCount = 0;
unsigned long auswahlZeit;
int aktuellerModus = 0;
unsigned int hell;

unsigned long oldTime = millis ();
unsigned long nachSchauZeit = 0;
unsigned long hellZeit;
unsigned int hellCount = 0;
unsigned int dunkelCount = 0;
bool dunkelBool = false;

byte lichtan = LOW;

//Waehlscheibe
uint8_t zahlemann;  //Per Wählscheibe ermittelte Zahl
unsigned long kienmuehle;  //Sondereingabe bei drücken der Taste2
byte Einsteller = 1; //Globale Variable für ISR, Start bei 1

// aus Communication.h

unsigned int blockTemp; // #define transmitBlockTemp       0x40  // Data send:   blockTemp in °C*10
unsigned int auslaufTemp; // #define transmitAuslaufTemp     0x41  // Data send:   hahnTemp in °C*10
unsigned int power = 0; // #define transmitPower           0x42  // Data send:   Leistung in W (power1+power2)
unsigned int inVoltage = 0; // #define transmitInVoltage       0x43  // Data send:   inVoltage in V*100
unsigned int kuehlFlow = 0; // #define transmitKuehlFlow       0x44  // Data send:   Durchfluss Kühlwasser (extern) pro 10000ms

unsigned int highTemperatur = 200; // #define setHighTemperatur       0x60  // Data get: Zieltemperatur Block * 100 (2°C)
unsigned int midTemperatur = 600; // #define setMidTemperatur        0x61  // Data get: Normale Temperatur in °C * 100 (6°C)
unsigned int lowTemperatur = 900; // #define setLowTemperatur        0x62  // Data get: Energiespar Temperatur * 100 (9°C)
unsigned int minCurrent = 10; // #define setMinCurrent           0x63  // Data get: Current in mA / 10 (11 = 0,11 A), Untere Regelgrenze
unsigned int lowCurrent = 50; // #define setLowCurrent           0x64  // Data get: current in mA / 10, Obere Regelgrenze bei wenig Strom
unsigned int midCurrent = 600; // #define setMidCurrent           0x65  // Data get: Current in mA / 10, Obere Regelgrenze bei normalem Strom
unsigned int highCurrent = 900; // #define setHighCurrent          0x66  // Data get: Current in mA / 10, Obere Regelgrenze bei gutem Strom

unsigned int normVoltage = 500; //#define setNormVoltage          0x68  // Data get: norm Voltage * 100, passt normal, mehr als 9V macht wenig Sinn bei den Peltierelementen
unsigned int maxVoltage = 1000; //#define setMaxVoltage           0x69  // Data get: max Voltage * 100, das wäre dann eigentlich die Batteriespannung
unsigned int lowBatteryVoltage = 1140; //#define setLowBatteryVoltage    0x6A  // 11V Eingangsspannung
unsigned int midBatteryVoltage = 1200; //#define setMidBatteryVoltage    0x6B  // 12V Eingangsspannung
unsigned int highBatteryVoltage = 1300; //#define setHighBatteryVoltage   0x6C  // 13V Eingangsspannung

unsigned int wasserTemp; //#define setWasserTemp           0x6D  // Data get: kühlwasserTemp in °C*100 vom DS18B20 Sensor vom Master: Fühler neben Peltier
unsigned int einlaufTemp; //#define setEinlaufTemp          0x6E  // Data get: Biertemperatur in °C*100 vom DS18B20 Sensor vom Master: Bierzulauf

unsigned int consKp = 80; //#define setConsKp               0x70  // Data get: konservativer Kp (ist alles mal 100)
unsigned int consKi = 5; //#define setConsKi               0x71  // Data get: konservativer Ki
unsigned int consKd = 5; //#define setConsKd               0x72  // Data get: konservativer Kd
unsigned int aggKp = 150; //#define setAggKp                0x73  // Data get: aggressiver Kp
unsigned int aggKi = 20; //#define setAggKi                0x74  // Data get: aggressiver Ki
unsigned int aggKd = 50; //#define setAggKd                0x75  // Data get: aggressiver Kd
unsigned int unterschiedAggPid = 10; //#define setUnterschiedAggPid    0x75  // mal zehn grad nehmen ab wann der aggressiv regelt
unsigned int steuerZeit = 200; //#define setSteuerZeit           0x76  // alle sekunde mal nachjustieren

bool ebiModeBool = false; //#define ebiMode               0xF9      //    1 an, 0 aus       Temperatur auf 2°C, Hahn auf, Zapfmusik
bool beginZapfBool = false; // #define beginZapf             0xFA      //    Beginn das Zapfprogramm -> PID auf aggressiv
bool endZapfBool = false; //#define endZapf               0xFB      //    Data send : milliliter
bool kurzBevorZapfEndeBool = false; //#define kurzBevorZapfEnde     0xFC      //    sagt das wir kurz vor Ende sind → Valve schließen -> PID auf konservativ

String dataOnSd = "";

// Steuerungskram
unsigned int zielTemperatur = highTemperatur;
unsigned int setVoltage = normVoltage; //mal gaaanz klein beginnen
unsigned int setCurrent = minCurrent;
unsigned int maxCurrent = midCurrent; //Obere Regelgrenze auf mittlere stellen
bool lowPower = false;
bool veryLowPower = false;
bool debugMode = false;

// DREHENCODER
// encoder lib: http://www.pjrc.com/teensy/td_libs_Encoder.html
//#define ENCODER_OPTIMIZE_INTERRUPTS
Encoder Dreher (rotaryDT, rotaryCLK); //PINS für Drehgeber
//   Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
//   avoid using pins with LEDs attached

volatile int DreherKnopfStatus = 0; //Da wird der Statatus vom Drehgeberknopf gelesen
long oldPosition = 0; //Fuer Drehgeber

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver (); // called this way, it uses the default address 0x40
tempsens temp;	//Temperatursensorik
benutzer user;  //Benutzer
audio sound; 	//Audioobjekt
zValve ventil; // Ventilsteuerung, Druck, Reinigungspumpe
zPrinter drucker;
zLog logbuch;

unsigned int tempAnzeigeZeit = millis (); //für zehnsekündige Temperaturanzeige


// Variablen aus

#endif /* ZAPFMEGA12_H_ */
