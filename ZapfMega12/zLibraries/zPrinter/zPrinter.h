/*
 * zPrinter.h
 *
 *  Created on: 15.07.2023
 *      Author: alfred3
 */

#ifndef ZPRINTER_H_
#define ZPRINTER_H_

#include "Arduino.h"
#include "gemein.h"
#include <Adafruit_Thermal.h> //Thermal Printer
#include "benutzer.h"


class zPrinter
{
public:
  zPrinter ();
  virtual
  ~zPrinter ();
  void initialise(HardwareSerial *S, benutzer *_puser, char* pbuf);
  void printerSleep(void) ;
  void printerWakeUp(void) ;
  void printerButtonPressed() ;
  void printerZapfEnde(unsigned int zahl) ;
  void printerErrorZapfEnde(unsigned int zahl) ;
  void printMessage(String printMessage) ;
  void printFeed(int feedrate) ;
  void printerTest() ;
  void printerSetup() ;
  void printKaethe();
  inline void setPrinterOn() {printerOn = true;}
  inline void setPrinterOff() {printerOn = false;}

private:
   Adafruit_Thermal printer (HardwareSerial *serial, uint8_t dtr); //PRINTERm, Hardware Serial2 DTR pin
   benutzer *_user;
   Adafruit_Thermal *_printer;
   bool printerOn;
   char *_buf;


};

#endif /* ZPRINTER_H_ */
