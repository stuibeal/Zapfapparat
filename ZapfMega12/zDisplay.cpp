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

zDisplay::zDisplay () :
    MCUFRIEND_kbv (0, 0, 0, 0, 0), GFXcanvas1 (60,22)

//, GFXcanvas1(60,22)

{
  _sd = nullptr;
  _canvas = nullptr;
  r = 0;
  g = 0;
  b = 0;
  strcpy (namebuf, "/");
  MCUFRIEND_kbv _tft;  //tft objekt
  //myCanvas = new
  _canvas = new GFXcanvas1 (60, 21);

}

zDisplay::~zDisplay ()
{
  // TODO Auto-generated destructor stub
}

/*
 * Hauptprogramm übergibt Pointer zum SD Objekt
 */
void
zDisplay::beginn (SdFat *psd)
{
  _sd = psd; //speichert den Pointer

  _tft.begin (0x9486); //ID für ILI9486 Chipsatz
  _tft.setRotation (1);
  _tft.setTextSize (2);
  _tft.fillScreen (TFT_WHITE);
  _tft.setTextColor (ZGRUEN, WHITE);

  _tft.setCursor (10, 10); //Cursor setzen

  _tft.println ("Z-Apfapparat");
  _tft.println ("Version 0.5 BETA 2022/2023"); //Bootausgabe

  _canvas->setTextWrap (false);

  //BMP SHOW
  root = _sd->open (namebuf);
  pathlen = strlen (namebuf);

}

void
zDisplay::printText (void)
{
  _tft.setFont (0);
  _tft.setTextSize (2);
  _tft.setCursor (10, 305);
  _tft.setTextColor (TFT_BLACK, TFT_WHITE);
  _tft.println ("                         ");
  _tft.setCursor (10, 305);

}

uint16_t
zDisplay::read16 (File &f)
{
  uint16_t result;         // read little-endian
  f.read (&result, sizeof(result));
  return result;
}

uint32_t
zDisplay::read32 (File &f)
{
  uint32_t result;
  f.read (&result, sizeof(result));
  return result;
}

/*!
 * @brief Liest BMPs von der SD Karte und gibt sie aus
 * @param nm
 * @param x
 * @param y
 * @return
 */
