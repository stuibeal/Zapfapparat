/*
 * zDisplay.cpp
 *
 *  Created on: 28.05.2023
 *      Author: alfred3
 *
 *      Hier ist das zDisplay Objekt für alle Dinge die
 *      die GUI so braucht
 */

#include "zDisplay.h"
#include "globalVariables.h"
#include "avr/pgmspace.h"
#include "Arduino.h"

zDisplay::zDisplay() :
		MCUFRIEND_kbv(0, 0, 0, 0, 0), U8G2_FOR_ADAFRUIT_GFX() {
	_sd = nullptr;
	r = 0;
	g = 0;
	b = 0;
	MCUFRIEND_kbv _tft;  //tft objekt
	U8G2_FOR_ADAFRUIT_GFX u8g2;

	_oldText1 = nullptr;
	_oldText2 = nullptr;
	infoWarteZeit = 0;
	infoGezeigt = 0;
}

zDisplay::~zDisplay() {
	// destructor
}

/*
 * Hauptprogramm übergibt Pointer zum SD Objekt
 */
void zDisplay::beginn(SdFat *psd) {
	_sd = psd; //speichert den Pointer
	_tft.begin(0x9486); //ID für ILI9486 Chipsatz
	_tft.setRotation(1);
	u8g2.begin(_tft);
	_tft.fillScreen(TFT_WHITE);
	u8g2.setForegroundColor(ZDUNKELGRUEN);
	u8g2.setBackgroundColor(WHITE);
	u8g2.setCursor(230, 25);
	u8g2.setFont(FONT_BOLD12);
	u8g2.println(F(_NAME_));
	u8g2.setFont(FONT_NORMAL10);
	printInitText(F(_VERSION_));

	//BMP SHOW
	strcpy(buf, "/");
	root = _sd->open(buf);
	pathlen = strlen(buf);

}
/**
 * @fn void printInitText(const char*)
 * @brief Schreibt beim "booten" Dinge ins Display
 *
 * @param text
 */
void zDisplay::printInitText(const __FlashStringHelper* text) {
	u8g2.setCursor(230, u8g2.getCursorY());
	u8g2.println(text);
}

void zDisplay::printInitText(const char *text) {
	u8g2.setCursor(230, u8g2.getCursorY());
	u8g2.println(text);
}

/**
 * @fn void infoText(const char*)
 * @brief Schreibt am unteren Displayrand eine Mitteilung linksbündig
 *
 * @param text
 */
void zDisplay::infoText(const char *text) {
	_tft.fillRect(0, 292, 480, 28, BLACK);
	u8g2.setCursor(10, 316);
	u8g2.setForegroundColor(WHITE);
	u8g2.setBackgroundColor(BLACK);
	u8g2.setFont(FONT_NORMAL12);
	u8g2.print(text);
	infoWarteZeit = millis();
	infoGezeigt= true;

}
void zDisplay::infoText(const __FlashStringHelper *text) {
	_tft.fillRect(0, 292, 480, 28, BLACK);
	u8g2.setCursor(10, 316);
	u8g2.setForegroundColor(WHITE);
	u8g2.setBackgroundColor(BLACK);
	u8g2.setFont(FONT_NORMAL12);
	u8g2.print(text);
	infoWarteZeit = millis();
	infoGezeigt= true;
}

void zDisplay::infoCheck() {
	if (infoGezeigt) {
		if (millis()- infoWarteZeit > 10000) {
			showTastenFunktion(nullptr, nullptr);
			infoWarteZeit = millis();
			infoGezeigt = false;
		}
	}
}

/**
 * @fn uint16_t read16(File&)
 * @brief Hilfsroutine für showBMP
 *
 * @param f
 * @return
 */
uint16_t zDisplay::read16(File &f) {
	uint16_t result;         // read little-endian
	f.read(&result, sizeof(result));
	return result;
}

