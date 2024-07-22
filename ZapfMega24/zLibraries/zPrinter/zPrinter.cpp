/*
 * zprinter.cpp
 *
 *  Created on: 15.07.2023
 *      Author: alfred3
 */

#include "zPrinter.h"
#include "globalVariables.h"
#include "./zLibraries/zAdafruit_Thermal_Printer_Library/Adafruit_Thermal.h"
#include "benutzer.h"
#include "z-logo-sw.h"

Adafruit_Thermal printer(&Serial2, PRINTER_DTR);


zPrinter::zPrinter()
{
	printerOn = 1;
}

zPrinter::~zPrinter() {

}

void zPrinter::initialise() {
	//Adafruit_Thermal *_printer = new Adafruit_Thermal(S, PRINTER_DTR); //PRINTERm, Hardware Serial2 DTR pin
	//Adafruit_Thermal printer = Adafruit_Thermal(S, PRINTER_DTR);
	pinMode(PRINTER_ON_PIN, OUTPUT);
	//pinMode (PRINTER_DTR, INPUT);
	digitalWrite(PRINTER_ON_PIN, HIGH);
	Serial2.begin(PRINTER_BAUDRATE);
	delay(1000);
	printer.begin();
	delay(1000);
	printer.println(_NAME_);
	printer.println(_VERSION_);
	//printer.setLineHeight(24);
	printer.feedRows(2);
	printer.sleep();
	//_printer = &printer;
	//digitalWrite (PRINTER_ON_PIN, LOW);

}

void zPrinter::printerSleep(void) {

	printer.sleep();
	//digitalWrite (PRINTER_ON_PIN, LOW);
}

/*
 * Command: wake up printer for action
 */
void zPrinter::printerWakeUp(void) {
	digitalWrite(PRINTER_ON_PIN, HIGH);
	//delay(1000);
	//printer.begin();
	//delay(1000);
	printer.wake();       // MUST wake() before printing again, even if reset
	printer.setDefault(); // Restore printer to defaults
}

/*
 * Command: run simple print job when button pressed
 */
void zPrinter::printerButtonPressed() {
	printerWakeUp();
	printer.justify('C');
	printer.setSize('L');
	printer.println(F("Bier her"));
	printer.println(F("Bier her"));
	printer.println(F("oda I foll um"));
	printer.feed(2);
	printerSleep();
}

void zPrinter::printerZapfEnde(uint16_t zahl) {
	if (printerOn) {
		printerWakeUp();

		printer.printBitmap(Z_LOGO_SW_WIDTH, Z_LOGO_SW_HEIGHT, z_logo_sw);
		printer.justify('L');
		printer.setSize('S');
		printer.setLineHeight(24);
		printer.print(F("Zapfkamerad "));
		printer.print(user.getName());
		printer.println(F(" hat gerade"));
		printer.print((int) zahl);
		printer.println(F(" ml gezapft!"));
		printer.print(F("Gesamtmenge des Tages: "));
		printer.print(user.gesamt());
		printer.println(F(" ml"));
		printer.print(F("Gesamtmenge des Tages: "));
		printer.print(user.gesamt());
		printer.println(F(" ml"));
		if (user.aktuell == 3) {
			printer.setSize('L');
			printer.println(F("OPTIMAL!"));
		}
		printer.printBarcode("2024081201334", EAN13);
		printer.feed(20);
		printerSleep();
	}
}

void zPrinter::printerErrorZapfEnde(unsigned int zahl) {
	if (printerOn) {
		printerWakeUp();
		printer.justify('C');
		printer.setSize('S');

		sprintf(buf, "Du hast nur %d ml gezapft!", zahl);
		printer.println(buf);
		printer.setSize('L');
		printer.println(user.getName());
		printer.println(F("Schämen Sie sich!"));
		printer.setSize('S');
		printer.feed(20);
		printerSleep();
	}
}

void zPrinter::printMessage(String printMessage) {
	printerWakeUp();
	printer.setSize('S');
	printer.justify('L');
	printer.println(printMessage);
	printerSleep();
}

void zPrinter::printFeed(int feedrate) {
	printerWakeUp();
	printer.feed(feedrate);
	printerSleep();
}

/*
 * Command: run a test print job
 */
void zPrinter::printerTest() {
	printerWakeUp();

	// Test inverse on & off
	printer.inverseOn();
	printer.println(F("Inverse ON"));
	printer.inverseOff();

	// Test character double-height on & off
	printer.doubleHeightOn();
	printer.println(F("Double Height ON"));
	printer.doubleHeightOff();

	// Set text justification (right, center, left) -- accepts 'L', 'C', 'R'
	printer.justify('R');
	printer.println(F("Right justified"));
	printer.justify('C');
	printer.println(F("Center justified"));
	printer.justify('L');
	printer.println(F("Left justified"));

	// Test more styles
	printer.boldOn();
	printer.println(F("Bold text"));
	printer.boldOff();

	printer.underlineOn();
	printer.println(F("Underlined text"));
	printer.underlineOff();

	printer.setSize('L');        // Set type size, accepts 'S', 'M', 'L'
	printer.println(F("Large"));
	printer.setSize('M');
	printer.println(F("Medium"));
	printer.setSize('S');
	printer.println(F("Small"));

	printer.justify('C');
	printer.println(F("normal\nline\nspacing"));
	printer.setLineHeight(50);
	printer.println(F("Taller\nline\nspacing"));
	printer.setLineHeight(); // Reset to default
	printer.justify('L');

	// CODE39 is the most common alphanumeric barcode:
	printer.printBarcode("LEAP", CODE39);
	printer.setBarcodeHeight(100);
	// Print UPC line on product barcodes:
	printer.printBarcode("123456789123", UPC_A);

	// Print QR code bitmap:
	//printer.printBitmap(qrcode_width, qrcode_height, qrcode_data);

	printer.feed(2);
	printerSleep();
}

/*
 * Command: one-time setup
 */
void zPrinter::printerSetup() {

}

void zPrinter::printKaethe() {
	printMessage(F("Z-Apfapparat Version 0.4"));
	printMessage(F("ready!"));
	printMessage(F("Prost!"));
	printMessage(F("Ich weis nichts"));  //programmiert von Käthe 30.7.22
	printFeed(4);
}

