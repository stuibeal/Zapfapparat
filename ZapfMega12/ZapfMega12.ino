/* Der original Z-Gesellschaft Z-Apfapparat
 Version 0.4 - Ereignis 2024

 STM32F411 Flow:

 STM32F411C6EU6
 - Temperaturregelung (
 -

 MEGA2560:
 - MIDI File output
 - MIDI note output
 - UI verbessern
 - Valve auf/Zu noch machen oder nicht mehr?
 - Druck
 - I2C Kommunikation mit den STM32s
 - Gute Nacht Freunde -> Zeit ändern! nach RTC gehen
 - Knöpfe vorne
 - Konfig über File einlesen
 - Drucker
 - Uhr
 - Datenlogging

 HARDWARE:
 - WS READY von 19 auf 33 (bisher PULS)
 - WS PULS von 33 auf 19!!!! weil Interrupt



 */

#include "ZapfMega12.h"

SdFat SD;  // SD-KARTE
zDisplay ZD;   // neues zDisplay Objekt

//Hier Variablen definieren
byte aktuellerTag = 1;  //dann gehts mit der Musik aus
unsigned int minTemp = 200;
unsigned int zielTemp = 200;
unsigned int totalMilliLitres = 0;
volatile byte WSpulseCount = 0;
unsigned long auswahlZeit;
int aktuellerModus = 0;
unsigned int hell;

unsigned long oldTime = millis ();
unsigned long nachSchauZeit = 0;
unsigned long hellZeit;
unsigned int hellCount = 0;
unsigned int dunkelCount = 0;
bool dunkelBool = false;

byte lichtan = LOW;

char clockString[20];

//Waehlscheibe
uint8_t zahlemann;  //Per Wählscheibe ermittelte Zahl
unsigned long kienmuehle;  //Sondereingabe bei drücken der Taste2
byte Einsteller = 1; //Globale Variable für ISR, Start bei 1

// aus Communication.h

unsigned int blockTemp; // #define transmitBlockTemp       0x40  // Data send:   blockTemp in °C*10
unsigned int auslaufTemp; // #define transmitAuslaufTemp     0x41  // Data send:   hahnTemp in °C*10
unsigned int power = 0; // #define transmitPower           0x42  // Data send:   Leistung in W (power1+power2)
unsigned int inVoltage = 0; // #define transmitInVoltage       0x43  // Data send:   inVoltage in V*100
unsigned int kuehlFlow = 0; // #define transmitKuehlFlow       0x44  // Data send:   Durchfluss Kühlwasser (extern) pro 10000ms

unsigned int highTemperatur = 200; // #define setHighTemperatur       0x60  // Data get: Zieltemperatur Block * 100 (2°C)
unsigned int midTemperatur = 600; // #define setMidTemperatur        0x61  // Data get: Normale Temperatur in °C * 100 (6°C)
unsigned int lowTemperatur = 900; // #define setLowTemperatur        0x62  // Data get: Energiespar Temperatur * 100 (9°C)
unsigned int minCurrent = 10; // #define setMinCurrent           0x63  // Data get: Current in mA / 10 (11 = 0,11 A), Untere Regelgrenze
unsigned int lowCurrent = 50; // #define setLowCurrent           0x64  // Data get: current in mA / 10, Obere Regelgrenze bei wenig Strom
unsigned int midCurrent = 600; // #define setMidCurrent           0x65  // Data get: Current in mA / 10, Obere Regelgrenze bei normalem Strom
unsigned int highCurrent = 900; // #define setHighCurrent          0x66  // Data get: Current in mA / 10, Obere Regelgrenze bei gutem Strom

unsigned int normVoltage = 500; //#define setNormVoltage          0x68  // Data get: norm Voltage * 100, passt normal, mehr als 9V macht wenig Sinn bei den Peltierelementen
unsigned int maxVoltage = 1000; //#define setMaxVoltage           0x69  // Data get: max Voltage * 100, das wäre dann eigentlich die Batteriespannung
unsigned int lowBatteryVoltage = 1140; //#define setLowBatteryVoltage    0x6A  // 11V Eingangsspannung
unsigned int midBatteryVoltage = 1200; //#define setMidBatteryVoltage    0x6B  // 12V Eingangsspannung
unsigned int highBatteryVoltage = 1300; //#define setHighBatteryVoltage   0x6C  // 13V Eingangsspannung

unsigned int wasserTemp; //#define setWasserTemp           0x6D  // Data get: kühlwasserTemp in °C*100 vom DS18B20 Sensor vom Master: Fühler neben Peltier
unsigned int einlaufTemp; //#define setEinlaufTemp          0x6E  // Data get: Biertemperatur in °C*100 vom DS18B20 Sensor vom Master: Bierzulauf

