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

// One Wire Temperatursensoren Pins
const uint8_t ONE_WIRE_BUS30 = 30;
const uint8_t ONE_WIRE_BUS32 = 32;
const uint8_t ONE_WIRE_BUS34 = 34;
const uint8_t ONE_WIRE_BUS35 = 35;
const uint8_t ONE_WIRE_BUS36 = 36;



//Thermodrucker
#define PRINTER_ON_PIN    38 // Schaltet Printer ein, Printer ist an Serial2 (RX17, TX16)
#define printerBaudRate   9600
#define PRINTER_DTR 	  A10

//Old Things, good Things. Brauch ma nimmer.
#define oldValve          49  //Outputpin Magnetventil

//Input Pins
#define helligkeitSensor  A6
#define pressureSensor    A7

// Taster etc
#define taste1            37  //Taster unterm Drehencoder (hardwareentprellt)
#define taste2            39  //Taster unterm Drehencoder (hardwareentprellt)
#define WSready           19  //Wählscheibe Ready Interrupt
#define WSpuls            33  //Wählscheibe puls
#define rotaryKnob        18  //Knopf vom Rotary Encoder (hardwareentprellt)
#define rotaryDT          3
#define rotaryCLK         2  //RotaryEnc DT(INT)  D3  D2  RotaryEnc CLK (INT)

//Output Pins
#define otherMcOn         A5  //schaltet die anderen Microcontroller mit P-Mosfet ein
#define valveAuf          42  //mach auf den Hahn, geht hardwaremäßig nur wenn valveZu aus
#define valveZu           44  //mach zu den Hahn, geht hardwaremäßig nur wenn valveAuf aus
#define oldPump           5   // PWM outputpin Pumpe -> Reinigung!
#define taste1Pwm         6   //Tastenbeleuchtung via Optokoppler (12VLed eingebaut)
#define taste2Pwm         8   //Tastenbeleuchtung via Optokoppler (12VLed eingebaut)
#define lcdBacklightPwm   4   //Hintergrundbeleuchtung Display
#define FLOW_SM6020       A9
#define FLOW_WINDOW		  A8
const uint8_t TASTE1_LED = 6;
const uint8_t TASTE2_LED = 8;


// Audio
#define AUDIO_BOARD      41  //Audioboard mit Roland SCB-7 einschalten (DC Wandler wird geschaltet)
#define AUDIO_AMP        40  //wenn LOW sind die ClassD Verstärker gemutet
#define mp3RX             15  // Serial3: RX15 TX14: MP3 Player YX5300
#define mp3TX             14
#define MIDI_TX           1  // Serial: Midi Out
#define BEEP_OUT          11  // beeper out -> not really used?
#define MIDI_RESET        A14 // Reset Knopf vom SCB-7 (high: reset)


void setHardwareConstants(void);
void setBenutzerDaten(void);


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
 * 2022 Z.Gesellschaft / Zapfapparat 0.4
 * nochmal als const angelegt in besserer Schreibweise. beim nächsten
 * kompilieren der anden uC dann ändern!
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

const uint8_t FLOW_I2C_ADDR =  0x12;    // Flowzähl uC (in Cube 1 bit linksshiften!)
const uint8_t FLOW_I2C_ANTWORTBYTES = 2; // die menge an Antwortbytes
const uint8_t TEMP_I2C_ADDR =  0x13;    // Temperaturregel uC

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

const uint8_t EBI_MODE = 0xF9;    // don't know really know
const uint8_t BEGIN_ZAPF = 0xFA;  // Zapfprogramm beginnen
const uint8_t END_ZAPF = 0xFB;    // Zapfprogramm beenden, gezapfte Milliliter übertragen
const uint8_t KURZ_VOR_ZAPFENDE = 0xFC; // sagt das wir kurz vor Ende sind → Valve schließen -> PID auf konservativ
const uint8_t LOW_ENERGY = 0xFD;         // //LEDS nicht benutzen  byte3: 0: vollgas   1: sparen
const uint8_t WACH_AUF = 0xFE;      // alles wieder hochfahren und für den neuen Tag bereiten
const uint8_t ZAPFEN_STREICH = 0xFF;  // alles runterfahren, licht aus, ton aus. Halt die Klappe, ich hab Feierabend.

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


