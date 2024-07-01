/*
 * waehlscheibe.h
 *
 *  Created on: 29.06.2024
 *      Author: al
 */

#ifndef WAEHLSCHEIBE_H_
#define WAEHLSCHEIBE_H_
#define WSready           19  //Wählscheibe Ready Interrupt
#define WSpuls            33  //Wählscheibe puls
#define WS_LED_ADDRESS     0x40
#define WS_LED_FREQUENCY   400     //min 24Hz, max 1524Hz
#define GRUEN_LED_ABGEDUNKELT 40
#define WEISS_LED_ABGEDUNKELT 20

//#include "Adafruit_PWMServoDriver.h"   //PWM LED wählscheibe, VOR DEM DISPLAY includen!!!!!!!!!!!


void beginWaehlscheibeLed(void);
void wsLedGrundbeleuchtung(void);
uint8_t readWaehlscheibe(void);
void oldWaehlscheibeFun(void);




#endif /* WAEHLSCHEIBE_H_ */
