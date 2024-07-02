/*
 * audio.cpp
 *
 *  Created on: 21.06.2023
 *      Author: alfred3
 */

#include "audio.h"
//#include "gemein.h"

//Class c_audio
audio::audio() : MD_YX5300(MP3Stream), MD_MIDIFile()
{
	audioMillis = millis();
	_sd = nullptr;
	_mp3 = nullptr;
	_SMF = nullptr;
	state = AUDIO_RESTART;
	standby = 0;
	statuscode = 0;
}

audio::~audio() {
	//  Auto-generated destructor stub
}

void audio::starte(SdFat *pSD, MD_MIDIFile * pSMF, MD_YX5300 * pMp3) {
	//Überschreibt die Pointer mit den übergebenen Pointern
	_sd = pSD;
	_SMF = pSMF;
	_mp3 = pMp3;

	MIDI.begin(MIDI_SERIAL_RATE);

	_SMF->begin(_sd);
	_SMF->setMidiHandler(midiCallback);
	_SMF->setSysexHandler(sysexCallback);
	_SMF->setFileFolder("/midi/");
	_SMF->looping(true);

	//MP3, Audio
	//ZD.println ("Starte ROLAND SCB-7 daughterboard");
	pinMode(MIDI_RESET, OUTPUT);  //MIDI Reset needs to be ~1 sec!
	digitalWrite(MIDI_RESET, HIGH);

	//ZD.println ("Starte Audio Amplificatrice");
	pinMode(AUDIO_AMP, OUTPUT);
	pinMode(AUDIO_BOARD, OUTPUT);

	digitalWrite(AUDIO_BOARD, HIGH);

	//MP3 Player
	MP3Stream.begin(MD_YX5300::SERIAL_BPS);
	_mp3->begin();
	_mp3->setSynchronous(true);
	delay(500); //warten auf init
	//ZD.println ("MP3 Player ready...");

	digitalWrite(MIDI_RESET, LOW); // MIDI Reset off
	delay(600);
	digitalWrite(AUDIO_AMP, HIGH);
	//ZD.println ("Harte Musik bereit");
	//_SMF->load("Ein-Prosit-1.mid");
	_SMF->looping(false);

}
bool audio::pruefePlaying() {
	_mp3->check();  //MP3 Player abfragen
	const MD_YX5300::cbData *status = _mp3->getStatus(); //statuspointer holen
	sprintf(debugmessage, "EOF:%d TC:%d MP3:%d SC:%d", _SMF->isEOF(),
			(_SMF->getTrackCount() == 0), MD_YX5300::STS_FILE_END,
			status->code);
	return ((status->code == MD_YX5300::STS_FILE_END)
			&& (_SMF->isEOF() || (_SMF->getTrackCount() == 0)));

}