/**
 * @fn uint32_t read32(File&)
 * @brief Hilfsroutine für showBMP
 *
 * @param f
 * @return
 */
uint32_t zDisplay::read32(File &f) {
	uint32_t result;
	f.read(&result, sizeof(result));
	return result;
}

/*!
 * @brief Liest BMPs von der SD Karte und gibt sie aus
 * @param nm	Filename
 * @param x	X-Position am Screen
 * @param y	Y-Position am Screen
 * @return
 */

uint8_t zDisplay::showBMP(const __FlashStringHelper *filename, int16_t x,
		int16_t y) {
	strcpy_P(buf, (const char*) filename);
	return showBMP(buf, x, y);
}

uint8_t zDisplay::showBMP(char const *nm, int16_t x, int16_t y) {
	File bmpFile;
	int bmpWidth, bmpHeight;    // W+H in pixels
	uint8_t bmpDepth;           // Bit depth (currently must be 24, 16, 8, 4, 1)
	uint32_t bmpImageoffset;    // Start of image data in file
	uint32_t rowSize;           // Not always = bmpWidth; may have padding
	uint8_t sdbuffer[3 * BUFFPIXEL];    // pixel in buffer (R+G+B per pixel)
	uint16_t lcdbuffer[(1 << PALETTEDEPTH) + BUFFPIXEL], *palette = NULL;
	uint8_t bitmask = 0xFF;
	uint8_t bitshift = 0;
	boolean flip = true;        // BMP is stored bottom-to-top
	int16_t w, h, row, col, lcdbufsiz = (1 << PALETTEDEPTH) + BUFFPIXEL;
	int16_t buffidx = 0; //war int
	uint32_t pos;               // seek position
	boolean is565 = false;      //

	uint16_t bmpID;
	uint16_t n;                 // blocks read
	uint8_t ret;

	if ((x >= _tft.width()) || (y >= _tft.height()))
		return 1;               // off screen

	bmpFile = _sd->open(nm);      // Parse BMP header
	bmpID = read16(bmpFile);    // BMP signature
	(void) read32(bmpFile);     // Read & ignore file size
	(void) read32(bmpFile);     // Read & ignore creator bytes
	bmpImageoffset = read32(bmpFile);       // Start of image data
	(void) read32(bmpFile);     // Read & ignore DIB header size
	bmpWidth = read32(bmpFile);
	bmpHeight = read32(bmpFile);
	n = read16(bmpFile);        // # planes -- must be '1'
	bmpDepth = read16(bmpFile); // bits per pixel
	pos = read32(bmpFile);      // format
	if (bmpID != 0x4D42)
		ret = 2; // bad ID
	else if (n != 1)
		ret = 3;   // too many planes
	else if (pos != 0 && pos != 3)
		ret = 4; // format: 0 = uncompressed, 3 = 565
	else if (bmpDepth < 16 && bmpDepth > PALETTEDEPTH)
		ret = 5; // palette
	else {
		bool first = true;
		is565 = (pos == 3);               // ?already in 16-bit format
		// BMP rows are padded (if needed) to 4-byte boundary
		rowSize = (bmpWidth * bmpDepth / 8 + 3) & ~3;
		if (bmpHeight < 0) {         // If negative, image is in top-down order.
			bmpHeight = -bmpHeight;
			flip = false;
		}

		w = bmpWidth;
		h = bmpHeight;
		if ((x + w) >= _tft.width())       // Crop area to be loaded
			w = _tft.width() - x;
		if ((y + h) >= _tft.height())      //
			h = _tft.height() - y;

		if (bmpDepth <= PALETTEDEPTH) {   // these modes have separate palette
										  //bmpFile.seek(BMPIMAGEOFFSET); //palette is always @ 54
			bmpFile.seek(bmpImageoffset - (4 << bmpDepth)); //54 for regular, diff for colorsimportant
			bitmask = 0xFF;
			if (bmpDepth < 8)
				bitmask >>= bmpDepth;
			bitshift = 8 - bmpDepth;
			n = 1 << bmpDepth;
			lcdbufsiz -= n;
			palette = lcdbuffer + lcdbufsiz;
			for (col = 0; col < n; col++) {
				pos = read32(bmpFile);    //map palette to 5-6-5
				palette[col] = ((pos & 0x0000F8) >> 3) | ((pos & 0x00FC00) >> 5)
						| ((pos & 0xF80000) >> 8);
			}
		}

		// Set TFT address window to clipped image bounds
		_tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
		for (row = 0; row < h; row++) { // For each scanline...
										// Seek to start of scan line.  It might seem labor-
										// intensive to be doing this on every line, but this
										// method covers a lot of gritty details like cropping
										// and scanline padding.  Also, the seek only takes
										// place if the file position actually needs to change
										// (avoids a lot of cluster math in SD library).
			uint8_t r = 0;
			uint8_t g = 0;
			uint8_t b = 0;
			int lcdidx, lcdleft;
			if (flip)   // Bitmap is stored bottom-to-top order (normal BMP)
				pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
			else
				// Bitmap is stored top-to-bottom
				pos = bmpImageoffset + row * rowSize;
			if (bmpFile.position() != pos) { // Need seek?
				bmpFile.seek(pos);
				buffidx = sizeof(sdbuffer); // Force buffer reload
			}

			for (col = 0; col < w;) {  //pixels in row
				lcdleft = w - col;
				if (lcdleft > lcdbufsiz)
					lcdleft = lcdbufsiz;
				for (lcdidx = 0; lcdidx < lcdleft; lcdidx++) { // buffer at a time
					uint16_t color = 0;
					// Time to read more pixel data?
					if (buffidx >= sizeof(sdbuffer)) { // Indeed
						bmpFile.read(sdbuffer, sizeof(sdbuffer));
						buffidx = 0; // Set index to beginning
						r = 0;
					}
					switch (bmpDepth) {  // Convert pixel from BMP to TFT format
					case 24:
						b = sdbuffer[buffidx++];
						g = sdbuffer[buffidx++];
						r = sdbuffer[buffidx++];
						color = _tft.color565(r, g, b);
						break;
					case 16:
						b = sdbuffer[buffidx++];
						r = sdbuffer[buffidx++];
						if (is565)
							color = (r << 8) | (b);
						else
							color = (r << 9) | ((b & 0xE0) << 1) | (b & 0x1F);
						break;
					case 1:
					case 4:
					case 8:
						if (r == 0) {
							b = sdbuffer[buffidx++], r = 8;
							color = palette[(b >> bitshift) & bitmask];
							r -= bmpDepth;
							b <<= bmpDepth;
						}
						break;
					}
					lcdbuffer[lcdidx] = color;

				}
				_tft.pushColors(lcdbuffer, lcdidx, first);
				first = false;
				col += lcdidx;
			}           // end cols
		}               // end rows
		_tft.setAddrWindow(0, 0, _tft.width() - 1, _tft.height() - 1); //restore full screen
		ret = 0;        // good render
	}
	bmpFile.close();
	return (ret);
}