unsigned int consKp = 80; //#define setConsKp               0x70  // Data get: konservativer Kp (ist alles mal 100)
unsigned int consKi = 5; //#define setConsKi               0x71  // Data get: konservativer Ki
unsigned int consKd = 5; //#define setConsKd               0x72  // Data get: konservativer Kd
unsigned int aggKp = 150; //#define setAggKp                0x73  // Data get: aggressiver Kp
unsigned int aggKi = 20; //#define setAggKi                0x74  // Data get: aggressiver Ki
unsigned int aggKd = 50; //#define setAggKd                0x75  // Data get: aggressiver Kd
unsigned int unterschiedAggPid = 10; //#define setUnterschiedAggPid    0x75  // mal zehn grad nehmen ab wann der aggressiv regelt
unsigned int steuerZeit = 200; //#define setSteuerZeit           0x76  // alle sekunde mal nachjustieren

bool ebiModeBool = false; //#define ebiMode               0xF9      //    1 an, 0 aus       Temperatur auf 2°C, Hahn auf, Zapfmusik
bool beginZapfBool = false; // #define beginZapf             0xFA      //    Beginn das Zapfprogramm -> PID auf aggressiv
bool endZapfBool = false; //#define endZapf               0xFB      //    Data send : milliliter
bool kurzBevorZapfEndeBool = false; //#define kurzBevorZapfEnde     0xFC      //    sagt das wir kurz vor Ende sind → Valve schließen -> PID auf konservativ

String dataOnSd = "";

// I2C Kommunikation
uint8_t recieveByte[3]; // Das will der Master von uns
uint8_t sendeByte[2]; //Bytearray zum senden über I2C
uint8_t aRxBuffer[3];
uint8_t aTxBuffer[3];

// Steuerungskram
unsigned int zielTemperatur = highTemperatur;
unsigned int setVoltage = normVoltage; //mal gaaanz klein beginnen
unsigned int setCurrent = minCurrent;
unsigned int maxCurrent = midCurrent; //Obere Regelgrenze auf mittlere stellen
bool lowPower = false;
bool veryLowPower = false;
bool debugMode = false;

// DREHENCODER
// encoder lib: http://www.pjrc.com/teensy/td_libs_Encoder.html
//#define ENCODER_OPTIMIZE_INTERRUPTS
Encoder Dreher (rotaryDT, rotaryCLK); //PINS für Drehgeber
//   Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
//   avoid using pins with LEDs attached

volatile int DreherKnopfStatus = 0; //Da wird der Statatus vom Drehgeberknopf gelesen
long oldPosition = 0; //Fuer Drehgeber

DateTime dateTime = DateTime (0, 1, 1, DateTime::SATURDAY, 0, 0, 0); //DCF RealTimeClock DateTime Objekt

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver (); // called this way, it uses the default address 0x40
tempsens temp;	//Temperatursensorik
benutzer user;  //Benutzer
audio sound; 	//Audioobjekt
zValve ventil; // Ventilsteuerung, Druck, Reinigungspumpe
zPrinter drucker;

unsigned int tempAnzeigeZeit = millis (); //für zehnsekündige Temperaturanzeige

void
setup (void)
{
  //Erstmal bei den anderen MCs den Strom an
  pinMode (otherMcOn, OUTPUT);
  digitalWrite (otherMcOn, HIGH);

  //Pins herrichten
  pinMode (helligkeitSensor, INPUT);
  pinMode (lcdBacklightPwm, OUTPUT); //LCD Display Hintergrundbeleuchtung
  analogWrite (lcdBacklightPwm, 128);

  // Tasten in der Front
  pinMode (taste1, INPUT);
  pinMode (taste2, INPUT);
  pinMode (taste1Pwm, OUTPUT);
  pinMode (taste2Pwm, OUTPUT);
  analogWrite (taste1Pwm, 10);
  analogWrite (taste2Pwm, 10);

  //Serial.begin(115200); //kein Serial!!! MIDI!!!!!!!!!

  //DCF 77 Empfänger an STM uC

  //TFT
  ZD.beginn (&SD);  //mit Pointer zur SD starten

  //SD
  if (!SD.begin (SD_CS))
    {  // nachschauen ob die SD-Karte drin und gut ist
      ZD.println ("SD Karte nicht vorhanden! Bitte richten!");
      /***Hier funktion programmieren:
       ZD.println("Ohne SD Karte fortfahren: Z-Knopf drücken!");
       ***/
      return;   // don't do anything more if not
    }
  else
    {
      ZD.println ("SD Karte bassd - OPTIMAL!");

    }

  ZD.showBMP ("/bmp/z-logo.bmp", 200, 0);

  //Printer
  drucker.initialise(&Serial2, &user, &buf[0]);

  //Temperaturfuehler
  temp.begin ();
  ZD.println ("Temperaturfuehler hochgefahren...");

  //FLOWMETER
  pinMode (FLOW_SM6020, OUTPUT);
  digitalWrite (FLOW_SM6020, HIGH);
  pinMode (FLOW_WINDOW, INPUT);    //Wenn durchfluss, dann true
  ZD.println ("Flowmeter ifm SM6020 ein");

  //Rotary Encoder
  pinMode (rotaryKnob, INPUT); // Drehgeberknopf auf Input
  attachInterrupt (5, Einstellerumsteller_ISR, FALLING); //ISR= interrupt service routine; alternativ: (digitalPinToInterrupt(pin), ISR, mode)
  /*External Interrupts: 2 (interrupt 0), 3 (interrupt 1), 18 (interrupt 5), 19 (interrupt 4), 20 (interrupt 3), and 21 (interrupt 2).
   These pins can be configured to trigger an interrupt on a low value, a rising or falling edge, or a change in value. See the attachInterrupt() function for details.
   Den Schalter hardwaremäßig entprellt: 320k pullup, 10k pulldown und signal-kondensator-ground -> kondensator lädt und entprellt.
   ABER: SDA/SCL ist parallel zu PIN 20/21, brauch den Interrupt also selber. So sind nur pins 2,3,18,19 am MEGA frei.
   */
  //Aktueller Einstellmodus (1=Temperatur in °C*100, 2=Zapfmenge in ml
  //Wählscheibe
  pinMode (WSpuls, INPUT); // WSpuls auf Input (Interrupt)
  pinMode (WSready, INPUT);  //Wählscheibe Puls

  sound.starte(&SD);
  ZD.println ("Harte Musik bereit");

  //Altdaten auslesen (SD karte) nach Stromweg oder so...

  //PWM Treiber hochfahren
  ZD.println ("start PWM waehler....");
  pwm.begin ();
  pwm.setPWMFreq (1000);  // Maximale Frequenz (1kHz) -> reicht für LED
  pwm.setPWM (0, 0, 16);   //helle LEDS abdunkeln grün
  pwm.setPWM (11, 0, 16);  //helle LEDS abdunkeln weiß
  for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++)
    {
      pwm.setPWM (pwmnum, 0, 64); //alles leicht einschalten
    }
  ZD.println ("PWM Waehlscheibe ready...");

  //Valve
  ventil.begin ();
  ventil.check ();   //dann sollte das aufgehen
  ZD.println ("Ventilsteuerung aktiviert");

  //DCF RTC
  RTC_DCF.begin ();
  RTC_DCF.enableDCF77Reception ();
  RTC_DCF.enableDCF77LED ();   //später ausschalten in der nacht!)
  RTC_DCF.setDateTime (&dateTime); //Damit irgendwas drin is
  ZD.println ("RTC DCF77 aktiviert");

  //I2C
  Wire.begin (); // Master of the universe
  Wire.setClock (400000); // I2C in FastMode 400kHz

  //Make Windows 95 great again
  anfang ();
  oldTime = millis ();
  nachSchauZeit = millis ();


}  //VOID SETUP

