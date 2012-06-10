/*
 * TidePortSanLuis.h
 * A library for calculating the current tide height at
 * Port San Luis, CA.
 *  Created on: Jun 10, 2012
 *      Author: millerlp
 */

#ifndef TidePortSanLuis_h
#define TidePortSanLuis_h

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

class TideCalc {
 public:
	TideCalc();
    float currentTide(DateTime now);
};

#endif
