/*
 * zDisplay.h
 *
 *  Created on: 28.05.2023
 *      Author: alfred3
 *
 *
 *      Hier ist das zDisplay Headerfile
 */

#ifndef ZDISPLAY_H_
#define ZDISPLAY_H_

#ifndef USE_SDFAT
#define USE_SDFAT
#endif

/*
 Für Display folgendes abändern in der Library:
 in mcufriend_shield.h: #define USE_SPECIAL
 in mcufriend_special.h: #define USE_MEGA_8BIT_PROTOSHIELD
 */

#include "Arduino.h"
#include "SdFat.h"
#include "gemein.h"
#include "stdio.h"
#include <string.h>
#include "./zLibraries/MCUFRIEND_kbv/MCUFRIEND_kbv.h"
//#include "Adafruit_GFX.h" // Hardware-specific library
#include "tempsens.h"
#include "benutzer.h"

//Fonts
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "./zLibraries/MCUFRIEND_kbv/FreeDefaultFonts.h"

// TFT-DISPLAY
/*
 * HEISSA HURRA! Nach fünftägiger Recherche festgestellt, dass es ganz einfach ist, wenn man es weiß.
 * Das 8-bit Interface sollte von den Pins D2-D9 auf Pins 22-29
 * Dazu in mcufiend_kbv/utility/mcufriend_shield.h:
 * #define USE_SPECIAL
 * und in mcufriend_special.h:
 * #define USE_MEGA_8BIT_PROTOSHIELD
 * protoshield deswegen, weil dann nur die Datenleitungen von 2-9 auf 22-29 flutschen. MEGA!
 * thanks to David Prentice and amikulics (arduino forum)
 * So schließt man das jetzt an:
 * ATA   MEGA                    LCD
 * 40....22.............................LCD_D0
 * 38....23.............................LCD_D1
 * 36....24.............................LCD_D2
 * 34....25.............................LCD_D3
 * 32....26.............................LCD_D4
 * 30....27.............................LCD_D5
 * 28....28.............................LCD_D6
 * 26....29.............................LCD_D7
 *
 * 04....A0.............................LCD_RD (read)
 * 06....A1.............................LCD_WR (write)
 * 08....A2.............................LCD_RS/DC Command Data
 * 10....A3.............................LCD_CS Chip Select
 * 12....A4.............................LCD_RST (reset)
 */
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define WGRUEN  0x05AA
#define ZGRUEN  0x06AB
#define GREY    0x8410

#ifndef min
  #define min(a, b) (((a) < (b)) ? (a) : (b))
  #endif

#define FONT "FreeSans12pt7b"
#define BOLD "FreeSansBold12pt7b"
#define NORMAL &FreeSans12pt7b
#define FETT &FreeSansBold12pt7b

// SHOW BMP
#define NAMEMATCH ""         // "" matches any name
#define PALETTEDEPTH   8     // support 256-colour Palette
#define BMPIMAGEOFFSET 54
#define BUFFPIXEL      20

class zDisplay : public MCUFRIEND_kbv, private GFXcanvas1
{
  public:
  zDisplay (); //Constructor
  virtual
  ~zDisplay (); //Destructor
  void
  beginn (SdFat *psd);
  void
  printText (void);  //Zeigt Text an
  uint8_t
  showBMP (char const *nm, int x, int y);
  void
  print_val (int val, int16_t x, int16_t y, int c, bool komma);
  //void fillScreen(unsigned short int color);
  void
  print_val2 (int val, int16_t x, int16_t y, int c, bool komma);
  void
  setCursor (int16_t x, int16_t y);
  void
  printInt (uint16_t zahl);
  void
  userShow (benutzer *user);
  void
  println (const char *text);
  void
  setTextColor (uint16_t c);
  void
  setTextSize (uint8_t s);
  void
  infoscreen (tempsens *temp, benutzer *user);
  MCUFRIEND_kbv _tft; //TFT Objekt zum aufrufen

private:
  //friend class GFXcanvas1;
  uint16_t
  read16 (File &f);
  uint32_t
  read32 (File &f);
  char namebuf[32] = "/";   //BMP files in PIC Verzeichnis
  File root;
  int pathlen = 0;
  //File bmpfile;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  GFXcanvas1 *_canvas;
  GFXcanvas1 myCanvas(uint16_t w, uint16_t h);


protected:
  SdFat *_sd; 		// Pointer zum SD-Objekt vom Hauptprogramm


//	File    _fd;      //SDFat file descriptor

};

#endif /* ZDISPLAY_H_ */
