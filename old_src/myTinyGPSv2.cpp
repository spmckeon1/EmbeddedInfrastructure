//
//  myTinyGPSv2.cpp
//  
//
//  Created by Stephen McKeon on 7/4/26.
//

#include "myConfig.h"
#ifdef HARDWARE_USES_GPS

#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <ArduinoTrace.h>
#include <commonItems_ESP32.h>
#include <myMQTT.h>

#include "myTinyGPSv2.h"

TinyGPSPlus gps;                                                                // The TinyGPS++ object

HardwareSerial gpsSerial(2);                                                    // Create an instance of the HardwareSerial class for Serial 2

String gpsPayload;
String GPS_DATA_PUBLISH_TOPIC = "NOT IN USE";


/*------------------ SETUP GPS ------------------*/

void gpsSetup() {
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  mySP("GPS initialized: TX: " + String(GPS_TX_PIN) + " GPS_RX_PIN: " +
       String(GPS_RX_PIN) + ", GPS Serial speed is: " + String(GPS_BAUD) +
       "\n", FN, LN);
}


/*------------------ HANDLE All GPS EVENTS ------------------*/

void gpsEvents(){
  static runTime isTimeInfo = {1, -1, CT_SECOND};                               // data struct for keeping track of when to send GPS data
  collectGpsData();
    if(isTimeToRun(&isTimeInfo)) {
      processGpsData();
      sendGpsData();
    }
}

/*------------------ TAKE CARE OF COLLECTING THE GPS DATA ------------------*/

void collectGpsData() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    // Uncomment for troubleshooting
    // Serial.write(c);
    gps.encode(c);
  }
}

/*------------------ CONVERT THE GPS DATA INTO A JSON OBJECT AND THEN A JSON STRING ------------------*/

void processGpsData() {
  JsonDocument doc;

    doc["location"]["lat"] = gps.location.lat();
    doc["location"]["lng"] = gps.location.lng();
    doc["fix"]["valid"] = gps.location.isValid();
    const uint32_t INVALID_AGE = (uint32_t)ULONG_MAX;
    uint32_t age = gps.location.age();
    if (age == INVALID_AGE) {
        age = 0;
    }
    doc["fix"]["age"] = age;
    doc["motion"]["speedKph"] = round(gps.speed.kmph() * 10.0) / 10.0;
    doc["motion"]["courseDeg"] = gps.course.deg();
    doc["altitude"]["meters"] = gps.altitude.meters();
    doc["quality"]["satellites"] = gps.satellites.value();
    doc["quality"]["hdop"] = gps.hdop.hdop();
    doc["utc"]["year"]   = gps.date.year();
    doc["utc"]["month"]  = gps.date.month();
    doc["utc"]["day"]    = gps.date.day();
    doc["utc"]["hour"]   = gps.time.hour();
    doc["utc"]["minute"] = gps.time.minute();
    doc["utc"]["second"] = gps.time.second();

    gpsPayload = "";
    serializeJson(doc, gpsPayload);
}

/*------------------ SEND THE GPS DATA TO MQTT ------------------*/

void sendGpsData() {
//  DUMP(gpsPayload);
  mqttPublishMsg(GPS_DATA_PUBLISH_TOPIC, 0, false, gpsPayload,LN);
 }

#endif // HARDWARE_USES_GPS
