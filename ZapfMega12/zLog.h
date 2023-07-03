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
#include "benutzer.h"
#include "gemein.h"
#include "./zLibraries/RTC_DCF/DateTime.h"
#include "./zLibraries/RTC_DCF/RealTimeClock_DCF.h"


class zLog {
public:
	zLog();
	virtual ~zLog();
	void begin(SdFat *psd, benutzer *puser);
	DateTime dateTime;
	enum { LOG=0, DEBUG };
	enum wochadog {MODA, ERDA, MIGGA, PFINSDA, FREIDA, SAMSDA, SUNDA};



protected:
	SdFat *_psd;
	benutzer *_puser;

};

#endif /* ZLOG_H_ */
