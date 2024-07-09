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

zDisplay::zDisplay() :
		MCUFRIEND_kbv(0, 0, 0, 0, 0), U8G2_FOR_ADAFRUIT_GFX() {
	_sd = nullptr;
	r = 0;
	g = 0;
	b = 0;
	strcpy(namebuf, "/");
	MCUFRIEND_kbv _tft;  //tft objekt
	U8G2_FOR_ADAFRUIT_GFX u8g2;
	//myCanvas = new
	//_infoCanvas = new GFXcanvas1(200, 160);

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
	u8g2.println(_NAME_);
	u8g2.setFont(FONT_NORMAL10);
	printInitText(_VERSION_);

	//BMP SHOW
	root = _sd->open(namebuf);
	pathlen = strlen(namebuf);

}
void zDisplay::printInitText(const char *text) {
	u8g2.setCursor(230, u8g2.getCursorY());
	u8g2.println(text);
}

void zDisplay::printText(void) {
	_tft.setFont(0);
	_tft.setTextSize(2);
	_tft.setCursor(10, 305);
	_tft.setTextColor(TFT_BLACK, TFT_WHITE);
	_tft.println("                                ");
	_tft.setCursor(10, 305);
}

void zDisplay::infoText(const char *text) {
	_tft.fillRect(0, 292, 480, 28, BLACK);
	u8g2.setCursor(10, 318);
	u8g2.setForegroundColor(WHITE);
	u8g2.setBackgroundColor(BLACK);
	u8g2.setFont(FONT_NORMAL12);
	u8g2.print(text);
}

