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
 *
 */

#ifndef ZLOG_H_
#define ZLOG_H_

#include "SdFat.h"
#include "benutzer.h"

class zLog {
public:
	zLog();
	virtual ~zLog();
	void begin(SDFAT *psd, benutzer *puser);

protected:
	SDFAT *_psd;
	benutzer *_puser;

};

#endif /* ZLOG_H_ */
