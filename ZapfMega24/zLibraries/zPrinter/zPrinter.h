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
#include "stdint.h"

#define PRINTER_ON_PIN    38 // Schaltet Printer ein, Printer ist an Serial2 (RX17, TX16)
#define printerBaudRate   9600
#define PRINTER_BAUDRATE  9600
#define PRINTER_DTR 	  A10

class zPrinter : public Adafruit_Thermal, benutzer {
public:
	zPrinter();
	virtual
	~zPrinter();
	void initialise();
	void printerSleep(void);
	void printerWakeUp(void);
	void printerButtonPressed();
	void printerZapfEnde(uint16_t zahl);
	void printerErrorZapfEnde(unsigned int zahl);
	void printMessage(String printMessage);
	void printFeed(int feedrate);
	void printerTest();
	void printerSetup();
	void printKaethe();
	inline void setPrinterOn() {
		printerOn = true;
	}
	inline void setPrinterOff() {
		printerOn = false;
	}
	Adafruit_Thermal printer;// = Adafruit_Thermal(Serial2, PRINTER_DTR);//(HardwareSerial *serial, uint8_t dtr); //PRINTERm, Hardware Serial2 DTR pin

private:
	//Adafruit_Thermal *_printer;
	bool printerOn;
};

#endif /* ZPRINTER_H_ */
