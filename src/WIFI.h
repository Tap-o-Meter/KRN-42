#ifndef MY_WIFI_H
#define MY_WIFI_H
#include "Logger.h"
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Arduino.h>
#include <WiFiMulti.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <MycilaWebSerial.h>

#define RETRY_TIME 2000
#define WIFI_RETRIES_ADD 65

#define ID String(ESP.getEfuseMac(), HEX)

class WIFI {
  public:
    const String ap_name = "Line_"+ID;
    String getIP();
    void stopOTA();
    void loopOTA();
    void startMDNS();
    void eneableAP();
    bool isConnected();
    bool theresValidSSID();
    void resetWiFiSettings();
    void setUpOTA(uint8_t tap_number);
    bool setUpWiFi(uint8_t max_tries = 3, const char *ssid = NULL, const char *passphrase = NULL);
    void reconnect(bool not_blocking = false);
    void setUpWebServer(bool serial = false);
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