/**
 * @fn void printValue(uint16_t, uint16_t, int, bool)
 * @brief schreibt ans Display, bei bool komma mit zwei
 * 		  Stellen nach dem Komma. Wert solle mal 100 sein
 * @param x
 * @param y
 * @param val
 * @param komma
 */
void zDisplay::printValue(uint16_t x, uint16_t y, int val, bool komma) {
	char buf[10];
	if (komma == 1) {
		sprintf(buf, "%2d,%02d ", val / 100, val % 100);
	} else {
		sprintf(buf, "%4d", val);
	}
	u8g2.drawStr(x, y, buf);
}

/**
 * @fn void print_val3(int, int16_t, int16_t, bool)
 * @brief Für das Anzeigen von Zahlen auf der Zapfsäule.
 *
 * @param val
 * @param x
 * @param y
 * @param komma
 */
void zDisplay::print_val3(int val, int16_t x, int16_t y, bool komma) //Hilfsroutine zum Daten anzeigen
		{
	u8g2.setForegroundColor(WHITE);      // apply Adafruit GFX color
	u8g2.setBackgroundColor(ZDUNKELGRUEN);
	u8g2.setFontMode(0);
	u8g2.setFont(FONT_ZAHLEN);
	char buf[10];
	if (komma == 1) {
		sprintf(buf, "%2d,%02d", val / 100, val % 100);
	} else {
		sprintf(buf, "%4d", val);
	}
	u8g2.drawStr(x, y, buf);
}

