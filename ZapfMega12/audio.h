/*
 * audio.h
 *
 *  Created on: 21.06.2023
 *      Author: alfred3
 */

#ifndef AUDIO_H_
#define AUDIO_H_
#include "Arduino.h"
#include "gemein.h"
#include "MD_MIDIFile.h"
#include "MD_YX5300.h"
#include "string.h"

#ifndef DEBUG_A
#define DEBUG_A 0
#endif

class audio {


public:
	static const long  MIDI_RESET_WARTEZEIT = 400;

		static const long AUDIO_WARTEZEIT = 500;
		static const long AUDIO_STANDBYZEIT = 30000;
		static const int AUDIO_ON = 0;
		static const int AUDIO_SHUTDOWN = 1;
		static const int AUDIO_OFF = 2;
		static const int AUDIO_RESTART = 3;
		static const int AUDIO_MIDI_RESET = 4;
		static const int AUDIO_AMP_ON = 5;
		static const int AUDIO_STANDBY = 6;

	audio();
	virtual ~audio();
	void starte(MD_MIDIFile *pSMF, MD_YX5300 *pmp3);
	void pruefe();
	bool pruefePlaying();
	void on(void);
	void off(void);
	void midiReset(void);
	void setStandby(bool stby);
	inline bool isOn() {return (state == AUDIO_ON || state == AUDIO_STANDBY);}
	char debugmessage[80];
	uint8_t state;
private:
	unsigned long audioMillis;
	bool standby;


protected:
	MD_YX5300 *_mp3;  //pointer MP3
	MD_MIDIFile *_SMF; // pointer SMF Player




};

#endif /* AUDIO_H_ */
