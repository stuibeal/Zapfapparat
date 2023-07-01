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
#include "./libraries/RTC_DCF/DateTime.h"   // ELV RTC mit DCF
#include "./libraries/RTC_DCF/RealTimeClock_DCF.h"
#include "gemein.h"

class zLog {
public:
	zLog();
	virtual ~zLog();
	void begin(SdFat *psd, benutzer *puser);
	DateTime dateTime;
	//enum { LOG=0, DEBUG };

private:


protected:
	SdFat *_psd;
	benutzer *_puser;

};

#endif /* ZLOG_H_ */
