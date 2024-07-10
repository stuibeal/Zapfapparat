// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _ZapfMega24_H_
#define _ZapfMega24_H_
#include "Arduino.h"
//add your includes for the project ZapfMega24 here


//end of add your includes here


//add your function definitions for the project ZapfMega24 here
// Funktionen
void setup(void) ;
void anfang(void) ;
void loop() ;
void zapfStandbyProg(void);
void zapfErrorProg(void);
void zapfBeginnProg(void);
void amZapfenProg(void);
void godZapfenProg(void);
void kurzVorZapfEndeProg(void);
void zapfEndeProg(void);
void checkImmer(void);
void checkWhileZapfing(void);
void dauerCheck(void);
void warteZeitCheck(void);
uint8_t readTaste(uint8_t taste);
void waehlscheibe() ;
void waehlFunktionen() ;
void seltencheck(void) ;
void belohnungsMusik() ;
void infoseite(void) ;
uint8_t errorLed(void);
void userShow(void) ;
void showZapfapparatData(void) ;
void dataLogger(void) ;
void drehgeber() ;
void einstellerUmsteller_ISR() ;
void reinigungsprogramm(void);
void spezialprogramm(uint32_t input);




//Do not add code below this line
#endif /* _ZapfMega24_H_ */