const uint8_t GET_ML = 0x01;  // sende die gezapften Milliliter. byte 2&3 egal -> schickt ml an Master zurück
const uint8_t SET_USER_ML = 0x21; // Master schickt die vom User gewählten ml an den Zapf uC
const uint8_t LED_FUN_1 = 0x22; // progrämmsche warp1, parameter: laufzeit, delay, leds von oben nach unten oder so
const uint8_t LED_FUN_2 = 0x23; // progrämmsche warp2, parameter: laufzeit, delay, leds nach mittig zusammen
const uint8_t LED_FUN_3 = 0x24; // progrämmsche warp3, parameter: laufzeit, delay,
const uint8_t LED_FUN_4 = 0x25; // progrämmsche GAUSELMANN, parameter: was für LEDS, helligkeit



/*
 *  Definitionen für Bytes
 *  TEMPERATURREGLER						ALLES 2 Bytes -> 1 uint16_t
 *  							MASTER		SLAVE			MASTER 		MASTER
 *								SEND		RETURN			SEND		Variable
 *******************************************************************************/
#define transmitBlockTemp       0x40      //blockTemp in °C*10
#define transmitAuslaufTemp     0x41  // Data send:   hahnTemp in °C*10
const uint8_t GET_BLOCK_TEMP = 0x40;
const uint8_t GET_AUSLAUF_TEMP = 0x41;
#define transmitPower           0x42  // Data send:   Leistung in W (power1+power2)
#define transmitInVoltage       0x43  // Data send:   inVoltage in V*100
#define transmitKuehlFlow       0x44  // Data send:   Durchfluss Kühlwasser (extern) pro 10000ms

#define setHighTemperatur       0x60  // Data get: Zieltemperatur Block * 100 (2°C)
#define setMidTemperatur        0x61  // Data get: Normale Temperatur in °C * 100 (6°C)
#define setLowTemperatur        0x62  // Data get: Energiespar Temperatur * 100 (9°C)
#define setMinCurrent           0x63  // Data get: Current in mA / 10 (11 = 0,11 A), Untere Regelgrenze
#define setLowCurrent           0x64  // Data get: current in mA / 10, Obere Regelgrenze bei wenig Strom
#define setMidCurrent           0x65  // Data get: Current in mA / 10, Obere Regelgrenze bei normalem Strom
#define setHighCurrent          0x66  // Data get: Current in mA / 10, Obere Regelgrenze bei gutem Strom

#define setNormVoltage          0x68  // Data get: norm Voltage * 100, passt normal, mehr als 9V macht wenig Sinn bei den Peltierelementen
#define setMaxVoltage           0x69  // Data get: max Voltage * 100, das wäre dann eigentlich die Batteriespannung
#define setLowBatteryVoltage    0x6A  // 11V Eingangsspannung
#define setMidBatteryVoltage    0x6B  // 12V Eingangsspannung
#define setHighBatteryVoltage   0x6C  // 13V Eingangsspannung

#define setWasserTemp           0x6D  // Data get: kühlwasserTemp in °C*100 vom DS18B20 Sensor vom Master: Fühler neben Peltier
#define setEinlaufTemp          0x6E  // Data get: Biertemperatur in °C*100 vom DS18B20 Sensor vom Master: Bierzulauf

#define setConsKp               0x70  // Data get: konservativer Kp
#define setConsKi               0x71  // Data get: konservativer Ki
#define setConsKd               0x72  // Data get: konservativer Kd
#define setAggKp                0x73  // Data get: aggressiver Kp
#define setAggKi                0x74  // Data get: aggressiver Ki
#define setAggKd                0x75  // Data get: aggressiver Kd
#define setUnterschiedAggPid    0x75  // mal zehn grad nehmen ab wann der aggressiv regelt
#define setSteuerZeit           0x76  // alle sekunde mal nachjustieren






#endif /* GEMEIN_H_ */