/**
 * Hilfsroutine um die Userdaten anzuzeigen. Sinnvoll nachdem
 * der User gewählt hat
 * @param user 	Pointer zum Benutzerobjekt
 */
void zDisplay::userShow() {
	static bool lastUserVoll = 0;

	if (user.getGodMode() > 0) {
		sprintf(buf, "/god/%d0.bmp", user.getGodMode());
	} else {
		sprintf(buf, "/usr/%d.bmp", user.aktuell);
	}
	showUserPic(buf);
	/*USERNAME*/
	_tft.fillRect(235, 10, 35, 271, ZBRAUN); /* vorsichtshalber ausbraunen */
	u8g2.setFont(FONT_BOLD19);
	u8g2.setFontDirection(3); /* font DOWN TO TOP */
	u8g2.setBackgroundColor(ZBRAUN);
	u8g2.setForegroundColor(ZDUNKELGRUEN);
	u8g2.setCursor(260, 280);
	u8g2.print(user.userN[user.aktuell]); //konvertiert den Pointer von userName in c-String
	u8g2.setFontDirection(0);
	/*VOREINSTELLUNG INFOANZEIGE*/
	u8g2.setFont(FONT_NORMAL19);
	_tft.fillRect(271, 146, 209, 140, ZBRAUN);
	u8g2.setFont(FONT_NORMAL10); /*10er font is 16 hoch*/
	u8g2.setForegroundColor(BLACK);
	u8g2.setBackgroundColor(ZBRAUN);
	uint16_t x = 271; /*da fängt der Rahmen an*/
	uint16_t y = 180; /*erste Zeile*/

	printSetCursor(x, y - 15, F("TEMPERATUR"));
	printSetCursor(x, y, F("SOLL IN °C"));
	y = 215;
	printSetCursor(x, y - 15, F("ZAPFMENGE"));
	printSetCursor(x, y, F("IN ML"));
	y = 250;
	printSetCursor(x, y - 15, F("HOIWE AM"));
	printSetCursor(x, y, F("HEUTIGEN TAG"));
	y = 285;
	printSetCursor(x, y - 15, F("REST IM"));
	printSetCursor(x, y, F("FASS IN L"));
	showAllUserData();

	/*Wenn über 8 Halbe soll der das volle Bild zeigen*/
	if (user.getBierTag() > 3999 && !lastUserVoll) {
		showBMP(F("/bmp/bg_voll.bmp"), 102, 0);
		lastUserVoll = 1;
	} else if (user.getBierTag() < 4000 && lastUserVoll) {
		showBMP(F("/bmp/bg_leer.bmp"), 102, 0); /* Bier leer, w:132 h:291 */
		lastUserVoll = 0;
	}
}

/**
 * @fn void infoscreen(tempControl*, benutzer*)
 * @brief Zeigt alle möglichen Daten an.
 *
 * @param temp
 * @param user
 */
