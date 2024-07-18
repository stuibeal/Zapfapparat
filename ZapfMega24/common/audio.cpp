/*
 * audio.cpp
 *
 *  Created on: 21.06.2023
 *      Author: alfred3
 */

#include "audio.h"
#include "globalVariables.h"
#include "avr/pgmspace.h"

//#include "gemein.h"

/* Merke:
 * static Variablen müssen einmalig außerhalb der Klasse initialisiert werden,
 * weil sie wie extern variablen behandelt werden.
 * Static meint, werden nicht in alle Instanzen der Klasse eingebaut.
 * ist hier egal, gibt die klasse nur einmal als objekt.
 */
uint8_t audio::state = AUDIO_RESTART;
audio::mp3Dinge audio::mp3D;

//Class c_audio
audio::audio() :
		MD_YX5300(MP3Stream), MD_MIDIFile() {
	audioMillis = millis();
	_sd = nullptr;
	_mp3 = nullptr;
	_SMF = nullptr;
	state = AUDIO_RESTART;
	for (uint8_t i = 0; i < MAX_PLAYLIST_SONGS; i++) {
		playlistFolder[i] = 0;
		playlistSong[i] = 0;
	}
	mp3D.songsInPlayList = 0;
	mp3D.actualPlayListSong = 0;
	mp3D.standby = 0;
	mp3D.lastMp3Status = 0;
	mp3D.currentTrack = 0;
	mp3D.folderFiles = 0;
	mp3D.waiting = false;
}

audio::~audio() {
	//  Auto-generated destructor stub
}

void audio::starte(SdFat *pSD, MD_MIDIFile *pSMF, MD_YX5300 *pMp3) {
	//Überschreibt die Pointer mit den übergebenen Pointern
	_sd = pSD;
	_SMF = pSMF;
	_mp3 = pMp3;

	MIDI.begin(MIDI_SERIAL_RATE);

	_SMF->begin(_sd);
	_SMF->setMidiHandler(midiCallback);
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
	_mp3->setCallback(cbResponse);
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
//	sprintf(buf, "EOF:%d TC:%d MP3:%d SC:%d", _SMF->isEOF(),
//			(_SMF->getTrackCount() == 0), MD_YX5300::STS_FILE_END,
//			status->code);
	return ((status->code == MD_YX5300::STS_FILE_END)
			&& (_SMF->isEOF() || (_SMF->getTrackCount() == 0)));

}