void
waehlscheibe ()
{
  sound.setStandby (true); //dann checkt er nicht ob er ausschalten soll
  sound.on ();
  DEBUGMSG(sound.debugmessage);
  auswahlZeit = millis ();

  flowDataSend (LED_FUN_1, 10, 200, 0);

  if (!digitalRead (taste2))
    {
      //wenn man sich verwählt hat bei der Nummerneingabe wirds gelöscht
      kienmuehle = 0;

      ventil.openValve ();
      analogWrite (taste2Pwm, 10);
      //geh mal Zapfen
      iBefehl (tempi2c, beginZapf);
      //flowDataSend(LED_FUN_2, 1000,1000);

    }
  else
    {
      analogWrite (TASTE2_LED, 255);
      //hier eigentlich die Kienmühle funktion

    }
  sound.pruefe ();
  DEBUGMSG(sound.debugmessage);

  zahlemann = 0;
  for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++)
    {
      pwm.setPWM (pwmnum, 0, 256); //Licht AN
    }
  bool old_waehler2 = 1;
  bool waehler2 = digitalRead (WSpuls);
  while (digitalRead (WSready) == 1)
    {

      old_waehler2 = waehler2;
      waehler2 = digitalRead (WSpuls);

      unsigned long temptime;

      if (waehler2 < old_waehler2)
	{
	  temptime = millis ();  //hier die Wählscheibe auslesen
	}

      if ((waehler2 > old_waehler2) && (millis () - temptime > 50))
	{ //wenn Signal wieder von 0V auf 5V geht und mehr als 50ms vergangen sind, eins hochzählen
	  zahlemann++; //Wählscheibe (US): 60ms PULS 0V, 40ms Pause (5V), ánsonsten immer 5V
	  temptime = millis ();
	  pwm.setPWM (zahlemann, 4096, 0);
	  if (zahlemann > 1)
	    {
	      pwm.setPWM (zahlemann - 1, 0, 512);
	    }
	}
    }

  flowDataSend (GET_ML, 0, 0, 0);  //LEDFun ausschalten
  sound.pruefe ();
  DEBUGMSG(sound.debugmessage);

  if (zahlemann > 0)
    {
      for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++)
	{
	  pwm.setPWM (pwmnum, 0, 64); //Licht aus
	}
      pwm.setPWM (zahlemann, 4096, 0);

      //Ab hier werden die Userdaten angezeigt.
      if ((zahlemann < 11) && !digitalRead (taste2))
	{
	  if (zahlemann == 10)
	    {
	      zahlemann = 0; // user beim "nuller"
	    }
	  user.aktuell = zahlemann;
	  flowDataSend (SET_USER_ML, 0, 0, user.menge ());
	  delay (500); //damit der Zeit hat
	  flowDataSend (BEGIN_ZAPF, 0, 0, 0);

	  switch (user.getGodMode ())
	    {
	    case 1:
	      sound.loadLoopMidi("d_runni2.mid");
	      break;
	    case 2:
	      sound.loadLoopMidi("keen.mid");
	      break;
	    }

	  if (user.temp () > minTemp)
	    {
	      zielTemp = user.temp ();
	    }
	  else
	    {
	      zielTemp = minTemp;
	    }

	  userShow ();  // Zeigt die Userdaten an
	  auswahlZeit = millis ();

	  beginZapfBool = true;

	}

      /*!
       * Addiert die gewählten Zahlen zu kienmuehle dazu
       */
      if ((digitalRead (taste2)) > 0 && zahlemann > 0)
	{
	  kienmuehle = kienmuehle * 10;  //das passt so, Alfred!
	  if (zahlemann < 10)
	    {
	      kienmuehle += zahlemann;
	    }
	  ZD.printText ();
	  sprintf (buf, "Nr: %9d", kienmuehle);
	  ZD._tft.println (buf);
	}

    }
  //Erst am schluss das MIDI RESET aufheben!

  sound.setStandby (beginZapfBool); //wenn er nicht zapft, kein Standby!
  sound.pruefe ();
  DEBUGMSG(sound.debugmessage);
}