void zDisplay::infoscreen() {
	temp.requestSensors();
	_tft.fillScreen(BLACK);
	_tft.fillRect(0, 0, 480, 27, ZDUNKELGRUEN);
	u8g2.setFontMode(0);                 // use u8g2 none transparent mode
	u8g2.setFontDirection(0);            // left to right (this is default)
	u8g2.setForegroundColor(WHITE);      // apply Adafruit GFX color
	u8g2.setBackgroundColor(ZDUNKELGRUEN);
	u8g2.setFont(FONT_BOLD19); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
	u8g2.setCursor(5, 20);                // start writing at this position
	u8g2.print(F(_NAME_));
	u8g2.setCursor(240, 20);                // start writing at this position
	u8g2.setFontMode(0);                 // use u8g2 none transparent mode
	u8g2.print(F("Information"));       // UTF-8 string with german umlaut chars
	u8g2.setBackgroundColor(BLACK);
	u8g2.setCursor(240, 60);
	u8g2.setFont(FONT_NORMAL12);
	uint16_t y = 5;
	for (int x = 0; x < 20; x++) {
		if (x > 9) {
			y = 245;
		}
		if (x == 10) {
			u8g2.setCursor(240, 60);
		}
		u8g2.setFont(FONT_BOLD12);
		u8g2.setCursor(y, u8g2.getCursorY());
		u8g2.print(x);
		u8g2.setFont(FONT_NORMAL12);
		u8g2.setCursor(y + 35, u8g2.getCursorY());
		u8g2.print(user.userN[x]);
		sprintf(buf, "%u ml", user.bierTag[x]);
		u8g2.setCursor(y + 225 - u8g2.getUTF8Width(buf), u8g2.getCursorY());
		u8g2.println(buf);
	}
	temp.holeDaten();
	u8g2.setCursor(0, 230);
	u8g2.setFont(FONT_BOLD12);
	u8g2.println(_NAME_);
	u8g2.setFont(FONT_NORMAL12);
	u8g2.println(_VERSION_);
	u8g2.println();
	u8g2.setFont(FONT_NORMAL10);
	u8g2.setCursor(0, 270);
	printlnInfoTemp(230, 0, F("Block DS18B20: "), temp.getDSblockTemp());
	printlnInfoTemp(230, 0, F("Block PT100 Innen: "), temp.getBlockInnenTemp());
	printlnInfoTemp(230, 0, F("Zapfhahn: "), temp.getHahnTemp());
	u8g2.setCursor(250, 270);
	printlnInfoTemp(470, 250, F("Getränkezulauf: "), temp.getZulaufTemp());
	printlnInfoTemp(470, 250, F("Kühlwasser: "), temp.getKuehlWasserTemp());
	printlnInfoTemp(470, 250, F("Gehäuse: "), temp.getHausTemp());
	u8g2.print(F("Helligkeit: "));
	u8g2.print(power.getHelligkeit());
	u8g2.setCursor(250, u8g2.getCursorY());
	u8g2.print(F("Kühlwasserflow: "));
	u8g2.print(temp.getKuehlWasserFlowRate());
}

void zDisplay::printSetCursor(uint16_t x, uint16_t y,
		const __FlashStringHelper* text) {
	u8g2.setCursor(x, y);
	u8g2.print(text);
}

void zDisplay::printlnInfoTemp(uint16_t right_x, uint16_t left_x,
		const __FlashStringHelper* text, int16_t temp) {
	u8g2.setCursor(left_x, u8g2.getCursorY());
	u8g2.print(text);
	sprintf(buf, "%2d,%02d°C", temp / 100, temp % 100);
	u8g2.setCursor(right_x - u8g2.getUTF8Width(buf), u8g2.getCursorY());
	u8g2.println(buf);
}
/**
 * @fn void backgroundPicture()
 * @brief Grundlegendes Hintergrundbild mit Beschriftung
 *
 */
