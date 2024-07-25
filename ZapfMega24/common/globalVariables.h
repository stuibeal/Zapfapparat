/*
 * globalVariables.h
 *
 *  Created on: 01.07.2024
 *      Author: al
 */
#ifndef GLOBALVARIABLES_H_
#define GLOBALVARIABLES_H_
#include "zWireHelper.h"
#include "audio.h"
#include "tempControl.h"
#include "benutzer.h"
#include "zPower.h"
#include "string.h"
#include "zLog.h"
#include "../zDisplay.h"
#include "../zLibraries/zPrinter/zPrinter.h"
#include "zValve.h"
#include "SdFat.h"

//globale Variablen

extern zWireHelper flowmeter;
extern audio sound;
extern tempControl temp;
extern benutzer user;
extern zPower power;
extern char buf[80];
extern PCA9685 wsLed;
extern zLog logbuch;
extern zDisplay ZD;
extern zPrinter drucker;
extern zValve ventil;
extern volatile uint8_t einsteller;
extern uint8_t oldeinsteller;
extern SdFat SD;
extern void anfang();
#endif /* GLOBALVARIABLES_H_ */