void
waehlFunktionen ()
{
  switch (kienmuehle)
    {
    case 847: // UHRZEIT
      RTC_DCF.getDateTime (&dateTime);
      sprintf (buf, "Es is %02u:%02u:%02u am %02u.%02u.%02u",
	       dateTime.getHour (), dateTime.getMinute (),
	       dateTime.getSecond (), dateTime.getDay (), dateTime.getMonth (),
	       dateTime.getYear ());
      ZD.printText ();
      ZD._tft.println (buf);
      break;
    case 463633: //GODOFF
      user.setGodMode (0);
      ZD.userShow (&user);
      break;
    case 43373: //IDDQD
      user.setGodMode (1);
      ZD.userShow (&user);
      break;
    case 5336: // KEEN
      user.setGodMode (2);
      ZD.userShow (&user);
      break;
    case 1275: //Die Telefonnummer der Kienmühle
      sound.mp3Play (11, 1); //Magnum
      pwm.setPWM (0, 0, 2048);   //Grüne LED an
      for (uint8_t dw = 0; dw < 2; dw++)
	{ //mega Lightshow!!
	  for (uint16_t i = 0; i < 11; i++)
	    {
	      delay (50);
	      for (uint8_t x = 11; x > 0; x--)
		{
		  delay (30);
		  pwm.setPWM (x + i, 4096, 0);
		  pwm.setPWM (x + i + 1, 0, 4096);
		}
	    }

	  for (uint16_t i = 11; i > 0; i--)
	    {
	      delay (100 % i);
	      for (uint8_t x = 0; x < 11; x++)
		{
		  delay (50);
		  pwm.setPWM (x + i - 1, 0, 4096);
		  pwm.setPWM (x + i, 4096, 0);
		}
	    }
	  delay (500);
	}
      pwm.setPWM (11, 0, 16);  //helle LEDS abdunkeln weiß
      pwm.setPWM (0, 0, 16);   //helle LEDS abdunkeln grün
      break;
    case 9413: //Telefonnummer
	       //Start Reinigungsprogramm
      { //hier braces damit die variablen allein hier sind
	ZD.printText ();
	ZD._tft.println ("        REINIGUNGSPROGRAMM");
	iBefehl (tempi2c, zapfenStreich);
	ventil.cleanPumpOn ();
	delay(500); //zum taste loslassen!
	unsigned long waitingTime = millis ();
	int sekunden = 0;
	while (!digitalRead (taste1))
	  {
	    if (millis()-waitingTime >= 1000)
	      {
		waitingTime = millis ();
		sekunden++;
		ZD.printText ();
		ZD._tft.println (sekunden);
	      }

	  }
	ventil.cleanPumpOff ();
	ZD.printText ();
	ZD._tft.println ("ENDE REINIGUNGSPROGRAMM      ");
      }
      break;

    default:
      uint16_t varSet = kienmuehle / 1000; //ersten zwei zahlen
      uint16_t varContent = kienmuehle % 1000; // letzten drei zahlen
      switch (varSet)
	{
	case 9: //Mediaplayer

	  break;
	case 11:

	  break;
	}
      if (kienmuehle > 10000)
	{
	  if (kienmuehle > 11000 && kienmuehle < 11999)
	    {
	      consKp = kienmuehle - 11000;
	      iDataSend (tempi2c, setConsKp, consKp);
	    }
	  if (kienmuehle > 12000 && kienmuehle < 12999)
	    {
	      consKi = kienmuehle - 12000;
	      iDataSend (tempi2c, setConsKi, consKi);
	    }
	  if (kienmuehle > 13000 && kienmuehle < 13999)
	    {
	      consKd = kienmuehle - 13000;
	      iDataSend (tempi2c, setConsKd, consKd);
	    }
	  if (kienmuehle > 14000 && kienmuehle < 14999)
	    {
	      aggKp = kienmuehle - 14000;
	      iDataSend (tempi2c, setAggKp, aggKp);
	    }
	  if (kienmuehle > 15000 && kienmuehle < 15999)
	    {
	      aggKi = kienmuehle - 15000;
	      iDataSend (tempi2c, setAggKi, aggKi);
	    }
	  if (kienmuehle > 16000 && kienmuehle < 16999)
	    {
	      aggKd = kienmuehle - 16000;
	      iDataSend (tempi2c, setAggKd, aggKd);
	    }
	}

      if (kienmuehle > 9900)
	{   //Plays songs from ordner 11
	  sound.mp3Play (11, kienmuehle - 9900);
	}

    }
  kienmuehle = 0;

}


