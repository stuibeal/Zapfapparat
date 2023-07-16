/*
 * z_printer->cpp
 *
 *  Created on: 15.07.2023
 *      Author: alfred3
 */

#include "zPrinter.h"

zPrinter::zPrinter ()
  //:Adafruit_Thermal(0, 0)

{
  _user = nullptr;
  _printer = nullptr;
  _buf = nullptr;
  printerOn = 1;

}

zPrinter::~zPrinter ()
{

}

void
zPrinter::initialise (HardwareSerial *S, benutzer *_puser, char *pbuf) {
  _user = _puser;
  _buf = pbuf;
  Adafruit_Thermal *_printer = new Adafruit_Thermal(S, PRINTER_DTR); //PRINTERm, Hardware Serial2 DTR pin
  pinMode (PRINTER_ON_PIN, OUTPUT);
  //pinMode (PRINTER_DTR, INPUT);
  digitalWrite (PRINTER_ON_PIN, HIGH);
  S->begin (9600);
  delay (1000);
  _printer->begin ();
  delay (1000);
  _printer->println ("HONK");
  _printer->sleep ();
  //digitalWrite (PRINTER_ON_PIN, LOW);

}

void
zPrinter::printerSleep (void)
{

  _printer->sleep ();
  //digitalWrite (PRINTER_ON_PIN, LOW);
}


/*
 * Command: wake up printer for action
 */
void
zPrinter::printerWakeUp (void)
{
  digitalWrite (PRINTER_ON_PIN, HIGH);
  //delay(1000);
  //_printer->begin();
  //delay(1000);
  _printer->wake ();       // MUST wake() before printing again, even if reset
  _printer->setDefault (); // Restore printer to defaults
}

/*
 * Command: run simple print job when button pressed
 */
void
zPrinter::printerButtonPressed ()
{
  printerWakeUp ();
  _printer->justify ('C');
  _printer->setSize ('L');
  _printer->println (F("Bier her"));
  _printer->println (F("Bier her"));
  _printer->println (F("oda I foll um"));
  _printer->feed (2);
  printerSleep ();
}

void
zPrinter::printerZapfEnde (unsigned int zahl)
{
  if (printerOn)
    {
      printerWakeUp ();
      _printer->justify ('C');
      _printer->setSize ('S');
      _printer->println (_user->getName ());
      _printer->print (zahl);
      _printer->println (" ml gezapft!");
      _printer->setSize ('S');
      _printer->println (_user->gesamt ());
      if (_user->aktuell == 3)
	{
	  _printer->setSize ('L');
	  _printer->println ("OPTIMAL!");
	}
      _printer->feed (20);
      printerSleep ();
    }
}

void
zPrinter::printerErrorZapfEnde (unsigned int zahl)
{
  if (printerOn)
    {
      printerWakeUp ();
      _printer->justify ('C');
      _printer->setSize ('S');
      sprintf (_buf, "Du hast nur %d ml gezapft!", zahl);
      _printer->println (_buf);
      _printer->setSize ('L');
      _printer->println (_user->getName ());
      _printer->println ("Schämen Sie sich!");
      _printer->setSize ('S');
      _printer->feed (20);
      printerSleep ();
    }
}

void
zPrinter::printMessage (String printMessage)
{
  printerWakeUp ();
  _printer->setSize ('S');
  _printer->justify ('L');
  _printer->println (printMessage);
  printerSleep ();
}

void
zPrinter::printFeed (int feedrate)
{
  printerWakeUp();
  _printer->feed (feedrate);
  printerSleep ();
}

/*
 * Command: run a test print job
 */
void
zPrinter::printerTest ()
{
  printerWakeUp ();

  // Test inverse on & off
  _printer->inverseOn ();
  _printer->println (F("Inverse ON"));
  _printer->inverseOff ();

  // Test character double-height on & off
  _printer->doubleHeightOn ();
  _printer->println (F("Double Height ON"));
  _printer->doubleHeightOff ();

  // Set text justification (right, center, left) -- accepts 'L', 'C', 'R'
  _printer->justify ('R');
  _printer->println (F("Right justified"));
  _printer->justify ('C');
  _printer->println (F("Center justified"));
  _printer->justify ('L');
  _printer->println (F("Left justified"));

  // Test more styles
  _printer->boldOn ();
  _printer->println (F("Bold text"));
  _printer->boldOff ();

  _printer->underlineOn ();
  _printer->println (F("Underlined text"));
  _printer->underlineOff ();

  _printer->setSize ('L');        // Set type size, accepts 'S', 'M', 'L'
  _printer->println (F("Large"));
  _printer->setSize ('M');
  _printer->println (F("Medium"));
  _printer->setSize ('S');
  _printer->println (F("Small"));

  _printer->justify ('C');
  _printer->println (F("normal\nline\nspacing"));
  _printer->setLineHeight (50);
  _printer->println (F("Taller\nline\nspacing"));
  _printer->setLineHeight (); // Reset to default
  _printer->justify ('L');

  // CODE39 is the most common alphanumeric barcode:
  _printer->printBarcode ("LEAP", CODE39);
  _printer->setBarcodeHeight (100);
  // Print UPC line on product barcodes:
  _printer->printBarcode ("123456789123", UPC_A);

  // Print QR code bitmap:
  //_printer->printBitmap(qrcode_width, qrcode_height, qrcode_data);

  _printer->feed (2);
  printerSleep ();
}

/*
 * Command: one-time setup
 */
void
zPrinter::printerSetup ()
{

}

void
zPrinter::printKaethe() {
  printMessage("Z-Apfapparat Version 0.4");
  printMessage("ready!");
  printMessage("Prost!");
  printMessage("Ich weis nichts");  //programmiert von Käthe 30.7.22
  printFeed(4);
}

