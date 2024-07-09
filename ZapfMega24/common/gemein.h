/*
 * gemein.h
 *
 *  Created on: 28.05.2023
 *      Author: alfred3
 */

#ifndef GEMEIN_H_
#define GEMEIN_H_
/*
 * Allgemein
 */
#define _NAME_ "ZAPFAPPARAT"
#define _VERSION_ "V 0.6 Beta 2024"
#define MIN_TEMP 200 /* 2°C, kälter sollts nicht eingestellt sein */
#define MIN_MENGE 20
#define STANDARD_TEMP 500
#define STANDARD_MENGE 250



//SD Karte
#define SD_CS     53 //CS Pin am MEGA für SPI

// Taster etc
#define TASTE1_PIN		  37//Taster unterm Drehencoder (hardwareentprellt)
#define TASTE2_PIN		  39
#define ROTARY_SW_PIN	  18  //Knopf vom Rotary Encoder (hardwareentprellt)
#define ROTARY_DT_PIN	  3   //RotaryEnc DT(INT)  D3  D2  RotaryEnc CLK (INT)
#define ROTARY_CLK_PIN	  2

//Output Pins
#define FLOW_SM6020       A9  // ein aus
#define FLOW_WINDOW		  A8   //hier schickt der flowmeter ein signal wenns flowt

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
#define ZDUNKELGRUEN 0x0326  //Zapf dunkel
#define GREY    0x8410
#define NORMAL FreeSans12pt7b
#define FETT FreeSansBold12pt7b
#define KOMMA 1
#define GANZZAHL 0
#define WARTE_ZEIT 30000



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
/*
 *  Definitionen für Bytes
 *  FLOWMETER+TEMPERATUR
 *  										ALLES 2 Bytes -> 1 uint16_t
 *  							MASTER		SLAVE			MASTER
 *								SEND		RETURN			SEND
 *******************************************************************************/
#define EBI_MODE  0xF9    // don't know really know
#define BEGIN_ZAPF  0xFA  // Zapfprogramm beginnen
#define END_ZAPF  0xFB // Zapfprogramm beenden, gezapfte Milliliter übertragen
#define KURZ_VOR_ZAPFENDE  0xFC // sagt das wir kurz vor Ende sind → Valve schließen -> PID auf konservativ
#define LOW_ENERGY  0xFD // //LEDS nicht benutzen  byte3: 0: vollgas   1: sparen
#define WACH_AUF  0xFE // alles wieder hochfahren und für den neuen Tag bereiten
#define ZAPFEN_STREICH  0xFF
 // alles runterfahren, licht aus, ton aus. Halt die Klappe, ich hab Feierabend.

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

/**
 * Grundlegend I2C Adressen
 */

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
#define MAGNUM 3
#define MACGYVER 4
#define MIAMI 5
#define SEINFELD 6
#define ALF 7
#define COLT 8
#define DOTT 9
#define INDY 10
#define JUBI 11



#endif /* GEMEIN_H_ */
