/*
 * zLog.h
 *
 *  Created on: 01.07.2023
 *      Author: Alfred3
 *
 *      TODO: Methoden
 *      - log Zapfung
 *      - log ab und zu (voltage etc)
 *      - print Debug msg (on off var!)
 *      - DCF hier integrieren
 *
 */

#ifndef ZLOG_H_
#define ZLOG_H_

#include "SdFat.h"
#include "./common/benutzer.h"
#include "./common/gemein.h"
#include "./zLibraries/RTC_DCF/DateTime.h"
#include "./zLibraries/RTC_DCF/RealTimeClock_DCF.h"
#include "tempsens.h"
#include "string.h"

class zLog : public RealTimeClock_DCF, DateTime
{
public:
  zLog ();
  virtual
  ~zLog ();
  void
  initialise (SdFat *psd, benutzer *puser, tempsens *ptemp, char *buf);
  inline void
  setLogState(uint8_t state) {logState = state;}
  inline uint8_t
  getlogState() {return logState;}
  void
  getClockString(void);


  DateTime dateTime;

  enum logstate
  {
    LOG = 0, DEBUG
  };
  enum wochadog
  {
    MODA, ERDA, MIGGA, PFINSDA, FREIDA, SAMSDA, SUNDA
  };


private:
  uint8_t logState;
  char * _buf;


protected:
  SdFat *_sd;
  benutzer *_user;
  tempsens *_temp;


};

#endif /* ZLOG_H_ */