void
anfang (void)
{
  //mp3.playTrack(3); //Brantl Edel Pils
  //ZD._tft.fillScreen(BLACK);

  ZD.showBMP ("/bmp/back01.bmp", 0, 0);

  analogWrite (TASTE2_LED, 10);
  analogWrite (TASTE1_LED, 10);
  analogWrite (lcdBacklightPwm, 20);
  for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++)
    {
      pwm.setPWM (pwmnum, 0, 64); //alles leicht einschalten
    }
  pwm.setPWM (0, 0, 16);   //helle LEDS abdunkeln grün
  pwm.setPWM (11, 0, 16);  //helle LEDS abdunkeln weiß
  DEBUGMSG("vor Usershow");
  userShow ();
  sound.bing();

}

void
aufWachen (void)
{
  // nothing
}

void
einSchlafen (void)
{
  //nothing
}

void
tickMetronome (void)
// flash a LED to the beat
{
  static uint32_t lastBeatTime = 0;
  static boolean inBeat = false;
  static uint8_t leuchtLampe = B00000001;
  uint16_t beatTime;

  beatTime = 60000 / sound._SMF->getTempo () / 4; // msec/beat = ((60sec/min)*(1000 ms/sec))/(beats/min)
  if (!inBeat)
    {
      if ((millis () - lastBeatTime) >= beatTime)
	{
	  lastBeatTime = millis ();

	  inBeat = true;
	  if (leuchtLampe & 1)
	    {
	      digitalWrite (TASTE1_LED, HIGH);
	    }
	  else
	    {
	      digitalWrite (TASTE2_LED, HIGH);
	    }

	  //ZD.setCursor(5, 30);
	  //sprintf(buf, "leuchtLampe: %02x ", leuchtLampe);
	  //ZD.printInt(totalMilliLitres);
	  flowDataSend (LED_FUN_4, leuchtLampe, 0xFF, 0);
	  //flowDataSend(LED_FUN_1, 1, 70);
	  //leuchtLampe << 1;
	  leuchtLampe++;

	}
    }
  else
    {
      if ((millis () - lastBeatTime) >= 100) // keep the flash on for 100ms only
	{

	  if (!(leuchtLampe & 1))
	    {
	      analogWrite (TASTE1_LED, 20);
	    }
	  else
	    {
	      analogWrite (TASTE2_LED, 20);
	    }

	  inBeat = false;

	}
    }
}

void
seltencheck (void)
{
  //sprintf(buf, "hell: %d dunkelcount %d", hell, dunkelCount);
  //DEBUGMSG(buf);

  hell = analogRead (helligkeitSensor);
  inVoltage = temp.getInVoltage ();

  //Hier checken ob was gespielt wird, ansonsten audio aus
  //audio.check();

  if (hell < 6)
    {
      dunkelCount++;
    }
  else
    {
      dunkelCount = 0;
    }

  if (dunkelCount > 9)
    {
      //Nachtprogramm
      DEBUGMSG("Nachtprogramm")
      dunkelBool = true;

      sound.mp3Play (12, 1); //Gute NAcht Freunde

      for (int x = 0; x < 256; x++)
	{
	  analogWrite (lcdBacklightPwm, x);
	  delay (50);
	}

      for (int x = 255; x >= 0; x--)
	{
	  analogWrite (TASTE1_LED, x);
	  delay (50);
	}

      for (int x = 255; x >= 0; x--)
	{
	  analogWrite (taste2Pwm, x);
	  delay (50);
	}

      for (int x = 255; x >= 0; x--)
	{
	  for (uint8_t pwmnum = 0; pwmnum < 13; pwmnum++)
	    {
	      pwm.setPWM (pwmnum, 0, x); //Wählscheibe runterdimmen
	    }
	  delay (10);
	}
      flowDataSend (endZapf, 0, 0, 0);
      flowDataSend (zapfenStreich, 0, 0, 0);
      iBefehl (tempi2c, zapfenStreich);
      user.gesamtMengeTag = 0;
      aktuellerTag++;
      ventil.closeValve ();
      for (int x = 0; x <= 11; x++)
	{
	  ventil.check ();
	  delay (1000);
	}

      delay (130000); //dann ist gute nach Freunde aus
      sound.mp3Play (12, aktuellerTag); //lied 2-7
      delay (240000);

      digitalWrite (AUDIO_AMP, LOW);
      digitalWrite (otherMcOn, LOW);
      //Daten noch loggen

      //Userdaten noch löschen
      for (int x = 0; x <= 10; x++)
	{
	  user.bierTag[x] = 0;
	}

      while (dunkelBool == true)
	{
	  hell = analogRead (helligkeitSensor);
	  if (hell > 200)
	    {
	      hellCount++;
	      delay (10000);
	    }
	  else
	    {
	      hellCount = 0;
	    }
	  if (digitalRead (taste1) == HIGH)
	    {
	      Serial.println ("Taste gedrückt");
	      dunkelBool = false;
	      delay (100);
	      anfang ();
	    }
	  if (hellCount > 9)
	    {
	      Serial.println ("Hellcount über 1");
	      dunkelBool = false;
	      digitalWrite (AUDIO_AMP, HIGH);
	      digitalWrite (otherMcOn, HIGH);
	      delay (3000);   //pause machen damit die auch alle hochkommen
	      anfang ();
	    }
	}
      Serial.println ("NAchtprogramm so verlassen");
    }

  temp.request ();

}
//ZD.print_val(zulauf.getTempC() * 100, 200, 170, 1, 1);