uint8_t
zDisplay::showBMP (char const *nm, int x, int y)
{
  File bmpFile;
  int bmpWidth, bmpHeight;    // W+H in pixels
  uint8_t bmpDepth;           // Bit depth (currently must be 24, 16, 8, 4, 1)
  uint32_t bmpImageoffset;    // Start of image data in file
  uint32_t rowSize;           // Not always = bmpWidth; may have padding
  uint8_t sdbuffer[3 * BUFFPIXEL];    // pixel in buffer (R+G+B per pixel)
  uint16_t lcdbuffer[(1 << PALETTEDEPTH) + BUFFPIXEL], *palette = NULL;
  uint8_t bitmask, bitshift;
  boolean flip = true;        // BMP is stored bottom-to-top
  int w, h, row, col, lcdbufsiz = (1 << PALETTEDEPTH) + BUFFPIXEL, buffidx;
  uint32_t pos;               // seek position
  boolean is565 = false;      //

  uint16_t bmpID;
  uint16_t n;                 // blocks read
  uint8_t ret;

  if ((x >= _tft.width ()) || (y >= _tft.height ()))
    return 1;               // off screen

  bmpFile = _sd->open (nm);      // Parse BMP header
  bmpID = read16 (bmpFile);    // BMP signature
  (void) read32 (bmpFile);     // Read & ignore file size
  (void) read32 (bmpFile);     // Read & ignore creator bytes
  bmpImageoffset = read32 (bmpFile);       // Start of image data
  (void) read32 (bmpFile);     // Read & ignore DIB header size
  bmpWidth = read32 (bmpFile);
  bmpHeight = read32 (bmpFile);
  n = read16 (bmpFile);        // # planes -- must be '1'
  bmpDepth = read16 (bmpFile); // bits per pixel
  pos = read32 (bmpFile);      // format
  if (bmpID != 0x4D42)
    ret = 2; // bad ID
  else if (n != 1)
    ret = 3;   // too many planes
  else if (pos != 0 && pos != 3)
    ret = 4; // format: 0 = uncompressed, 3 = 565
  else if (bmpDepth < 16 && bmpDepth > PALETTEDEPTH)
    ret = 5; // palette
  else
    {
      bool first = true;
      is565 = (pos == 3);               // ?already in 16-bit format
      // BMP rows are padded (if needed) to 4-byte boundary
      rowSize = (bmpWidth * bmpDepth / 8 + 3) & ~3;
      if (bmpHeight < 0)
	{         // If negative, image is in top-down order.
	  bmpHeight = -bmpHeight;
	  flip = false;
	}

      w = bmpWidth;
      h = bmpHeight;
      if ((x + w) >= _tft.width ())       // Crop area to be loaded
	w = _tft.width () - x;
      if ((y + h) >= _tft.height ())      //
	h = _tft.height () - y;

      if (bmpDepth <= PALETTEDEPTH)
	{   // these modes have separate palette
	    //bmpFile.seek(BMPIMAGEOFFSET); //palette is always @ 54
	  bmpFile.seek (bmpImageoffset - (4 << bmpDepth)); //54 for regular, diff for colorsimportant
	  bitmask = 0xFF;
	  if (bmpDepth < 8)
	    bitmask >>= bmpDepth;
	  bitshift = 8 - bmpDepth;
	  n = 1 << bmpDepth;
	  lcdbufsiz -= n;
	  palette = lcdbuffer + lcdbufsiz;
	  for (col = 0; col < n; col++)
	    {
	      pos = read32 (bmpFile);    //map palette to 5-6-5
	      palette[col] = ((pos & 0x0000F8) >> 3) | ((pos & 0x00FC00) >> 5)
		  | ((pos & 0xF80000) >> 8);
	    }
	}

      // Set TFT address window to clipped image bounds
      _tft.setAddrWindow (x, y, x + w - 1, y + h - 1);
      for (row = 0; row < h; row++)
	{ // For each scanline...
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
	  if (bmpFile.position () != pos)
	    { // Need seek?
	      bmpFile.seek (pos);
	      buffidx = sizeof(sdbuffer); // Force buffer reload
	    }

	  for (col = 0; col < w;)
	    {  //pixels in row
	      lcdleft = w - col;
	      if (lcdleft > lcdbufsiz)
		lcdleft = lcdbufsiz;
	      for (lcdidx = 0; lcdidx < lcdleft; lcdidx++)
		{ // buffer at a time
		  uint16_t color;
		  // Time to read more pixel data?
		  if (buffidx >= sizeof(sdbuffer))
		    { // Indeed
		      bmpFile.read (sdbuffer, sizeof(sdbuffer));
		      buffidx = 0; // Set index to beginning
		      r = 0;
		    }
		  switch (bmpDepth)
		    {  // Convert pixel from BMP to TFT format
		    case 24:
		      b = sdbuffer[buffidx++];
		      g = sdbuffer[buffidx++];
		      r = sdbuffer[buffidx++];
		      color = _tft.color565 (r, g, b);
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
		      if (r == 0)
			b = sdbuffer[buffidx++], r = 8;
		      color = palette[(b >> bitshift) & bitmask];
		      r -= bmpDepth;
		      b <<= bmpDepth;
		      break;
		    }
		  lcdbuffer[lcdidx] = color;

		}
	      _tft.pushColors (lcdbuffer, lcdidx, first);
	      first = false;
	      col += lcdidx;
	    }           // end cols
	}               // end rows
      _tft.setAddrWindow (0, 0, _tft.width () - 1, _tft.height () - 1); //restore full screen
      ret = 0;        // good render
    }
  bmpFile.close ();
  return (ret);
}

void
zDisplay::print_val (int val, int16_t x, int16_t y, int c, bool komma) //Hilfsroutine zum Daten anzeigen
{
  char buf[10];
  int16_t x1, y1;
  uint16_t w, h;
  _tft.setTextSize (1);
  _tft.setFont (&FreeSans12pt7b);

  _tft.getTextBounds ("00,00", x, y, &x1, &y1, &w, &h);
  float val2;
  if (komma == 1)
    {
      val2 = val % 100;
      val2 = val2 / 100;
      val2 = val2 + (val / 100);
      dtostrf (val2, 5, 2, buf);   //e.g. 12.34
    }
  else
    {
      val2 = val;
      dtostrf (val2, 4, 0, buf);   //e.g. 1234
    }

  _tft.fillRect (x1, y1 - 2, w + 3, h, ZGRUEN);

  if (c == 1)
    {
      _tft.setTextColor (BLACK);
      _tft.setFont (&FreeSansBold12pt7b);
    }
  else
    {
      _tft.setTextColor (WHITE);
    }
  if (c == 2)
    {
      _tft.setTextColor (RED);
    }
  _tft.setCursor (x, y);
  _tft.print (buf);
}

