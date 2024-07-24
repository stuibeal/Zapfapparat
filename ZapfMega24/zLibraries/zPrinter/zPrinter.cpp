/*
 * zprinter.cpp
 *
 *  Created on: 15.07.2023
 *      Author: alfred3
 */

#include "zPrinter.h"
#include "globalVariables.h"
#include "Adafruit_Thermal.h"
#include "benutzer.h"
#include "Arduino.h"
#include "avr/pgmspace.h"

Adafruit_Thermal printer(&Serial2, PRINTER_DTR);

zPrinter::zPrinter() {
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
	delay(200);
	printer.begin();
	delay(200);
	printer.setCharset(CHARSET_GERMANY);
	printer.setHeatConfig(6, 200, 80);
	printer.println(F(" "));
	printer.println(F(_NAME_));
	printer.println(F(_VERSION_));
	//printer.setLineHeight(24);
	printer.feed(20);
	printer.sleep();
	//_printer = &printer;
	//digitalWrite (PRINTER_ON_PIN, LOW);

}

void zPrinter::printerSleep(void) {

	printer.sleep();
	//digitalWrite (PRINTER_ON_PIN, LOW);
}

void zPrinter::schaltAus(void) {
	printer.sleep();
	digitalWrite(PRINTER_ON_PIN, LOW);

}

void zPrinter::schaltEin(void) {
	digitalWrite(PRINTER_ON_PIN, HIGH);
	delay(1000);

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

void zPrinter::printerZapfEnde() {
	if (printerOn) {
		temp.requestSensors();
		printerWakeUp();
		printer.justify('C');
		printer.setSize('M');

		printer.print(F("Z-Gesellschaft\n"));
		printer.println(F(_NAME_));
		printer.setSize('S');
		printer.println(F(_VERSION_));
		printer.justify('C');
		printer.setSize('L');
		printer.underlineOn(2);
		printer.println(user.getName());
		printer.underlineOff();
		printer.setSize('S');
		printer.justify('L');
		printer.print(F("Zapfmenge: "));
		printer.print((int) user.lastZapfMenge);
		printer.print(F(" ml\n"));
		printer.print(F("Überzapfung: "));
		printer.print((int) (user.lastZapfMenge - user.getMenge()));
		printer.println(F(" ml"));
		printer.print(F("Nachzapfung: "));
		printer.print((int) user.zapfMenge);
		printer.println(F(" ml"));
		printer.print(F("Strombedarf: "));
		printer.print(temp.getStromVerbrauchLetzteZapfung());
		printer.println(F(" Wh"));
		printer.print(F("Tagesmenge: "));
		printer.print(user.getBierTag());
		printer.println(F(" ml"));
		printer.print(F("Gesamtmenge: "));
		printer.print(user.getBierGesamt());
		printer.println(F(" ml"));
		printer.print(F("Zapfhahntemperatur: "));
		temp.requestSensors();
		printer.print(temp.getHahnTemp()/100);
		printer.print(F(","));
		printer.print(temp.getHahnTemp()%100);
		printer.println(F("°C"));

		if (user.lastZapfMenge - user.getMenge() + user.zapfMenge == 0) {
			printer.setSize('L');
			printer.println(F("DU BIST DER"));
			printer.println(F("ZAPFHELD DES"));
			printer.println(F("BAYERISCHEN"));
			printer.println(F("VATERLANDS!"));
		}
		if (user.aktuell == 3) {
			printer.setSize('L');
			printer.println(F("OPTIMAL!"));
		}
		logbuch.getClockBarcode();
		printer.printBarcode(buf, EAN13);
		printer.feed(30);
		printerSleep();
	}
}

void zPrinter::printerErrorZapfEnde() {
	if (printerOn) {
		printerWakeUp();
		printer.justify('C');
		printer.setSize('M');
		printer.println(F("HEY! DU HONK!"));
		printer.println(F("Wir sind nicht"));
		printer.println(F("zum Spaß hier."));
		printer.setSize('S');
		printer.print(user.getName());
		printer.println(F(", Du hast nur"));
		printer.println(F("windige, erbärmliche"));
		printer.print(user.zapfMenge);
		printer.println(F(" ml gezapft."));
		printer.setSize('L');
		printer.println(F("Schäme Dich!"));
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

