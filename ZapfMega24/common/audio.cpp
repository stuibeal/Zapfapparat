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
	plMillis = millis();
	wartezeit = 0;
	plWartezeit = 0;
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
	mp3D.pauseForMidi = 0;
	mp3D.playTheList = 0;
	mp3D.playStatus = S_STOPPED;
}

audio::~audio() {
	//  Auto-generated destructor stub
}

void audio::stromAn() {
	pinMode(MIDI_RESET, OUTPUT);  //MIDI Reset needs to be ~1 sec!
	digitalWrite(MIDI_RESET, HIGH);
	pinMode(AUDIO_AMP, OUTPUT);
	pinMode(AUDIO_BOARD, OUTPUT);
	digitalWrite(AUDIO_BOARD, HIGH);
	delay(1000);
	digitalWrite(MIDI_RESET, LOW); // MIDI Reset off
	delay(600);
	digitalWrite(AUDIO_AMP, HIGH);

}

void audio::stromAus() {
	digitalWrite(MIDI_RESET, LOW);
	digitalWrite(AUDIO_BOARD, LOW);

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

	stromAn();
	//MP3, Audio

	//MP3 Player
	MP3Stream.begin(MD_YX5300::SERIAL_BPS);
	_mp3->begin();
	_mp3->setSynchronous(true);
	_mp3->setCallback(cbResponse);
	delay(500); //warten auf init
	//ZD.println ("MP3 Player ready...");

	_SMF->looping(false);
	mp3D.playStatus = S_STOPPED;

}