void
belohnungsMusik ()
{
  if (user.tag () > 2000 && user.getMusik () == 0)
    {
      user.setMusik (1);
      sound.mp3Play (user.aktuell, 1);
    }
  if (user.tag () > 2500 && user.getMusik () == 1)
    {
      user.setMusik (2);
      sound.mp3Play (user.aktuell, 2);
    }
  if (user.tag () > 3000 && user.getMusik () == 2)
    {
      user.setMusik (3);
      sound.mp3Play (user.aktuell, 3);
    }
  if (user.tag () > 3500 && user.getMusik () == 3)
    {
      user.setMusik (0);
      sound.mp3Play (user.aktuell, 4);
      ZD.showBMP ("/bmp/back02.bmp", 0, 0);
      userShow ();
    }

}

//Infoknopf
void
infoseite (void)
{
  analogWrite (TASTE1_LED, 10);
  sound.loadSingleMidi ("SKYFALL.MID");
  ZD.infoscreen (&temp, &user);

  //ZD.setFont(&FreeSans9pt7b);
  /*
   ZD.print("DS18 Block: "); ZD.printInt(temp.block1Temp);
   ZD.print("DS18 Auslauf: "); ZD.printInt(temp.hahnTemp);
   ZD.print("DS18 Zulauf: "); ZD.printInt(temp.zulaufTemp);
   ZD.print("DS18 Gehäuse: "); ZD.printInt(temp.hausTemp);
   ZD.print("DS18 Kühlwa.: "); ZD.printInt(temp.kuehlwasserTemp);
   ZD.print("cons. kP: "); ZD.printInt(consKp);
   ZD.print("cons. kI: "); ZD.printInt(consKi);
   ZD.print("cons. kD: "); ZD.printInt(consKd);
   ZD.print("aggr. kP: "); ZD.printInt(aggKp);
   ZD.print("aggr. kI: "); ZD.printInt(aggKi);
   ZD.print("aggr. kD: "); ZD.printInt(aggKd);
   */

  /*
   ZD.setCursor(10,295);
   ZD.println("");
   ZD.print("Gesamtbier Tag: "); ZD.print(BierMengeTag);ZD.print("      ");
   ZD.print("Gesamtbier: "); ZD.println(BierMengeTotal);
   */

  for (int x = 10; x < 256; x++)
    {
      analogWrite (TASTE1_LED, x);
      delay (10);
    }
  while (digitalRead (taste1))
    {
    }
  for (int x = 255; x > 11; x--)
    {
      analogWrite (TASTE1_LED, x);
      delay (10);
    }
  anfang ();
  userShow ();

}

