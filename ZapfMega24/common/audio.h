/*
 * audio.h
 *
 *  Created on: 21.06.2023
 *      Author: alfred3
 */

#ifndef AUDIO_H_
#define AUDIO_H_
#include  <Arduino.h>
#include "MD_MIDIFile.h"
#include "MD_YX5300.h"
#include "string.h"

#ifndef DEBUG_A
#define DEBUG_A 0
#endif
//#define AUDIO_WARTEZEIT 1500

#define USE_SOFTWARESERIAL 0   ///kein Softwareserial, native Port! - für MIDI
#define MP3Stream Serial3  // Hängt am Serial Port 3 für MP3-Player
#define MIDI Serial // MIDI hängt am Serial0
#define MIDI_SERIAL_RATE 31250

// Audio
#define AUDIO_BOARD      41  //Audioboard mit Roland SCB-7 einschalten (DC Wandler wird geschaltet)
#define AUDIO_AMP        40  //wenn LOW sind die ClassD Verstärker gemutet
#define mp3RX             15  // Serial3: RX15 TX14: MP3 Player YX5300
#define mp3TX             14
#define MIDI_TX           1  // Serial: Midi Out
#define BEEP_OUT          11  // beeper out -> not really used?
#define MIDI_RESET        A14 // Reset Knopf vom SCB-7 (high: reset)

class audio: public MD_YX5300, MD_MIDIFile {
public:
	static const unsigned long MIDI_RESET_WARTEZEIT = 500;
	static const unsigned long AUDIO_WARTEZEIT = 1500;
	static const unsigned long AUDIO_ON_WARTEZEIT = 500;
	static const unsigned long AUDIO_STANDBYZEIT = 30000;
	static const int AUDIO_ON = 0;
	static const int AUDIO_SHUTDOWN = 1;
	static const int AUDIO_OFF = 2;
	static const int AUDIO_RESTART = 3;
	static const int AUDIO_MIDI_RESET = 4;
	static const int AUDIO_AMP_ON = 5;
	static const int AUDIO_STANDBY = 6;
	static const int DING = 1;  //MICROWAVE DING
	static const int BRANTL = 3; //Brantl Edel Pils

	audio();
	virtual ~audio();
	void starte(SdFat *pSD, MD_MIDIFile *pSMF, MD_YX5300 *mp3); //Mit Pointer starten
	void pruefe();
	bool pruefePlaying();
	void on(void);
	void off(void);
	void midiReset(void);
	void setStandby(bool stby);
	void mp3Play(uint8_t folder, uint8_t song);
	void bing();
	inline bool isOn() {
		return (state == AUDIO_ON || state == AUDIO_STANDBY);
	}
	static void midiCallback(midi_event *pev);
	static void sysexCallback(sysex_event *pev);
	void midiSilence(void);
	void midiNextEvent(void);
	void loadLoopMidi(const char*);
	void loadSingleMidi(const char*);

	char debugmessage[80];
	int state;
	MD_YX5300 *_mp3;  //pointer MP3
	MD_MIDIFile *_SMF; // pointer SMF Player

private:
	unsigned long audioMillis;
	bool standby;
	int statuscode;

protected:
	SdFat *_sd;

};

#endif /* AUDIO_H_ */
