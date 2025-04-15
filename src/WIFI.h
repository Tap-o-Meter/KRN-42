#ifndef MY_WIFI_H
#define MY_WIFI_H
#include "EEPROM.h"
#include "Logger.h"
#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFiMulti.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
// #include <WiFiUdp.h>
// #include "secrets.h"

#define RETRY_TIME 2000
#define WIFI_RETRIES_ADD 65

class WIFI {
  public:
    const String ap_name = "Line_"+String((uint32_t)ESP.getEfuseMac(), HEX);
    String getIP();
    void stopOTA();
    void loopOTA();
    void startMDNS();
    bool isConnected();
    String macAddress();
    bool theresValidSSID();
    void resetWiFiSettings();
    void setUpOTA(uint8_t tap_number);
    bool setUpWiFi(uint8_t max_tries = 3);
    void reconnect(bool not_blocking = false);
    // void localIP();
    // bool refreshWiFiStatus();
    // bool getConnectionStatus();
  private:
    uint32_t retry_connection_in = 0;
    Preferences preferences;
    // bool last_connection_state = false;

    //Logger
    void DEBUG(const char *message);
    // void ERROR(ErrorType error);
};
#endif