void
loop ()
{
  //DEBUGMSG("ich bin im loop");
  byte oldeinsteller = Einsteller;
  sound.midiNextEvent();

  Drehgeber ();

  //Valve Control
  ventil.check ();

  //Wenn die Wählscheibe betätigt wird
  if (digitalRead (WSready))
    {
      waehlscheibe ();
    }

  //Wenn was rumgestellt wird
  if (oldeinsteller != Einsteller)
    {
      UserDataShow ();
    }

  //Wenn jemand an den Tasten rumspielt
  if (digitalRead (taste1))
    {
      infoseite ();
    }

  // Wenn Nummer Fertig und Taste losgelassen
  if (!digitalRead (taste2) && kienmuehle > 0)
    {
      analogWrite (TASTE2_LED, 20);
      waehlFunktionen ();
    }

  if (user.getGodMode () == 0)  //sound._SMF.->isPaused () ||
    {
      if ((millis () - oldTime > 1000))
	{
	  oldTime = millis ();
	  anzeigeAmHauptScreen ();
	  sound.pruefe ();
	  if (DEBUG_A)
	    {
	      DEBUGMSG(sound.debugmessage);
	    }

	  //RTC_DCF.getDateTime(&dateTime);
	  //printClock();
	}

      if ((millis () - nachSchauZeit) > 10000 && !beginZapfBool)
	{
	  seltencheck ();
	  nachSchauZeit = millis ();

	}
    }

  if (beginZapfBool)
    {
      if (user.getGodMode () > 0)
	{
	  oldFlowWindow = flowWindow;
	  flowWindow = digitalRead (FLOW_WINDOW);
	  if (oldFlowWindow == true && flowWindow == false)
	    {
	      sound._SMF->pause (true);
	    }
	  if (oldFlowWindow == false && flowWindow == true)
	    {
	      sound._SMF->pause (false);
	    }
	  if (!sound._SMF->isPaused ())
	    {
	      tickMetronome ();
	    }
	}

      flowDataSend (GET_ML, 0, 0, 0); // aktuelle ml vom Flow uC abfragen

      // Nachschaun ob er fertig ist und dann bingen und zamschreim
      if (totalMilliLitres >= user.menge () || digitalRead (taste2))
	{
	  if (user.getGodMode () == 1)
	    {
	      ZD.showBMP ("/god/11.bmp", 300, 50);
	    }
	  sound._SMF->close ();
	  sound.midiSilence ();
	  ventil.check ();
	  sound.bing ();

	  //Sollte er abgebrochen haben:
	  if (totalMilliLitres < user.menge ())
	    {
	      drucker.printerErrorZapfEnde (totalMilliLitres);
	    }

	  user.addBier (totalMilliLitres);

	  drucker.printerZapfEnde (totalMilliLitres);
	  UserDataShow ();
	  beginZapfBool = false;
	  sound.setStandby (beginZapfBool);
	  dataLogger ();
	  belohnungsMusik ();

	}

      // Nachschaun ob er eventuell zu lang braucht und nix zapft
      if (((millis () - auswahlZeit) > 10000) && (totalMilliLitres < 5))
	{
	  beginZapfBool = false;
	  sound.setStandby (beginZapfBool);
	  iBefehl (tempi2c, endZapf);
	  ventil.check ();
	  for (uint8_t pwmnum = 1; pwmnum < 11; pwmnum++)
	    {
	      pwm.setPWM (pwmnum, 0, 64); //alles leicht einschalten
	    }
	}

      if ((user.menge () - totalMilliLitres) < 30)
	{
	  iBefehl (tempi2c, kurzBevorZapfEnde);
	  ventil.check ();
	}
    }

  //Check Knöpfe (Benutzer) -> Display up -> Zapfprogramm

  //Zwischendurch mal was protokollieren (alle 5 minuten oder so)
}

void
userShow (void)
{
  Einsteller = 2; //Wieder bei Temperatur beginnen beim Drehknebel
  //char userName[BenutzerName[zahlemann].length() + 1];
  //strcpy(userName, BenutzerName[zahlemann].c_str());
  ZD.userShow (&user);
  UserDataShow ();
}

void
iBefehl (uint8_t empfaenger, uint8_t befehl)
{

  Wire.beginTransmission (empfaenger); // transmit to device
  Wire.write (befehl);        // mach Das du stück
  Wire.endTransmission ();    // stop transmitting

}

void
iDataSend (byte empfaenger, byte befehl, unsigned int sendedaten)
{

  Wire.beginTransmission (empfaenger); // transmit to device
  sendeByte[0] = highByte(sendedaten);
  sendeByte[1] = lowByte(sendedaten);
  Wire.write (befehl);        // mach Das du stück
  Wire.write (sendeByte[0]); // Schick dem Master die Daten, Du Lappen
  Wire.write (sendeByte[1]); // Schick dem Master die anderen Daten
  Wire.endTransmission ();    // stop transmitting

}

void
i2cIntDataSend (byte empfaenger, byte befehl, unsigned int sendedaten)
{
  Wire.beginTransmission (empfaenger); // transmit to device
  sendeByte[0] = highByte(sendedaten);
  sendeByte[1] = lowByte(sendedaten);
  Wire.write (befehl);        // mach Das du stück
  Wire.write (sendeByte[0]); // Schick dem Master die Daten, Du Lappen
  Wire.write (sendeByte[1]); // Schick dem Master die anderen Daten
  Wire.endTransmission ();    // stop transmitting
}

void
flowDataSend (uint8_t befehl, uint8_t option1, uint8_t option2, uint16_t wert)
{
  if (wert != 0)
    {
      option1 = highByte(wert);
      option2 = lowByte(wert);
    }
  Wire.beginTransmission (FLOW_I2C_ADDR); // schicke Daten an den Flow
  Wire.write (befehl);        // mach Das du stück
  Wire.write (option1); // optionen siehe communication.h
  Wire.write (option2); //
  Wire.endTransmission ();    // stop transmitting

  Wire.requestFrom (FLOW_I2C_ADDR, FLOW_I2C_ANTWORTBYTES); // Daten vom Flow müssen immer geholt werden
  while (Wire.available ())
    { // wenn Daten vorhanden, hol die Dinger
      aRxBuffer[0] = Wire.read (); // ein Byte als char holen
      aRxBuffer[1] = Wire.read (); // zweites Byte als char holen
    }
  totalMilliLitres = (aRxBuffer[0] << 8) + aRxBuffer[1]; // da der Flow immer die aktuellen ml ausgibt kann man die gleich in die Variable schreiben
}