void zDisplay::backgroundPicture() {
	_tft.fillScreen(ZBRAUN);
	_tft.fillRect(0, 293, 480, 28, BLACK);
	showBMP(F("/bmp/bg_zapf.bmp"), 0, 0); /* Zapfsäule, w:102 h:291 */
	showBMP(F("/bmp/bg_leer.bmp"), 102, 0); /* Bier leer, w:132 h:291 */
	showBMP(F("/bmp/bg_rahm.bmp"), 271, 8); /* Userbildrahmen, w:199 h:136 */
	/* Userbild 160x100 geht nach x290 y26 */
	u8g2.setFont(FONT_SMALL);
	u8g2.setForegroundColor(WHITE);
	u8g2.setBackgroundColor(ZDUNKELGRUEN);
	u8g2.setFontMode(0);
	printSetCursor(17, 40, F("KÜHLBLOCK"));
	printSetCursor(17, 52, F("TEMPERATUR"));
	printSetCursor(75, 97, F("°C"));
	printSetCursor(17, 140, F("ZAPFMENGE"));
	printSetCursor(75, 185, F("ml"));
	printSetCursor(17, 230, F("DRUCK"));
	printSetCursor(75, 275, F("atü"));
}

/**
 * @fn void showUserPic(const char*)
 * @brief Zeigt das Userbild an. Krass.
 *
 * @param bmp
 */
void zDisplay::showUserPic(const char *bmp) {
	showBMP(bmp, 290, 26);
}

/**
 * @fn void showUserGod2Pic(void)
 * @brief Sollte der User im Godmode sein wird nach dem Zapfen
 * 		  ein anderes Bild angezeigt. Auch krass.
 *
 */
void zDisplay::showUserGod2Pic(void) {
	if (user.getGodMode() > 0) {
		sprintf(buf, "/god/%d1.bmp", user.getGodMode());
		showUserPic(buf);
	}
}

/**
 * @fn void showSingleUserData(uint8_t)
 * @brief Zeigt eine einzelne Zeile der Userdaten an der richtigen stelle an.
 *
 * @param whatLine
 */
void zDisplay::showSingleUserData(uint8_t whatLine) {
	u8g2.setFont(FONT_ZAHLEN);
	if (einsteller == whatLine) {
		u8g2.setForegroundColor(BLACK);
	} else {
		u8g2.setForegroundColor(ZDUNKELGRUEN);
	}
	uint16_t zeilenAbstand = 35;
	uint16_t cursX = 385;
	uint16_t cursY = 177 + ((whatLine - 1) * zeilenAbstand);
	u8g2.setBackgroundColor(ZBRAUN);

	switch (whatLine) {
	case 0:
		break;
	case 1:
		printValue(cursX, cursY, user.getSollTemperatur(), KOMMA);
		break;
	case 2:
		printValue(cursX, cursY, user.getMenge(), GANZZAHL);
		break;
	case 3:
		printValue(cursX, cursY, user.getBierTag() / 5, KOMMA);
		break;
	case 4:
		printValue(cursX, cursY, user.getRestMengeFass() / 10, KOMMA);
		break;
	}
}

/**
 * @fn void showAllUserData()
 * @brief Zeigt alle Userdaten an
 *
 */
void zDisplay::showAllUserData() {
	for (uint8_t i = 1; i < 5; i++) {
		showSingleUserData(i);
	}
}

/**
 * @fn void showBalken(uint16_t, uint16_t)
 * @brief für den Godmode eine Balkenanzeige zum Zapfvorgang
 *
 * @param istwert
 * @param zielwert
 */
void zDisplay::showBalken(uint16_t istwert, uint16_t zielwert) {
	long int aktuelleBreite = 0;
	aktuelleBreite = map((long int) istwert, 0, (long int) user.getMenge(), 0,
			480);
	if (aktuelleBreite > 0) {
		_tft.fillRect(0, 292, aktuelleBreite, 4, ZDUNKELGRUEN);
	}
}

/**
 * @fn void showTastenFunktion(const char*, const char*)
 * @brief Zeigt die aktuellen Tastenfunktionen an, links und rechtsbündig
 *
 * @param textTaste1
 * @param textTaste2
 */
