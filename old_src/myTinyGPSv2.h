//
//  myTinyGPSv2.h
//  
//
//  Created by Stephen McKeon on 7/4/26.
//

#ifndef __MY_TINY_GPS_V2_H__
#define __MY_TINY_GPS_V2_H__

#include "myConfig.h"
#ifdef HARDWARE_USES_GPS

#warning "IN MY_TINY_GPS_V2_H"


#define GPS_RX_PIN 4      // ESP32 RX2  <-- GPS TX (yellow wire)
#define GPS_TX_PIN 5      // ESP32 TX2  <-- GPS RX (white wire)
#define GPS_BAUD   38400


extern String GPS_DATA_PUBLISH_TOPIC;


//FUNCTION FORWARDS
extern void gpsSetup();
extern void gpsEvents();
extern void collectGpsData();
extern void processGpsData();
extern void sendGpsData();


#endif // HARDWARE_USES_GPS
#endif // __MY_TINY_GPS_V2_H__