void
anzeigeAmHauptScreen (void)
{
  //DEBUGMSG("vor transmitBlocktemp");
  //ZD.print_val2 (temp.getBlock1Temp (), 20, 100, 3, 1);
  ZD.printVal (temp.getBlock1Temp (), 25, 100, WHITE, ZDUNKELGRUEN, &FETT, KOMMA);
  //DEBUGMSG("vor transmitauslauf");
  //DEBUGMSG("vor transmitauslauf");
  ZD.print_val2 (temp.getBlock2Temp (), 20, 125, 1, 1);
  ZD.print_val2 (totalMilliLitres, 20, 150, 1, 0);
  ZD.printText ();
  ZD._tft.println (ventil.getPressure ());

}

/* Name:			dataLogger
 * Beschreibung:	Diese Funktion schreibt die aktuellen Daten auf die SD Karte
 *
 */
void
dataLogger (void)
{
  // TBD: other files müssen alle zu sein
  // jeden Tag ein File, ein gesamtfile

  //Zeit einlesen
  RTC_DCF.getDateTime (&dateTime);

  char fileBuf[20] = "";
  String dataString = "";
  sprintf (fileBuf, "LOG_%02u%02u%2u.csv", dateTime.getDay (),
	   dateTime.getMonth (), dateTime.getYear ());

  if (!SD.exists (fileBuf))
    {
      //Wenn Datei noch nicht vorhanden, Kopfzeile schreiben!
      dataString = "Datum Zeit, ";
      for (uint8_t x = 0; x < 10; x++)
	{
	  dataString += user.username[x];
	  dataString += ", ";
	}
      dataString += "Gesamtmenge, ";
      dataString += "Batterie-Volt, ";
      dataString += "Helligkeit";

      File dataFile = SD.open (fileBuf, FILE_WRITE);
      // if the file is available, write to it:
      if (dataFile)
	{
	  dataFile.println (dataString);
	  dataFile.close ();
	}
      // if the file isn't open, pop up an error:
      else
	{
	  //Serial.println("konnte z-log.csv nicht öffnen");
	}
    }

  // Daten schreiben
  char timeBuf[20] = "00.00.00 00:00:00, ";
  sprintf (timeBuf, "%02u.%02u.%02u %02u:%02u:%02u, ", dateTime.getDay (),
	   dateTime.getMonth (), dateTime.getYear (), dateTime.getHour (),
	   dateTime.getMinute (), dateTime.getSecond ());
  File dataFile = SD.open (fileBuf, FILE_WRITE);

  dataString = timeBuf;

  for (uint8_t x = 0; x < 11; x++)
    {
      dataString += String (user.tag ());
      dataString += ",";
    }
  dataString += String (user.gesamtMengeTag);
  dataString += ",";
  dataString += String (inVoltage);
  dataString += ",";
  dataString += String (hell);

  // if the file is available, write to it:
  if (dataFile)
    {
      dataFile.println (dataString);
      dataFile.close ();
    }
  // if the file isn't open, pop up an error:
  else
    {
      //Serial.println("konnte z-log.csv nicht öffnen");
    }

}


void
UserDataShow ()
{
  int x = 400;
  int y = 210;
  switch (Einsteller)
    {
    case 1:
      ZD.print_val (user.temp (), x, y, 1, 1);
      ZD.print_val (user.menge (), x, y + 30, 0, 0);
      ZD.print_val (user.tag () / 5, x, y + 60, 0, 1);
      ZD.print_val (user.gesamtMengeTag / 10, x, y + 90, 0, 1);
      break;
    case 2:
      ZD.print_val (user.temp (), x, y, 0, 1);
      ZD.print_val (user.menge (), x, y + 30, 1, 0);
      ZD.print_val (user.tag () / 5, x, y + 60, 0, 1);
      ZD.print_val (user.gesamtMengeTag / 10, x, y + 90, 0, 1);
      break;
    }

}

void
Drehgeber ()
{
  int x = 400;
  int y = 210;

  long newPosition = Dreher.read ();
  if (newPosition != oldPosition)
    {
      switch (Einsteller)
	{
	case 1:
	  user.setTemp (user.temp () + (newPosition - oldPosition));
	  if ((user.temp () < minTemp))
	    {
	      user.setTemp (minTemp);
	    }
	  ZD.print_val (user.temp (), x, y, 1, 1);
	  break;
	case 2:
	  user.setMenge (user.menge () + (newPosition - oldPosition));
	  if (user.menge () <= 20)
	    {
	      user.setMenge (20);
	    }
	  ZD.print_val (user.menge (), x, y + 30, 1, 0);
	  break;
	case 3:
	  if (newPosition < oldPosition)
	    {
	    }
	  else
	    {
	    }
	  ZD.print_val (user.tag (), x, y + 60, 2, 0);
	  break;
	}

      oldPosition = newPosition;

    }
}

void
Einstellerumsteller_ISR ()
{ //Interruptroutine, hier den Drehgeberknopf abfragen
  DreherKnopfStatus = digitalRead (rotaryKnob);
  Einsteller++;
  if (Einsteller > 2)
    {
      Einsteller = 1;
    }
  UserDataShow ();
}
