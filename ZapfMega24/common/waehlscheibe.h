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



void beginWaehlscheibe(void);
uint8_t readWaehlscheibe(void);
void oldWaehlscheibeFun(void);




#endif /* WAEHLSCHEIBE_H_ */
