/*
 * zLog.cpp
 *
 *  Created on: 01.07.2023
 *      Author: Alfred3
 */

#include "zLog.h"

zLog::zLog() {
	DateTime dateTime = DateTime(0, 1, 1, DateTime::SATURDAY, 0, 0, 0);
	_psd = nullptr;
	_puser = nullptr;
}

zLog::~zLog() {
	// TODO Auto-generated destructor stub
}

void zLog::begin(SdFat *psd, benutzer *puser) {
	_psd = psd;
	_puser = puser;
}

