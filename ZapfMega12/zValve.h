/*
 * zValve.h
 *
 *  Created on: 07.07.2023
 *      Author: alfred3
 *
 *      macht die Valve auf und zu
 *      checkt den Druck
 */

#ifndef ZVALVE_H_
#define ZVALVE_H_
#include "gemein.h"

class zValve
{
public:
  zValve ();
  virtual
  ~zValve ();
  void begin ();
  inline uint8_t getState() {return state;}
  enum valveState
  {
    ZU, MACHT_AUF, AUF, MACHT_ZU, BISSL_AUF, BISSL_ZU
  };

private:
  uint8_t state;
  static unsigned long valveMillis;

};


#endif /* ZVALVE_H_ */