void audio::pruefe() {
	unsigned long wartezeit = millis() - audioMillis;
	switch (state) {
	case AUDIO_ON: //Audio ist an
		if (wartezeit >= 12000) {
			_mp3->check();  //MP3 Player abfragen
			const MD_YX5300::cbData *status = _mp3->getStatus(); //statuspointer holen
			statuscode = status->code;
			/**
			 * Wenn MP3 File End = true
			 * UND
			 * MIDI End of File (ausgespielt) ODER Trackcount == 0
			 */
			if ((status->code == MD_YX5300::STS_FILE_END)
					&& (_SMF->isEOF() || (_SMF->getTrackCount() == 0))) {
				digitalWrite(AUDIO_AMP, LOW); //AMP aus
				audioMillis = millis();
				state = AUDIO_SHUTDOWN;
			}
		}
		break;

	case AUDIO_SHUTDOWN:
		if (wartezeit >= AUDIO_WARTEZEIT) {
			digitalWrite(AUDIO_BOARD, LOW);  //board (midi usw) aus
			audioMillis = millis();
			state = AUDIO_OFF;
		}
		break;

	case AUDIO_OFF:
		// do nothing?
		break;

	case AUDIO_RESTART: // audio soll wieder an sein
		_mp3->check();
		digitalWrite(MIDI_RESET, HIGH); //midi Reset einschalten
		digitalWrite(AUDIO_BOARD, HIGH);
		audioMillis = millis();
		state = AUDIO_MIDI_RESET;
		break;

	case AUDIO_MIDI_RESET:
		if (wartezeit > MIDI_RESET_WARTEZEIT) {
			_mp3->check();
			digitalWrite(MIDI_RESET, LOW);
			audioMillis = millis();
			state = AUDIO_AMP_ON;
		}
		break;

	case AUDIO_AMP_ON:
		if (wartezeit > AUDIO_ON_WARTEZEIT) {
			_mp3->check();
			digitalWrite(AUDIO_AMP, HIGH);
			audioMillis = millis();
			if (standby) {
				state = AUDIO_STANDBY;
			} else {
				state = AUDIO_ON;
			}
		}
		break;
	case AUDIO_STANDBY:
		if (!standby) {
			_mp3->check();
			audioMillis = millis();
			state = AUDIO_ON;
		}
	}

	if (DEBUG_A) {
		/*
		 const MD_YX5300::cbData *status = _mp3->getStatus(); //statuspointer holen
		 sprintf(debugmessage, "EOF:%u TC:%u SC:%u WZ:%1u S:%u  ", _SMF->isEOF(),
		 (_SMF->getTrackCount() == 0),
		 status->code, wartezeit,state);
		 */
		sprintf(debugmessage, "WZ: %lu S: %d c: %u", wartezeit, state,
				statuscode);
	}

}

void audio::on() {
	if (state == AUDIO_OFF) {
		state = AUDIO_RESTART;
		pruefe();
	}
}

void audio::off() {
	strcpy(debugmessage, "Audio soll aus sein-1--");
	digitalWrite(AUDIO_AMP, LOW); //AMP aus
	audioMillis = millis();
	state = AUDIO_SHUTDOWN;
	pruefe();
}

void audio::midiReset() {
	digitalWrite(MIDI_RESET, HIGH);
	delay(1000);
	digitalWrite(MIDI_RESET, LOW);
}

void audio::setStandby(bool stby) {
	standby = stby;
}

void audio::mp3Play(uint8_t folder, uint8_t song) {
	on();
	_mp3->playSpecific(folder, song);
}

void audio::bing() {
	on();
	_mp3->playTrack(DING); //BING!
}

void audio::midiCallback(midi_event *pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
		{
	if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0)) {
		MIDI.write(pev->data[0] | pev->channel);
		MIDI.write(&pev->data[1], pev->size - 1);
	} else
		MIDI.write(pev->data, pev->size);
}

void audio::sysexCallback(sysex_event *pev)
// Called by the MIDIFile library when a system Exclusive (sysex) file event needs
// to be processed through the midi communications interface. Most sysex events cannot
// really be processed, so we just ignore it here.
// This callback is set up in the setup() function.
		{
	//DEBUG("\nS T", pev->track);
	//DEBUGS(": Data");
	//for (uint8_t i=0; i<pev->size; i++)
	//  DEBUGX(" ", pev->data[i]);
}

void audio::midiSilence(void)
// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the not#includees and sound
		{
	midi_event ev;

	// All sound off
	// When All Sound Off is received all oscillators will turn off, and their volume
	// envelopes are set to zero as soon as possible.
	ev.size = 0;
	ev.data[ev.size++] = 0xb0;
	ev.data[ev.size++] = 120;
	ev.data[ev.size++] = 0;

	for (ev.channel = 0; ev.channel < 16; ev.channel++)
		midiCallback(&ev);
}

void audio::midiNextEvent(void) {
	if (isOn()) {
		if (!_SMF->isEOF()) {
			_SMF->getNextEvent(); // Play MIDI data
		}
	}
	if (_SMF->isEOF()) {
		_SMF->close();
		midiSilence();
	}

}

void audio::loadLoopMidi(const char *midiFile) {
	_SMF->close();
	_SMF->load(midiFile);
	_SMF->looping(true);
	_SMF->pause(true);
}

void audio::loadSingleMidi(const char *midiFile) {
	_SMF->close();
	_SMF->load(midiFile);
	_SMF->looping(false);
	_SMF->pause(true);
}

