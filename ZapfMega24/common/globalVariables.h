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


//globale Variablen
extern zWireHelper flowmeter;
extern audio sound;
extern tempControl temp;
extern benutzer user;
extern zPower power;
extern char buf[80];
extern PCA9685 wsLed;
extern volatile uint8_t einsteller;
#endif /* GLOBALVARIABLES_H_ */