uint16_t zDisplay::read16(File &f) {
	uint16_t result;         // read little-endian
	f.read(&result, sizeof(result));
	return result;
}

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
uint8_t zDisplay::showBMP(char const *nm, int16_t x, int16_t y) {
	File bmpFile;
	int bmpWidth, bmpHeight;    // W+H in pixels
	uint8_t bmpDepth;           // Bit depth (currently must be 24, 16, 8, 4, 1)
	uint32_t bmpImageoffset;    // Start of image data in file
	uint32_t rowSize;           // Not always = bmpWidth; may have padding
	uint8_t sdbuffer[3 * BUFFPIXEL];    // pixel in buffer (R+G+B per pixel)
	uint16_t lcdbuffer[(1 << PALETTEDEPTH) + BUFFPIXEL], *palette = NULL;
	uint8_t bitmask, bitshift;
	boolean flip = true;        // BMP is stored bottom-to-top
	int16_t w, h, row, col, lcdbufsiz = (1 << PALETTEDEPTH) + BUFFPIXEL,
			buffidx; //war int
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
			uint8_t r;
			uint8_t g;
			uint8_t b;
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
					uint16_t color;
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

void zDisplay::print_val(int val, int16_t x, int16_t y, int c, bool komma) //Hilfsroutine zum Daten anzeigen
		{
	char buf[10];
	int16_t x1, y1;
	uint16_t w, h;
	_tft.setTextSize(1);
	_tft.setFont(&FreeSans12pt7b);

	_tft.getTextBounds("00,00", x, y, &x1, &y1, &w, &h);
	float val2;
	if (komma == 1) {
		val2 = val % 100;
		val2 = val2 / 100;
		val2 = val2 + (val / 100);
		dtostrf(val2, 5, 2, buf);   //e.g. 12.34
	} else {
		val2 = val;
		dtostrf(val2, 4, 0, buf);   //e.g. 1234
	}

	_tft.fillRect(x1, y1 - 2, w + 3, h, ZGRUEN);

	if (c == 1) {
		_tft.setTextColor(BLACK);
		_tft.setFont(&FreeSansBold12pt7b);
	} else {
		_tft.setTextColor(WHITE);
	}
	if (c == 2) {
		_tft.setTextColor(RED);
	}
	_tft.setCursor(x, y);
	_tft.print(buf);
}

void zDisplay::printValue(int val, bool komma) {
	char buf[10];
	if (komma == 1) {
		sprintf(buf, " %d,%02d ", val / 100, val % 100);
	} else {
		sprintf(buf, " %d ", val);
	}
	u8g2.print(buf);
}

void zDisplay::print_val3(int val, int16_t x, int16_t y, bool komma) //Hilfsroutine zum Daten anzeigen
		{
	u8g2.setForegroundColor(WHITE);      // apply Adafruit GFX color
	u8g2.setBackgroundColor(ZDUNKELGRUEN);
	u8g2.setFontMode(0);
	u8g2.setFont(u8g2_font_t0_22b_mn);
	char buf[10];
	if (komma == 1) {
		sprintf(buf, "%d,%02d" , val / 100, val % 100);
	} else {
		sprintf(buf, "%d", val);
	}
	//u8g2.print(buf);
	u8g2.drawStr(x, y, buf);

}

/**
 * Schreibt einen INT in einen Canvas und Zeigt ihn an
 * @param val 	der anzuzeigende Wert
 * @param x	X Position des Canvas am Screen
 * @param y	Y Position des Canvas am Screen
 * @param textColor	Textfarbe
 * @param backColor	Hintergrundfarbe
 * @param _pfont	Pointer zur Schriftart
 * @param komma		Darstellung mit oder ohne Komma (def KOMMA/GANZZAHL)
 */
//void zDisplay::printVal(int val, int16_t x, int16_t y, uint16_t textColor,
//		uint16_t backColor, const GFXfont *_pfont, bool komma) //Hilfsroutine zum Daten anzeigen
//		{
//	char buf[10];
//	_canvas->setTextSize(1);
//	_canvas->fillScreen(0);
//	_canvas->setCursor(0, 20);  //Text Cursor bei Fonts ist unten
//
//	if (komma) {
//		sprintf(buf, "%d,%02d", val / 100, val % 100);
//	} else {
//		sprintf(buf, "%d", val);
//	}
//
//	_canvas->setFont(_pfont);
//	_canvas->print(buf);
//	_tft.drawBitmap(x, y, _canvas->getBuffer(), _canvas->width(),
//			_canvas->height(), textColor, backColor);
//}

void zDisplay::printInt(uint16_t wertInt) {
	int x = 300;
	int y = 200;

	_tft.setCursor(x, y);

	char wert[80] = "0000";
	sprintf(wert, "Honk: %4u %x", wertInt, wertInt);
	_tft.println(wert);

}
/**
 * Hilfsroutine um die Userdaten anzuzeigen. Sinnvoll nachdem
 * der User gewählt hat
 * @param user 	Pointer zum Benutzerobjekt
 */
void zDisplay::userShow(benutzer *user) {
	char namebuf[32] = "/usr/x.bmp";

	if (user->getGodMode() > 0) {
		sprintf(namebuf, "/god/%d0.bmp", user->getGodMode());
	}
	else{
		sprintf(namebuf, "/usr/%d.bmp", user->aktuell);
	}
	showUserPic(namebuf);
	/*USERNAME*/
	_tft.fillRect(235, 10, 35, 271, ZBRAUN); /* vorsichtshalber ausbraunen */
	u8g2.setFont(FONT_BOLD19);
	u8g2.setFontDirection(3); /* font DOWN TO TOP */
	u8g2.setBackgroundColor(ZBRAUN);
	u8g2.setForegroundColor(ZDUNKELGRUEN);
	u8g2.setCursor(260, 280);
	u8g2.print(user->getName()); //konvertiert den Pointer von userName in c-String
	u8g2.setFontDirection(0);
	/*VOREINSTELLUNG INFOANZEIGE*/
	u8g2.setFont(FONT_NORMAL19);
	_tft.fillRect(271, 146, 209, 140, ZBRAUN);
	u8g2.setFont(FONT_NORMAL10); /*10er font is 16 hoch*/
	u8g2.setForegroundColor(BLACK);
	u8g2.setBackgroundColor(ZBRAUN);
	u8g2.setCursor(271, 161); /* In Linie mit dem Rahmen */
	u8g2.println("TEMPERATUR");
	u8g2.setCursor(271, u8g2.getCursorY());
	u8g2.print("in °C");
	u8g2.setCursor(271, 195);
	u8g2.println("ZAPFMENGE");
	u8g2.setCursor(271, u8g2.getCursorY());
	u8g2.print("in ml");
	u8g2.setCursor(271, 229);
	u8g2.println("HOIWE");
	u8g2.setCursor(271, u8g2.getCursorY());
	u8g2.print("BIS JETZT");
	u8g2.setCursor(271, 263);
	u8g2.println("REST IM FASS");
	u8g2.setCursor(271, u8g2.getCursorY());
	u8g2.print("in Liter");

	showAllUserData();
}

void zDisplay::infoscreen(tempControl *temp, benutzer *user) {
	temp->requestSensors();
	temp->holeDaten();
	_tft.fillScreen(BLACK);
	_tft.fillRect(0, 0, 480, 27, ZDUNKELGRUEN);
	u8g2.setFontMode(0);                 // use u8g2 none transparent mode
	u8g2.setFontDirection(0);            // left to right (this is default)
	u8g2.setForegroundColor(WHITE);      // apply Adafruit GFX color
	u8g2.setBackgroundColor(ZDUNKELGRUEN);
	u8g2.setFont(FONT_BOLD19); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
	u8g2.setCursor(5, 20);                // start writing at this position
	u8g2.print(_NAME_);
	u8g2.setCursor(240, 20);                // start writing at this position
	u8g2.setFontMode(0);                 // use u8g2 none transparent mode
	u8g2.print("Information");          // UTF-8 string with german umlaut chars
	u8g2.setBackgroundColor(BLACK);
	u8g2.setCursor(240, 60);
	u8g2.setFont(FONT_NORMAL12);

	for (int x = 0; x < 11; x++) {
		u8g2.setCursor(240, u8g2.getCursorY());
		u8g2.print(x);
		u8g2.setCursor(260, u8g2.getCursorY());
		u8g2.print(user->username[x]);
		u8g2.setCursor(380, u8g2.getCursorY());
		u8g2.print(user->bierTag[x]);
		u8g2.println(" ml");
	}
	u8g2.setCursor(5, 60);
	u8g2.setFont(FONT_BOLD12);
	u8g2.println("ZAPFZYSTEM");
	u8g2.setFont(FONT_NORMAL12);
	u8g2.println(_VERSION_);
	u8g2.println();
	printlnTempC("Block DS18B20:", temp->getDSblockTemp());
	printlnTempC("Block außen:", temp->getBlockAussenTemp());
	printlnTempC("Block innen:", temp->getBlockInnenTemp());
	printlnTempC("Zapfhahn:", temp->getHahnTemp());
	printlnTempC("Gehäuse:", temp->getHausTemp());
	printlnTempC("Zulauf:", temp->getZulaufTemp());
	printlnTempC("Kühlwasser:", temp->getKuehlWasserTemp());

	temp->requestSensors();
}

void zDisplay::printlnTempC(const char *text, int16_t tempInC) {
	u8g2.print(text);
	u8g2.setCursor(160, u8g2.getCursorY());
	printValue(tempInC, KOMMA);
	u8g2.println("°C");
}

void zDisplay::backgroundPicture() {
	_tft.fillScreen(ZBRAUN);
	_tft.fillRect(0, 293, 480, 28, BLACK);
	showBMP("/bmp/bg_zapf.bmp", 0, 0); /* Zapfsäule, w:102 h:291 */
	showBMP("/bmp/bg_leer.bmp", 102, 0); /* Bier leer, w:132 h:291 */
	showBMP("/bmp/bg_rahm.bmp", 271, 8); /* Userbildrahmen, w:199 h:136 */
	/* Userbild 160x100 geht nach x290 y26 */
	u8g2.setFont(FONT_NORMAL10);
	u8g2.setForegroundColor(WHITE);
	u8g2.setBackgroundColor(ZDUNKELGRUEN);
	u8g2.setFontMode(0);
	u8g2.drawUTF8(17, 40, "KÜHLBLOCK");
	u8g2.drawUTF8(17, 55, "TEMPERATUR");
	u8g2.drawUTF8(70, 110, "°C");
	u8g2.drawUTF8(17, 140, "ZAPFMENGE");
	u8g2.drawUTF8(70, 205, "ml");
	u8g2.drawUTF8(17, 225, "DRUCK");
	u8g2.drawUTF8(70, 290, "atü");


}

void zDisplay::showUserPic(const char *bmp) {
	showBMP(bmp, 290, 26);
}

void zDisplay::showUserGod2Pic(void) {
	if (user.getGodMode() > 0) {
		char namebuf[30] = "/god/10.bmp";
		sprintf(namebuf, "/god/%d1.bmp", user.getGodMode());
		showUserPic(namebuf);
	}
}

void zDisplay::showSingleUserData(uint8_t whatLine) {
	if (einsteller == whatLine) {
		u8g2.setFont(FONT_BOLD19);
		u8g2.setForegroundColor(BLACK);
	} else {
		u8g2.setFont(FONT_NORMAL19);
		u8g2.setForegroundColor(ZDUNKELGRUEN);
	}
	uint8_t zeilenAbstand = 34;
	uint16_t cursX = 385;
	uint16_t cursY = 161 + 16 + (whatLine - 1) * zeilenAbstand;
	u8g2.setCursor(cursX, cursY);
	u8g2.setBackgroundColor(ZBRAUN);

	switch (whatLine) {
	case 0:
		break;
	case 1:
		printValue(user.bierTemp[user.aktuell], KOMMA);
		break;
	case 2:
		printValue(user.bierMenge[user.aktuell], GANZZAHL);
		break;
	case 3:
		printValue(user.bierTag[user.aktuell] / 500, KOMMA);
		break;
	case 4:
		printValue(user.restMengeFass / 1000, KOMMA);
		break;
	}
}

void zDisplay::showAllUserData() {
	for (uint8_t i = 1; i < 5; i++) {
		showSingleUserData(i);
	}
}

void zDisplay::showBalken(uint16_t istwert, uint16_t zielwert) {
	uint16_t aktuelleBreite = 0;
	map(aktuelleBreite, 0, zielwert, 0, 480);
	if (aktuelleBreite > 0){
		_tft.fillRect(0, 315, aktuelleBreite, 5, ZDUNKELGRUEN);
	}
}

void zDisplay::showTastenFunktion(const char* textTaste1, const char* textTaste2) {
	_tft.fillRect(0, 292, 480, 28, BLACK);
	u8g2.setCursor(10, 318);
	u8g2.setForegroundColor(WHITE);
	u8g2.setBackgroundColor(BLACK);
	u8g2.setFont(FONT_NORMAL12);
	u8g2.print(textTaste1);
	u8g2.setCursor(250, 318);
	u8g2.print(textTaste2);
}
