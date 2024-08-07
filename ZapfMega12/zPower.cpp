/*
 * zPower.cpp
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 */

#include "zPower.h"
#include "gemein.h"
#include "tempControl.h"

zPower::zPower ()
{
	inVoltage = 120;
	helligkeit =0 ;
	bkPowerState = BATT_NORMAL;
	bkMachineState = WORK;
	millisSeitZapfEnde = 0;
	millisSeitLetztemCheck = 0;
	_pTemp = nullptr;
}

zPower::~zPower ()
{
  // TODO Auto-generated destructor stub
}

void zPower::begin(tempControl *pTemp) {
	_pTemp = pTemp;
}

void zPower::check() {
   switch (_pTemp->getBatterieStatus()) {
   case 0x00:
   	   bkPowerState = BATT_ULTRAHIGH;
   	   break;
   case 0x01:
   	   bkPowerState = BATT_HIGH;
   	   break;
   case 0x02:
   	   bkPowerState = BATT_NORMAL;
   	   break;
   case 0x03:
   	   bkPowerState = BATT_LOW;
   	   break;
   case 0x04:
   	   bkPowerState = BATT_ULTRALOW;
   	   break;
   }

}
void zPower::setLed(uint8_t offon) {

}