void audio::pruefe() {
	unsigned long wartezeit = millis() - audioMillis;
	switch (state) {
	case AUDIO_ON: //Audio ist an
		if (wartezeit >= 12000) {
			_mp3->check();  //MP3 Player abfragen
			/**
			 * Wenn MP3 File End = true
			 * UND
			 * MIDI End of File (ausgespielt) ODER Trackcount == 0
			 */
			if ((mp3D.lastMp3Status == MD_YX5300::STS_FILE_END)
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
			if (mp3D.standby) {
				state = AUDIO_STANDBY;
			} else {
				state = AUDIO_ON;
			}
		}
		break;
	case AUDIO_STANDBY:
		if (!mp3D.standby) {
			_mp3->check();
			audioMillis = millis();
			state = AUDIO_ON;
		}
		break;
	} /*switch*/

	if (DEBUG_A) {
		sprintf(buf, "WZ:%lu S:%d 3:%u l%u/%u F%u S%u", wartezeit / 1000, state,
				mp3D.lastMp3Status, mp3D.actualPlayListSong,
				mp3D.songsInPlayList, playlistFolder[mp3D.actualPlayListSong],
				playlistSong[mp3D.actualPlayListSong]);
	}
	if (mp3D.playTheList) {
		_mp3->check();
		audioMillis = millis();
		if (mp3D.lastMp3Status == MD_YX5300::STS_FILE_END) {
			mp3D.actualPlayListSong++;
			if (mp3D.actualPlayListSong < mp3D.songsInPlayList) {
				mp3NextSongOnPlaylist();
				mp3D.playStatus = S_PLAYING;
			} else {
				mp3ClearPlaylist();
				state = AUDIO_ON;
				mp3D.playStatus = S_STOPPED;
				mp3D.playTheList = false;
			}
		}
	}
}

void audio::on() {
	if (state == AUDIO_OFF) {
		state = AUDIO_RESTART;
		pruefe();
	}
}

void audio::off() {
	strcpy(buf, "Audio soll aus sein-1--");
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
	mp3D.standby = stby;
}

void audio::mp3Play(uint8_t folder, uint8_t song) {
	on();
	_mp3->playSpecific(folder, song);
}

void audio::mp3AddToPlaylist(uint8_t folder, uint8_t song) {
	on();
	playlistFolder[mp3D.songsInPlayList] = folder;
	playlistSong[mp3D.songsInPlayList] = song;
	if (mp3D.songsInPlayList == 0) {
		mp3D.actualPlayListSong = 0;
		mp3PlaySongOnPlaylist(playlistFolder[mp3D.actualPlayListSong],
				playlistSong[mp3D.actualPlayListSong]);
	}
	mp3D.songsInPlayList++;

}

void audio::mp3ClearPlaylist(void) {
	for (uint8_t i = 0; i < MAX_PLAYLIST_SONGS; i++) {
		playlistFolder[i] = 0;
		playlistSong[i] = 0;
	}
	mp3D.songsInPlayList = 0;
}

void audio::mp3Pause() {
	if (mp3D.playStatus == S_PAUSED) {
		on();
		_mp3->playStart();
		mp3D.playStatus = S_PLAYING;
	} else {
		_mp3->playPause();
		mp3D.standby = 1;
		mp3D.playStatus = S_PAUSED;
	}
}

void audio::mp3Stop() {
	_mp3->playStop();
}

void audio::mp3NextSongOnPlaylist() {
	if (mp3D.actualPlayListSong < mp3D.songsInPlayList + 1) {
		mp3D.actualPlayListSong++;
	} else {
		mp3D.actualPlayListSong = 0;
	}
	mp3Play(playlistFolder[mp3D.actualPlayListSong],
			playlistSong[mp3D.actualPlayListSong]);
}

void audio::mp3PreviousSongOnPlaylist(void) {
	if (mp3D.actualPlayListSong > 0) {
		mp3D.actualPlayListSong--;
	} else {
		mp3D.actualPlayListSong = mp3D.songsInPlayList;
	}
	mp3Play(playlistFolder[mp3D.actualPlayListSong],
			playlistSong[mp3D.actualPlayListSong]);
}

void audio::mp3FillShufflePlaylist(uint8_t folder) {
	on();
	mp3ClearPlaylist();
	_mp3->playSpecific(folder, 1);
	_mp3->playPause();
	_mp3->queryFolderFiles(folder);
	delay(500);
	_mp3->check();
	mp3D.songsInPlayList = mp3D.folderFiles;
	if (mp3D.songsInPlayList > 0) {
		for (uint8_t i = 0; i < mp3D.songsInPlayList; i++) {
			playlistFolder[i] = folder;
			playlistSong[i] = i + 1; //lieder beginnen mit 1
		}
		shuffleArray();
	}
	mp3D.playTheList = true;
	mp3D.actualPlayListSong = 0;
	mp3Play(playlistFolder[mp3D.actualPlayListSong],
			playlistSong[mp3D.actualPlayListSong]);
	mp3D.playStatus = S_PLAYING;
}

void audio::mp3PlaySongOnPlaylist(uint8_t folder, uint8_t song) {
	if (mp3D.actualPlayListSong < mp3D.songsInPlayList) {
		mp3Play(folder, song);
	}
}

void audio::bing() {
	if (mp3D.playStatus == S_PLAYING) {
		mp3Pause();
		mp3D.pauseForMidi = true;
		midi_event ev;
		ev.size = 0;
		ev.data[ev.size++] = 0xd0;
		ev.data[ev.size++] = 127;
		ev.data[ev.size++] = 0;
		midiCallback(&ev);
		ev.size = 0;
		ev.data[ev.size++] = 0xC0;
		ev.data[ev.size++] = 10;
		ev.data[ev.size++] = 0;
		midiCallback(&ev);
		ev.size = 0;
		ev.data[ev.size++] = 0x90;
		ev.data[ev.size++] = 57;
		ev.data[ev.size++] = 127;
		midiCallback(&ev);
		delay(500);
		mp3Pause();
		midiSilence();

		mp3D.pauseForMidi = false;
	} else {
		mp3Play(20, 1);
	}
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

void audio::loadLoopMidi(const __FlashStringHelper *midiFile) {
	strcpy_P(buf, (char*) pgm_read_ptr(midiFile));
	loadLoopMidi(buf);
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

void audio::tickMetronome(void) {
	static uint32_t lastBeatTime = 0;
	static boolean inBeat = false;
	static uint8_t leuchtLampe = B00000001;
	uint16_t beatTime;

	beatTime = 60000 / _SMF->getTempo() / 4; // msec/beat = ((60sec/min)*(1000 ms/sec))/(beats/min)
	if (!inBeat) {
		if ((millis() - lastBeatTime) >= beatTime) {
			lastBeatTime = millis();

			inBeat = true;
			if (leuchtLampe & 1) {
				power.tastenLed(1, 255);
			} else {
				power.tastenLed(2, 255);
			}

			flowmeter.flowDataSend(LED_FUN_4, leuchtLampe, 0xFF);
			leuchtLampe++;

		}
	} else {
		if ((millis() - lastBeatTime) >= 100) // keep the flash on for 100ms only
				{

			if (!(leuchtLampe & 1)) {
				power.tastenLed(1, TASTEN_LED_NORMAL);
			} else {
				power.tastenLed(2, TASTEN_LED_NORMAL);
			}

			inBeat = false;

		}
	}
}

void audio::godModeSound(uint8_t godMode) {
	switch (godMode) {
	case IDDQD:
		loadLoopMidi(F("d_runni2.mid"));
		break;
	case KEEN:
		loadLoopMidi("keen.mid");
		break;
	case MAGNUM:
		loadLoopMidi("Magnum.mid");
		break;
	case MACGYVER:
		loadLoopMidi("Macgyver.mid");
		break;
	case MIAMI:
		loadLoopMidi("MiamiVice.mid");
		break;
	case SEINFELD:
		loadLoopMidi("Seinfeld.mid");
		break;
	case ALF:
		loadLoopMidi("ALF.mid");
		break;
	case COLT:
		loadLoopMidi("FALLGUY.mid");
		break;
	case DOTT:
		loadLoopMidi("Dott.mid");
		break;
	case INDY:
		loadLoopMidi("indy4Theme and Opening Credits.mid");
		break;
	case JUBI:
		loadLoopMidi("leaving_schlosskeller_bummbuumm.mid");
		break;

	}
}

void audio::shuffleArray() {
	randomSeed(analogRead(LICHT_SENSOR_PIN));
	uint8_t last = 0;
	uint8_t tempFolder = playlistFolder[last];
	uint8_t tempSong = playlistSong[last];
	for (uint8_t i = 0; i < mp3D.songsInPlayList; i++) {
		uint8_t index = random(mp3D.songsInPlayList);
		playlistFolder[last] = playlistFolder[index];
		playlistSong[last] = playlistSong[index];
		last = index;
	}
	playlistFolder[last] = tempFolder;
	playlistSong[last] = tempSong;
}

void audio::cbResponse(const MD_YX5300::cbData *status)
// Callback function used to process device unsolicited messages
// or responses to data requests
		{
	switch (status->code) {
	case MD_YX5300::STS_FILE_END:   // track has ended
		mp3D.lastMp3Status = status->code;
		mp3D.playStatus = S_STOPPED;
		break;

	case MD_YX5300::STS_PLAYING:   // current track index
		mp3D.currentTrack = status->data;
		break;

	case MD_YX5300::STS_FLDR_FILES:   // number of files in the folder
		mp3D.folderFiles = status->data;
		break;

	default:
		mp3D.lastMp3Status = status->code;
		break;
	}
	mp3D.waiting = false;
}

