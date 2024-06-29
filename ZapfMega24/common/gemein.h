/*
 * gemein.h
 *
 *  Created on: 28.05.2023
 *      Author: alfred3
 */

#ifndef GEMEIN_H_
#define GEMEIN_H_

#include "Arduino.h"
#include "stdint.h"
#include <stdio.h>
#include <string.h>

/*
 * HARDWARE
 */

//SD Karte
#define SD_CS     53 //CS Pin am MEGA für SPI

//Input Pins
#define helligkeitSensor  A6

// Taster etc
#define taste1            37  //Taster unterm Drehencoder (hardwareentprellt)
#define taste2            39  //Taster unterm Drehencoder (hardwareentprellt)
#define WSready           19  //Wählscheibe Ready Interrupt
#define WSpuls            33  //Wählscheibe puls
#define ROTARY_SW_PIN	  18  //Knopf vom Rotary Encoder (hardwareentprellt)
#define ROTARY_DT_PIN	  3   //RotaryEnc DT(INT)  D3  D2  RotaryEnc CLK (INT)
#define ROTARY_CLK_PIN	  2

//Output Pins
#define otherMcOn         A5  //schaltet die anderen Microcontroller mit P-Mosfet ein
#define taste1Pwm         6   //Tastenbeleuchtung via Optokoppler (12VLed eingebaut)
#define taste2Pwm         8   //Tastenbeleuchtung via Optokoppler (12VLed eingebaut)
#define lcdBacklightPwm   4   //Hintergrundbeleuchtung Display
#define FLOW_SM6020       A9
#define FLOW_WINDOW		  A8
#define TASTE1_LED 6
#define TASTE2_LED 8

// Darstellung
#define BLACK   0x0000
#define RED     0xF800
#define GREEN   0x07E0
#define WHITE   0xFFFF
#define WGRUEN  0x05AA
#define ZGRUEN  0x06AB
#define ZBRAUN  0xE6DA //dunkelbraun
#define ZHELLBRAUN 0xEF3B //hell
#define ZHELLGRUEN 	0x0428  //Zapf hell
#define ZDUNKELGRUEN 	0x0326  //Zapf dunkel
#define GREY    0x8410
#define NORMAL FreeSans12pt7b
#define FETT FreeSansBold12pt7b
#define KOMMA 1
#define GANZZAHL 0


// COMMUNICATION

//Communication

/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : communication.h
 * @brief          : Defines für I2C Kommunication
 *                   Sollte für alle uC verwendbar sein
 *
 *                    Die uC übertragen immer drei Bytes über I2C.
 *                    1. Byte: Befehl (hier als const angelegt)
 *                    2. Byte: normal HIGH Byte vom int
 *                    3. Byte: normal LOW Byte vom int
 *
 ******************************************************************************
 * @attention
 *
 * 2022 Z.Gesellschaft / Zapfapparat 0.6
 *
 ******************************************************************************
 */
/* USER CODE END Header */

//#include "Arduino.h"

/**
 * Grundlegend I2C Adressen
 */
#define flowi2c                 0x12   // Adresse vom Flow (in Cube linksshiften 1 Bit!)
#define tempi2c                 0x13   // Adresse vom Temp

#define FLOW_I2C_ADDR  0x12  // Flowzähl uC (in Cube 1 bit linksshiften!)
#define FLOW_I2C_ANTWORTBYTES  2 // die menge an Antwortbytes
#define TEMP_I2C_ADDR  0x13    // Temperaturregel uC
/*
 *  Definitionen für Bytes
 *  FLOWMETER+TEMPERATUR
 *  										ALLES 2 Bytes -> 1 uint16_t
 *  							MASTER		SLAVE			MASTER
 *								SEND		RETURN			SEND
 *******************************************************************************/

#define ebiMode               	0xF9      //1 an, 0 aus, Temperatur auf 2°C, Hahn auf, Zapfmusik
#define beginZapf             	0xFA      //Beginn das Zapfprogramm -> PID auf aggressiv
#define endZapf              	0xFB      //zapfMillis
#define kurzBevorZapfEnde     	0xFC      //sagt das wir kurz vor Ende sind → Valve schließen -> PID auf konservativ
#define lowEnergy             	0xFD      //LEDS nicht benutzen  byte3: 0: vollgas   1: sparen
#define wachAuf               	0xFE      //mach was
#define zapfenStreich         	0xFF      //Valve Schließen, LEDs aus

#define EBI_MODE  0xF9    // don't know really know
#define BEGIN_ZAPF  0xFA  // Zapfprogramm beginnen
#define END_ZAPF  0xFB // Zapfprogramm beenden, gezapfte Milliliter übertragen
#define KURZ_VOR_ZAPFENDE  0xFC // sagt das wir kurz vor Ende sind → Valve schließen -> PID auf konservativ
#define LOW_ENERGY  0xFD // //LEDS nicht benutzen  byte3: 0: vollgas   1: sparen
#define WACH_AUF  0xFE // alles wieder hochfahren und für den neuen Tag bereiten
#define ZAPFEN_STREICH  0xFF // alles runterfahren, licht aus, ton aus. Halt die Klappe, ich hab Feierabend.

/*
 *  Definitionen für Bytes
 *  FLOWMETER
 *
 *  Der Flowmeter schickt IMMER die aktuellen zapfMillis zurück.
 *  Diese werden dann vom Master durch setUserMilliLitres oder beginZapf wieder
 *  auf 0 zurückgestellt.
 *
 *  								ALLES 2 Bytes -> 1 uint16_t
 *  							MASTER		SLAVE			MASTER
 *								SEND		RETURN			SEND
 *******************************************************************************/
#define sendMilliLitres       	0x01      //zapfMillis		2 Bytes egal was
#define setUserMilliLitres    	0x21      //				userMilliLiter
#define makeFunWithLeds1      	0x22      //zapfMillis		Laufzeit (0-FF), Delay (0-FF)
#define makeFunWithLeds2      	0x23      //zapfMillis
#define makeFunWithLeds3      	0x24      //zapfMillis
#define makeFunWithLeds4      	0x25      //Gauselmann Mode.
/*** GAUSELMANN HOW TO:
 *  Befehl: 25 (hex!)
 *  Byte 1: Was für LED:
 *          einfach BINÄR eingeben! z.B. 0b01101100 (wär in HEX 6C)
 *  Byte 2: Helligkeit (00-FF)
 *
 */
#define GET_ML 0x01
#define SET_USER_ML 0x21
#define LED_FUN_1 0x22
#define LED_FUN_2 0x23
#define LED_FUN_3 0x24
#define LED_FUN_4 0x25

//God MODES
#define IDDQD 1
#define KEEN 2

#endif /* GEMEIN_H_ */
