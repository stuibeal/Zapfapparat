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
void waehlscheibe() ;
void waehlFunktionen() ;
void anfang(void) ;
void aufWachen(void) ;
void einSchlafen(void) ;
void tickMetronome(void)  		;
void seltencheck(void) ;
void belohnungsMusik() ;
void infoseite(void) ;
void beginZapfProgramm(void);
void godModeZapfMidi(void);
void loop() ;
void userShow(void) ;
void anzeigeAmHauptScreen(void) ;
void dataLogger(void) ;
void UserDataShow() ;
void Drehgeber() ;
void Einstellerumsteller_ISR() ;
void oldWaehlscheibeFun(void);
void reinigungsprogramm(void);
void spezialprogramm(uint32_t input);




//Do not add code below this line
#endif /* _ZapfMega24_H_ */
