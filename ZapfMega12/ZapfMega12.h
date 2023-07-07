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
#include <MD_YX5300.h>  // MP3-Player
#include <MD_MIDIFile.h> // MIDI SMF Player
#include <Adafruit_PWMServoDriver.h>   //PWM LED wählscheibe
#include "./zLibraries/RTC_DCF/DateTime.h"   // ELV RTC mit DCF
#include "./zLibraries/RTC_DCF/RealTimeClock_DCF.h"
#include <Adafruit_Thermal.h> //Thermal Printer
#include "benutzer.h"
#include "tempsens.h"
#include "audio.h"

// Includes aus

// Defines
#define USE_SDFAT
#define ENCODER_USE_INTERRUPTS  //damit die vom MEGA drin sind und er die als INT setzt
#define MP3Stream Serial3  // Hängt am Serial Port 3 für MP3-Player
//#include <MD_cmdProcessor.h>   //weiss nicht ob wir den brauchen
#define USE_SOFTWARESERIAL 0   ///kein Softwareserial, native Port! - für MIDI
#define MIDI Serial // MIDI hängt am Serial0
#define MIDI_SERIAL_RATE 31250

#define DEBUGTO 3  //0 Nothing, 1 Serial, 2 Printer, 3 Display, 4 SD File
#if DEBUGTO == 3
#define DEBUGMSG(s) { ZD.printText(); ZD._tft.println(s); }
#endif
// Defines aus



// Funktionen
void setup(void) ;
void waehlscheibe() ;
void waehlFunktionen() ;
void midiCallback(int *pev)    		;
void sysexCallback(int *pev)     		;
void midiSilence(void)    		;
void anfang(void) ;
void aufWachen(void) ;
void einSchlafen(void) ;
void tickMetronome(void)  		;
void seltencheck(void) ;
void belohnungsMusik() ;
void infoseite(void) ;
void loop() ;
void valveControl(uint8_t onoff) ;
void userShow(void) ;
void iBefehl(uint8_t empfaenger, uint8_t befehl) ;
void iDataSend(byte empfaenger, byte befehl, unsigned int sendedaten) ;
void i2cIntDataSend(byte empfaenger, byte befehl, unsigned int sendedaten) ;
void flowDataSend(uint8_t befehl, uint8_t option1, uint8_t option2, 		uint16_t wert) ;
void anzeigeAmHauptScreen(void) ;
void dataLogger(void) ;
void printerSleep(void) ;
void printerWakeUp(void) ;
void printerButtonPressed() ;
void printerZapfEnde(unsigned int zahl) ;
void printerErrorZapfEnde(unsigned int zahl) ;
void printMessage(String printMessage) ;
void printFeed(int feedrate) ;
void printerTest() ;
void printerSetup() ;
void UserDataShow() ;
void Drehgeber() ;
void Einstellerumsteller_ISR() ;
void flowDataSend(uint8_t empfaenger, uint8_t option1, uint8_t option2, uint16_t wert);

// Variablen
char buf[80];
bool oldFlowWindow;
bool flowWindow;



// Variablen aus

#endif /* ZAPFMEGA12_H_ */