void
zDisplay::print_val2 (int val, int16_t x, int16_t y, int c, bool komma) //Hilfsroutine zum Daten anzeigen
{
  char buf[10];
  _canvas->setTextSize (1);
  _canvas->fillScreen (0);
  _canvas->setCursor (2, 20);
  if (komma == 1)
    {
      sprintf (buf, "%d,%02d", val / 100, val % 100);
    }
  else
    {
      sprintf (buf, "%d", val);
    }

  if (c == 1) //schwarz fett
    {
      _canvas->setFont (FETT);
      _canvas->print (buf);
      _tft.drawBitmap (x, y, _canvas->getBuffer (), _canvas->width (),
		       _canvas->height (), BLACK, ZDUNKELGRUEN);
    }
  else //weiß
    {
      _canvas->setFont (NORMAL);
      _canvas->print (buf);
      _tft.drawBitmap (x, y, _canvas->getBuffer (), _canvas->width (),
		       _canvas->height (), WHITE, ZDUNKELGRUEN);
    }
  if (c == 2) //rot
    {
      _canvas->setFont (NORMAL);
      _canvas->print (buf);
      _tft.drawBitmap (x, y, _canvas->getBuffer (), _canvas->width (),
		       _canvas->height (), RED, ZDUNKELGRUEN);
    }
}

/*
 void zDisplay::fillScreen(unsigned short int color) {
 _tft.fillScreen(color);
 }
 */

void
zDisplay::setCursor (int16_t x, int16_t y)
{
  _tft.setCursor (x, y);
}

void
zDisplay::printInt (uint16_t wertInt)
{
  int x = 300;
  int y = 200;

  _tft.setCursor (x, y);

  char wert[80] = "0000";
  sprintf (wert, "Honk: %4u %x", wertInt, wertInt);
  _tft.println (wert);

}

void
zDisplay::userShow (benutzer *user)
{
  _tft.fillRect (280, 30, 200, 160, WHITE);
  _tft.drawRect (280, 30, 200, 160, ZGRUEN);
  char namebuf[32] = "/usr/x.bmp";

  switch (user->getGodMode ())
    {
    case 1:
      strcpy (namebuf, "/god/10.bmp");
      break;
    case 2:
      strcpy (namebuf, "/god/20.bmp");
      break;
    case 0:  //keinGODMode

      namebuf[5] = user->aktuell + 48; //ASCII Wert für Zahlemann in Char schreiben (Zahl 5 = Ascii 5 wenn man 48 dazu tut
      break;
    }
  showBMP (namebuf, 300, 50);
  _tft.setTextSize (1);
  _tft.setFont (FETT);
  _tft.setTextColor (ZGRUEN);
  _tft.setCursor (300, 175);
  //_tft.print(*userName);
  _tft.print (user->getName ()); //konvertiert den Pointer von userName in c-String
  //_tft.print(user->getName()->c_str());  //konvertiert den Pointer von userName in c-String
  _tft.setFont (NORMAL);
  int x = 300;
  int y = 210;
  _tft.fillRect (280, 190, 200, 130, ZGRUEN);
  _tft.setTextColor (WHITE);
  _tft.setTextSize (1);
  _tft.setCursor (x, y - 5);
  _tft.print ("o");
  // _tft.setTextSize(3);
  _tft.setCursor (x + 15, y);
  _tft.println ("°C");

  _tft.setCursor (x, y + 30);
  _tft.println ("ml");

  _tft.setCursor (x, y + 60);
  _tft.println ("Hoibe");

  _tft.setCursor (x, y + 90);
  _tft.println ("Mass");
}

void
zDisplay::println (const char *text)
{
  zDisplay::printText ();
  _tft.println (text);
}

void
zDisplay::setTextColor (uint16_t c)
{
  _tft.setTextColor (c);
}

void
zDisplay::setTextSize (uint8_t s)
{
  _tft.setTextSize (s);
}

void
zDisplay::infoscreen (tempsens *temp, benutzer *user)
{
  temp->request ();
  _tft.setFont (NORMAL);
  _tft.fillScreen (BLACK);
  _tft.setTextColor (WHITE);
  _tft.setTextSize (1);
  _tft.setCursor (10, 20);
  _tft.println ("Z-Apfapparat INFORMATIONSTAFEL");
  _tft.setCursor (0, 40);

  for (int x = 0; x < 10; x++)
    {
      _tft.setCursor (230, 60 + x * 25);
      _tft.print (x);
      _tft.print (" ");
      _tft.print (user->username[x]);
      _tft.setCursor (350, 60 + x * 25);
      _tft.print (user->bierTag[x]);
      _tft.println (" ml");
    }
  delay (2000);
  _tft.setCursor (0, 60);
  _tft.setFont (0);
  _tft.setTextSize (2);
  _tft.print ("block: ");
  _tft.println (temp->blockTemp);
  _tft.print ("hahn:  ");
  _tft.println (temp->hahnTemp);
  _tft.print ("haus:  ");
  _tft.println (temp->hausTemp);
  _tft.print ("kuehlw:");
  _tft.println (temp->kuehlwasserTemp);
  _tft.print ("zulauf:");
  _tft.println (temp->zulaufTemp);
  temp->request ();

}
