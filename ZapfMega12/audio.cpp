/*
 * audio.cpp
 *
 *  Created on: 21.06.2023
 *      Author: alfred3
 */

#include "audio.h"

//Class c_audio
audio::audio ()
{
  audioMillis = millis ();
  _mp3 = nullptr;
  _SMF = nullptr;
  state = 0;
}

audio::~audio ()
{
  //  Auto-generated destructor stub
}

void
audio::starte (MD_MIDIFile *pSMF, MD_YX5300 *pmp3)
{
  _mp3 = pmp3;
  _SMF = pSMF;
}
bool
audio::pruefePlaying ()
{
  _mp3->check ();  //MP3 Player abfragen
  const MD_YX5300::cbData *status = _mp3->getStatus (); //statuspointer holen
  sprintf (debugmessage, "EOF:%d TC:%d MP3:%d SC:%d", _SMF->isEOF (),
	   (_SMF->getTrackCount () == 0), MD_YX5300::STS_FILE_END,
	   status->code);
  return ((status->code == MD_YX5300::STS_FILE_END)
      && (_SMF->isEOF () || (_SMF->getTrackCount () == 0)));

}

void
audio::pruefe ()
{
  unsigned int wartezeit = millis () - audioMillis;
  switch (state)
    {
    case AUDIO_ON: //Audio ist an
      if (wartezeit > AUDIO_STANDBYZEIT)
	{
	  _mp3->check ();  //MP3 Player abfragen
	  const MD_YX5300::cbData *status = _mp3->getStatus (); //statuspointer holen
	  /**
	   * Wenn MP3 File End = true
	   * UND
	   * MIDI End of File (ausgespielt) ODER Trackcount == 0
	   */
	  if ((status->code == MD_YX5300::STS_FILE_END)
	      && (_SMF->isEOF () || (_SMF->getTrackCount () == 0)))
	    {
	      //strcpy(debugmessage, "Audio soll aus sein-1--");

	      digitalWrite (AUDIO_AMP, LOW); //AMP aus
	      audioMillis = millis ();
	      state = AUDIO_SHUTDOWN;
	    }
	}
      break;

    case AUDIO_SHUTDOWN:
      if (wartezeit > AUDIO_WARTEZEIT)
	{
	  digitalWrite (AUDIO_BOARD, LOW);  //board (midi usw) aus
	  //strcpy(debugmessage, "Audio aus---");
	  audioMillis = millis ();
	  state = AUDIO_OFF;
	}
      break;

    case AUDIO_OFF:
      // do nothing?
      break;

    case AUDIO_RESTART: // audio soll wieder an sein
      _mp3->check ();
      //strcpy(debugmessage, "Audio soll an sein---");
      digitalWrite (MIDI_RESET, HIGH); //midi Reset einschalten
      digitalWrite (AUDIO_BOARD, HIGH);
      audioMillis = millis ();
      state = AUDIO_MIDI_RESET;
      break;

    case AUDIO_MIDI_RESET:
      if (wartezeit > MIDI_RESET_WARTEZEIT)
	{
	  _mp3->check ();
	  digitalWrite (MIDI_RESET, LOW);
	  audioMillis = millis ();
	  state = AUDIO_AMP_ON;
	}
      break;

    case AUDIO_AMP_ON:
      if (wartezeit > AUDIO_WARTEZEIT)
	{
	  _mp3->check ();
	  digitalWrite (AUDIO_AMP, HIGH);
	  //strcpy(debugmessage, "Audio an---");
	  audioMillis = millis ();
	  if (standby)
	    {
	      state = AUDIO_STANDBY;
	    }
	  else
	    {
	      state = AUDIO_ON;
	    }
	}
      break;
    case AUDIO_STANDBY:
      if (!standby)
	{
	  _mp3->check ();
	  audioMillis = millis ();
	  state = AUDIO_ON;
	}
    }

  if (DEBUG_A)
    {
      /*
       const MD_YX5300::cbData *status = _mp3->getStatus(); //statuspointer holen
       sprintf(debugmessage, "EOF:%u TC:%u SC:%u WZ:%1u S:%u  ", _SMF->isEOF(),
       (_SMF->getTrackCount() == 0),
       status->code, wartezeit,state);
       */
      sprintf (debugmessage, "WZ: %5d S: %d", wartezeit, state);
    }

}

void
audio::on ()
{
  if (state == AUDIO_OFF)
    {
      state = AUDIO_RESTART;
      pruefe ();
    }
}

void
audio::off ()
{
  strcpy (debugmessage, "Audio soll aus sein-1--");
  digitalWrite (AUDIO_AMP, LOW); //AMP aus
  audioMillis = millis ();
  state = AUDIO_SHUTDOWN;
  pruefe ();
}

void
audio::midiReset ()
{
  digitalWrite (MIDI_RESET, HIGH);
  delay (1000);
  digitalWrite (MIDI_RESET, LOW);
}

void
audio::setStandby (bool stby)
{
  standby = stby;
}

void
audio::mp3Play (int folder, int song)
{

}

void
audio::bing ()
{
  _mp3->playTrack(1); //BING!
}