uint8_t audio::pruefe() {
	wartezeit = millis() - audioMillis;
	_mp3->check();
//	if (mp3D.lastMp3Status == MD_YX5300::STS_FILE_END) {
//		mp3D.playStatus = S_STOPPED;
//		mp3D.lastMp3Status = 0;
//	}

	switch (state) {
	case AUDIO_ON: //Audio ist an
		if (mp3D.playStatus == S_PLAYING) {
			audioMillis = millis();
		}
		if (mp3D.standby) {
			state = AUDIO_STANDBY;
		}
		if (mp3D.playTheList) {
			state = AUDIO_PLAYLIST;
		}
		if (wartezeit >= 12000) {
			/**
			 * Wenn MP3 File End = true
			 * UND
			 * MIDI End of File (ausgespielt) ODER Trackcount == 0
			 */
			if (mp3D.playStatus == S_STOPPED
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
		if (mp3D.standby) {
			state = AUDIO_RESTART;
		}
		break;

	case AUDIO_RESTART: // audio soll wieder an sein
		digitalWrite(MIDI_RESET, HIGH); //midi Reset einschalten
		digitalWrite(AUDIO_BOARD, HIGH);
		audioMillis = millis();
		state = AUDIO_MIDI_RESET;
		break;

	case AUDIO_MIDI_RESET:
		if (wartezeit > MIDI_RESET_WARTEZEIT) {
			digitalWrite(MIDI_RESET, LOW);
			audioMillis = millis();
			state = AUDIO_AMP_ON;
		}
		break;

	case AUDIO_AMP_ON:
		if (wartezeit > AUDIO_ON_WARTEZEIT) {
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
		audioMillis = millis();
		if (!mp3D.standby) {
			audioMillis = millis();
			state = AUDIO_ON;
		}
		break;
	case AUDIO_PLAYLIST:
		audioMillis= millis();
		checkPlayList();
		break;
	} /*switch*/

	return state;
}

void audio::checkPlayList() {
	if (mp3D.playStatus == S_STOPPED) {
		plWartezeit = millis() - plMillis;
	}
	if (mp3D.playStatus == S_STOPPED && mp3D.oldPlayStatus == S_PLAYING)
		mp3D.oldPlayStatus = S_STOPPED;
	/*|| mp3D.lastMp3Status == MD_YX5300::STS_VERSION)*/
	{
		if (mp3D.actualPlayListSong < mp3D.songsInPlayList) {
			mp3NextSongOnPlaylist();
		}
		if (mp3D.actualPlayListSong == mp3D.songsInPlayList) {
			plMillis = millis();
		}
	}
	if (mp3D.playStatus == S_STOPPED && plWartezeit > 10000) {
		mp3ClearPlaylist();
		state = AUDIO_ON;
		mp3D.playTheList = false;
	}
}

void audio::on() {
	if (state == AUDIO_OFF) {
		state = AUDIO_RESTART;
		pruefe();
	}
}

void audio::off() {
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
	mp3D.playStatus = S_PLAYING;
	pruefe();
	mp3D.lastMp3Status = 0;
//	while (pruefe() != AUDIO_ON && pruefe() != AUDIO_STANDBY) {
//	}
	_mp3->playSpecific(folder, song);

}

void audio::mp3PlayAndWait(uint8_t folder, uint8_t song) {
	on();
	mp3D.playStatus = S_PLAYING;
	pruefe();
	mp3D.lastMp3Status = 0;
	mp3Play(folder, song);
	do {
		sound.pruefe();
		if (DEBUG_A) {
			ZD.infoText(0, buf);
			delay(200);
		}
	} while (mp3D.playStatus == S_PLAYING);
}

void audio::mp3AddToPlaylist(uint8_t folder, uint8_t song) {
	//on();
	mp3D.songsInPlayList++;
	playlistFolder[mp3D.songsInPlayList] = folder;
	playlistSong[mp3D.songsInPlayList] = song;
	if (mp3D.songsInPlayList == 1) {
		mp3D.actualPlayListSong = 1;
		mp3Play(playlistFolder[mp3D.actualPlayListSong],
				playlistSong[mp3D.actualPlayListSong]);
		mp3D.playTheList = true;
	}

}

void audio::mp3ClearPlaylist(void) {
	for (uint8_t i = 0; i < MAX_PLAYLIST_SONGS; i++) {
		playlistFolder[i] = 0;
		playlistSong[i] = 0;
	}
	mp3D.songsInPlayList = 0;
	mp3D.actualPlayListSong = 0;
	mp3D.playTheList = 0;
	mp3D.standby = 0;
	_mp3->playStop();
	mp3D.playStatus = S_STOPPED;
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
	mp3D.playStatus = S_STOPPED;
}

void audio::mp3NextSongOnPlaylist() {
	if (mp3D.actualPlayListSong < mp3D.songsInPlayList) {
		mp3D.actualPlayListSong++;
	} else {
		mp3D.actualPlayListSong = 1;
	}
	mp3Play(playlistFolder[mp3D.actualPlayListSong],
			playlistSong[mp3D.actualPlayListSong]);
}

void audio::mp3PreviousSongOnPlaylist(void) {
	if (mp3D.actualPlayListSong > 1) {
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
	delay(500);
	_mp3->check();
	mp3D.songsInPlayList = mp3D.folderFiles;
	if (mp3D.songsInPlayList > 0) {
		for (uint8_t i = 1; i < mp3D.songsInPlayList + 1; i++) {
			playlistFolder[i] = folder;
			playlistSong[i] = i + 1; //lieder beginnen mit 1
		}
		shuffleArray();
		mp3D.playTheList = true;
		mp3D.actualPlayListSong = 1;
		mp3Play(playlistFolder[mp3D.actualPlayListSong],
				playlistSong[mp3D.actualPlayListSong]);
	}
}

void audio::mp3PlaySongOnPlaylist(uint8_t folder, uint8_t song) {
	if (mp3D.actualPlayListSong < mp3D.songsInPlayList) {
		mp3Play(folder, song);
		mp3D.playTheList = true;
		mp3D.playStatus = S_PLAYING;

	}
}

void audio::bing() {
	if (mp3D.playTheList) {
		mp3Pause();
		mp3D.pauseForMidi = true;
		midi_event ev;
		ev.size = 0;
		ev.data[ev.size++] = 0xD9; // channel pressure
		ev.data[ev.size++] = 127;
		ev.data[ev.size++] = 0;
		midiCallback(&ev);
//		ev.size = 0;
//		ev.data[ev.size++] = 0xC9; //hex: C: change instrument 9: channel 10
//		ev.data[ev.size++] = 5;
//		ev.data[ev.size++] = 0;
//		midiCallback(&ev);
//		ev.size = 0;
//		ev.data[ev.size++] = 0xB9; //hex: B: controller A: channel 10
//		ev.data[ev.size++] = 91; //Reverb
//		ev.data[ev.size++] = 100;
//		midiCallback(&ev);
		ev.size = 0;
		ev.data[ev.size++] = 0x99; //HEX 09: note on /9: channel 10
		ev.data[ev.size++] = 57; //PITCH (note number)
		ev.data[ev.size++] = 127; //velocity
		midiCallback(&ev);
		delay(100);
		ev.size = 0;
		ev.data[ev.size++] = 0x99; //HEX 09: note on /9: channel 10
		ev.data[ev.size++] = 50; //PITCH (note number)
		ev.data[ev.size++] = 127; //velocity
		midiCallback(&ev);

		delay(1000);
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
	strcpy_P(buf, (const char*) midiFile);
	loadLoopMidi(buf);
}

void audio::loadLoopMidi(const char *midiFile) {
	_SMF->close();
	uint8_t status = _SMF->load(midiFile);
	if (status != 0) {
		sprintf_P(buf, PSTR("MIDI Filestatus: %d"), status);
	}
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
		loadLoopMidi(F("/midi/d_runni2.mid"));
		break;
	case IDKFA:
		loadLoopMidi(F("/midi/E2M3.mid"));
		break;
	case IDCLEV:
		loadLoopMidi(F("/midi/d_e1m1.mid"));
		break;
	case KEEN:
		loadLoopMidi(F("/midi/keen.mid"));
		break;
	case MAGNUM:
		loadLoopMidi(F("/midi/Magnum.mid"));
		break;
	case MACGYVER:
		loadLoopMidi(F("/midi/Macgyver6.mid"));
		break;
	case MIAMI:
		loadLoopMidi(F("/midi/MiamiVice.mid"));
		break;
	case SEINFELD:
		loadLoopMidi(F("/midi/Seinfeld.mid"));
		break;
	case ALF:
		loadLoopMidi(F("/midi/ALF.mid"));
		break;
	case COLT:
		loadLoopMidi(F("/midi/FALLGUY.mid"));
		break;
	case DOTT:
		loadLoopMidi(F("/midi/Tdisco.mid"));
		break;
	case INDY:
		loadLoopMidi(F("/midi/indy4Theme and Opening Credits.mid"));
		break;
	case JUBI:
		loadLoopMidi(F("/midi/leaving_schlosskeller_bummbuumm.mid"));
		break;
	case GIANNA:
		loadLoopMidi(F("/midi/meravigl(1).mid"));
		break;
	case LIGABUE:
		loadLoopMidi(F("/midi/ilgiornodeigiorni.mid"));
		break;
	case GLORIA:
		loadLoopMidi(F("/midi/Gloria 1999(k).mid"));
		break;
	case VAGABONDO:
		loadLoopMidi(F("/midi/Io_vagabondo.mid"));
		break;
	case DESCENT:
		loadLoopMidi(F("/midi/grabbag.mid"));
	}
}

void audio::shuffleArray() {
	randomSeed(analogRead(LICHT_SENSOR_PIN));
	uint8_t last = 1;
	uint8_t tempFolder = playlistFolder[last];
	uint8_t tempSong = playlistSong[last];
	for (uint8_t i = 1; i < mp3D.songsInPlayList; i++) {
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
		mp3D.playStatus = S_PLAYING;
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

