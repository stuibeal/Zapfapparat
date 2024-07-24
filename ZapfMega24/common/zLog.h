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

#include <Arduino.h>
#include "SdFat.h"
#include "benutzer.h"
#include "gemein.h"
#include "DateTime.h"
#include "RealTimeClock_DCF.h"
#include "tempControl.h"
#include "string.h"

class zLog: public RealTimeClock_DCF, DateTime {
public:
	zLog();
	virtual ~zLog();
	void initialise(SdFat *psd, benutzer *puser, tempControl *ptemp, char *buf);
	inline void setLogState(uint8_t state) {
		logState = state;
	}
	inline uint8_t getlogState() {
		return logState;
	}
	void getClockString(void);
	void getClockBarcode(void);
	uint8_t getWochadog(void);
	void setDcfLed(bool onoff);
	void zLog::writeDataInBuf(uint16_t data, bool komma);
	bool logAfterZapf(void);

	DateTime dateTime;

	enum logstate {
		LOG = 0, DEBUG
	};
	enum wochadog {
		MODA, ERDA, MIGGA, PFINSDA, FREIDA, SAMSDA, SUNDA
	};

private:
	uint8_t logState;
	char *_buf;

protected:
	SdFat *_sd;
	benutzer *_user;
	tempControl *_temp;

};

#endif /* ZLOG_H_ */
