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

#include <Arduino.h>
#include "SdFat.h"
#include "gemein.h"
#include "globalVariables.h"
#include "stdio.h"
#include <string.h>
#include "./zLibraries/MCUFRIEND_kbv/MCUFRIEND_kbv.h"
#include "Adafruit_GFX.h"
#include "U8g2_for_Adafruit_GFX.h"
#include "tempControl.h"
#include "benutzer.h"

//#include "Adafruit_GFX.h" // Hardware-specific library
#define FONT_SMALL u8g2_font_t0_11_te
#define FONT_NORMAL10 u8g2_font_luRS10_tf
#define FONT_BOLD10 u8g2_font_luBS10_tf
#define FONT_NORMAL12 u8g2_font_luRS12_tf
#define FONT_BOLD12 u8g2_font_luBS12_tf
#define FONT_NORMAL19 u8g2_font_luRS19_tf
#define FONT_BOLD19 u8g2_font_luBS19_tf
#define FONT_ZAHLEN u8g2_font_inb19_mn


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


#ifndef min
  #define min(a, b) (((a) < (b)) ? (a) : (b))
  #endif

// SHOW BMP
#define NAMEMATCH ""         // "" matches any name
#define PALETTEDEPTH   8     // support 256-colour Palette
#define BMPIMAGEOFFSET 54
#define BUFFPIXEL      20

class zDisplay: private MCUFRIEND_kbv, U8G2_FOR_ADAFRUIT_GFX {
public:
	zDisplay(); //Constructor
	virtual ~zDisplay(); //Destructor
	void beginn(SdFat *psd);
	void printInitText(const char *text);
	void printInitText(const __FlashStringHelper* text);
	void infoText(const char *text);
	void infoCheck(void);
	void infoText(const __FlashStringHelper* text);
	uint8_t showBMP(const __FlashStringHelper *filename, int16_t x, int16_t y);
	uint8_t showBMP(char const *nm, int16_t x, int16_t y);
	void print_val(int val, int16_t x, int16_t y, int c, bool komma);
	void print_val3(int val, int16_t x, int16_t y, bool komma);
	void userShow();
	void infoscreen();
	void printValue(uint16_t x, uint16_t y, int val, bool komma);
	void printSetCursor(uint16_t x, uint16_t y, const __FlashStringHelper* text);
	void printlnInfoTemp(uint16_t right_x, uint16_t left_x, const __FlashStringHelper* text, int16_t temp);
	void backGroundUserData();
	void backgroundPicture(void);
	void showUserPic(const char *bmp);
	void showUserGod2Pic(void);
	void showSingleUserData(uint8_t whatLine);
	void showAllUserData(void);
	void showBalken(uint16_t istwert, uint16_t zielwert);
	void showTastenFunktion(const char* textTaste1, const char* textTaste2);
	void printProgrammInfo(const __FlashStringHelper *textUeberschrift);
	void printProgrammInfo(const char* textUeberschrift);
	void printProgrammInfoZeilen(uint8_t zeile, uint8_t spalte, const __FlashStringHelper* textZeile);
	void printProgrammInfoZeilen(uint8_t zeile, uint8_t spalte, const char* textZeile);

private:
	MCUFRIEND_kbv _tft; //TFT Objekt zum aufrufen
	U8G2_FOR_ADAFRUIT_GFX u8g2;
	uint16_t read16(File &f);
	uint32_t read32(File &f);

	File root;
	int pathlen = 0;
	//File bmpfile;
	uint8_t r;
	uint8_t g;
	uint8_t b;
	const char *_oldText1;
	const char *_oldText2;
	uint32_t infoWarteZeit;
	bool infoGezeigt;

protected:
	SdFat *_sd; 		// Pointer zum SD-Objekt vom Hauptprogramm

//	File    _fd;      //SDFat file descriptor

};

#endif /* ZDISPLAY_H_ */
