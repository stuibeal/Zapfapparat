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
#define MAX_PLAYLIST_SONGS 100

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
	void stromAn(void);
	void stromAus(void);
	void starte(SdFat *pSD, MD_MIDIFile *pSMF, MD_YX5300 *mp3); //Mit Pointer starten
	uint8_t pruefe();
	bool pruefePlaying();
	void on(void);
	void off(void);
	void midiReset(void);
	void setStandby(bool stby);
	void mp3Play(uint8_t folder, uint8_t song);
	void mp3PlayAndWait(uint8_t folder, uint8_t song);
	void mp3AddToPlaylist(uint8_t folder, uint8_t song);
	void mp3ClearPlaylist(void);
	void mp3Pause();
	void mp3Stop();
	void mp3NextSongOnPlaylist(void);
	void mp3PreviousSongOnPlaylist(void);
	void mp3FillShufflePlaylist(uint8_t folder);
	void mp3PlaySongOnPlaylist(uint8_t folder, uint8_t song);
	void bing();
	inline bool isOn() {
		return (state == AUDIO_ON || state == AUDIO_STANDBY);
	}
	static void midiCallback(midi_event *pev);
	void midiSilence(void);
	void midiNextEvent(void);
	void loadLoopMidi(const __FlashStringHelper *midiFile);
	void loadLoopMidi(const char*);
	void loadSingleMidi(const char*);
	void tickMetronome(void);
	void godModeSound(uint8_t godMode);

	inline uint8_t getPlaylistPlace(void) {
		return mp3D.actualPlayListSong;
	}
	inline uint8_t getPlaylistSize(void) {
		return mp3D.songsInPlayList;
	}
	inline uint8_t getPlFolder(void) {
		return playlistFolder[mp3D.actualPlayListSong];
	}
	inline uint8_t getPlSong(void) {
		return playlistSong[mp3D.actualPlayListSong];
	}

	static uint8_t state;
	MD_YX5300* _mp3;  //pointer MP3
	MD_MIDIFile* _SMF; // pointer SMF Player

	uint8_t playlistFolder[MAX_PLAYLIST_SONGS];
	uint8_t playlistSong[MAX_PLAYLIST_SONGS];

	enum playStatus_t { S_PAUSED, S_PLAYING, S_STOPPED };

	struct mp3Dinge {
		bool standby;
		bool waiting;
		bool playTheList;
		bool pauseForMidi;
		playStatus_t playStatus;
		uint16_t currentTrack;
		uint16_t folderFiles;
		uint16_t lastMp3Status;
		uint8_t songsInPlayList;
		uint8_t actualPlayListSong;
	};
	static mp3Dinge mp3D;


private:
	unsigned long audioMillis;
	void shuffleArray();
	static void cbResponse(const MD_YX5300::cbData *status);

protected:
	SdFat *_sd;

};

#endif /* AUDIO_H_ */