void zDisplay::showTastenFunktion(const char *textTaste1,
		const char *textTaste2) {
	if (textTaste1 == nullptr) {
		textTaste1 = _oldText1;
	}
	if (textTaste2 == nullptr) {
		textTaste2 = _oldText1;
	}
	_oldText1 = textTaste1;
	_oldText2 = textTaste2;
	_tft.fillRect(0, 292, 480, 28, BLACK);
	u8g2.setCursor(10, 316);
	u8g2.setForegroundColor(WHITE);
	u8g2.setBackgroundColor(BLACK);
	u8g2.setFont(FONT_NORMAL12);
	u8g2.print(textTaste1);
	uint16_t stringWeite = u8g2.getUTF8Width(textTaste2);
	u8g2.setCursor(470 - stringWeite, 318);
	u8g2.print(textTaste2);
}

void zDisplay::printProgrammInfo(const __FlashStringHelper* textUeberschrift) {
	_tft.fillRect(271, 146, 209, 140, ZBRAUN);
	u8g2.setFont(FONT_BOLD12);
	u8g2.setFontMode(1); //transparent
	u8g2.setForegroundColor(BLACK);
	u8g2.setFontDirection(0);
	uint16_t x = 271; /*da fängt der Rahmen an*/
	uint16_t y = 163; /*erste Zeile*/
	_tft.drawFastHLine(x + 1, y + 4, 199, BLACK);
	_tft.drawFastHLine(x, y + 3, 199, WHITE);
	u8g2.setCursor(x + 1, y + 1);
	u8g2.print(textUeberschrift);
	u8g2.setForegroundColor(ZDUNKELGRUEN);
	u8g2.setCursor(x, y);
	u8g2.print(textUeberschrift);
	u8g2.setFontMode(0);
}

void zDisplay::printProgrammInfo(const char *textUeberschrift) {
	_tft.fillRect(271, 146, 209, 140, ZBRAUN);
	u8g2.setFont(FONT_BOLD12);
	u8g2.setFontMode(1); //transparent
	u8g2.setForegroundColor(BLACK);
	u8g2.setFontDirection(0);
	uint16_t x = 271; /*da fängt der Rahmen an*/
	uint16_t y = 163; /*erste Zeile*/
	_tft.drawFastHLine(x + 1, y + 4, 199, BLACK);
	_tft.drawFastHLine(x, y + 3, 199, WHITE);
	u8g2.setCursor(x + 1, y + 1);
	u8g2.print(textUeberschrift);
	u8g2.setForegroundColor(ZDUNKELGRUEN);
	u8g2.setCursor(x, y);
	u8g2.print(textUeberschrift);
	u8g2.setFontMode(0);
}

void zDisplay::printProgrammInfoZeilen(uint8_t zeile, uint8_t spalte,
		const char *textZeile) {
	u8g2.setFont(FONT_NORMAL12); /*10er font is 16 hoch*/
	uint16_t zA = 16; /*Zeilenabstand*/
	uint16_t x = 271; /*da fängt der Rahmen an*/
	uint16_t y = 170 + (zeile * zA);
	switch (spalte) {
	case 1:
		break;
	case 2:
		x = 370;
	}
	u8g2.setForegroundColor(BLACK);
	u8g2.setBackgroundColor(ZBRAUN);
	u8g2.drawUTF8(x, y, textZeile);
}

void zDisplay::printProgrammInfoZeilen(uint8_t zeile, uint8_t spalte,
		const __FlashStringHelper* textZeile) {
	u8g2.setFont(FONT_NORMAL12); /*10er font is 16 hoch*/
	uint16_t zA = 16; /*Zeilenabstand*/
	uint16_t x = 271; /*da fängt der Rahmen an*/
	uint16_t y = 170 + (zeile * zA);
	switch (spalte) {
	case 1:
		break;
	case 2:
		x = 370;
	}
	u8g2.setForegroundColor(BLACK);
	u8g2.setBackgroundColor(ZBRAUN);
	u8g2.setCursor(x, y);
	u8g2.print(textZeile);
}
