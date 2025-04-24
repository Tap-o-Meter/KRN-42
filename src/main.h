
#ifndef MAIN_H
#define MAIN_H

// Include necessary headers, especially if some of your function prototypes
// rely on types defined in these headers, such as JsonObject.
#include <Arduino.h>
#include <FS.h>
#include "WIFI.h"
#include "line.h"
#include "Reader.h"
#include "Screen.h"
#include <SPIFFS.h>
#include "Logger.h"
#include "secrets.h"
#include <esp_wifi.h>
#include "SocketComm.h"
#include <ArduinoJson.h>
#include <WiFiManager32.h>
#include "freertos/portmacro.h"
// Include other necessary headers (e.g., if you're using JsonObject, you might need to include the JSON library header)

// Function prototypes
void setUpWiFi();                                       // THIS SHOULD BE COMPLETELY MOVE TO WIFI.H
void bootOptions();                                                                          
void otaUpdating();                                     
void flowCounter();                                     // THIS CAN BE MOVED TO LINE.H
void lineUnlocked();                                    
uint16_t mermando();                                    // ALL THE LINE STUFF SHOULD BE MOVED TO LINE.H
uint16_t safeReadCounter();
void setUpSocketConnection();                           
void filling(uint16_t pulses);                          // ALL THE LINE STUFF SHOULD BE MOVED TO LINE.H
uint16_t countQty(bool calibrate);
void handleTouch(bool remote = false);
void socketManager(void * pvParameters);                // THIS SHOULD BE MOVED TO SOCKETCOMM.H
bool countQty(String screen_msg, uint16_t ml);          // ALL THE LINE STUFF SHOULD BE MOVED TO LINE.H
void SetConnectedScreen(bool retriable = false);        
JsonObject decodeJson(const char * payload);
bool validateJsonResponse(JsonObject json_response);    // THIS SHOULD BE MOVED TO SOCKETCOMM.H
void commitPurchase(String concept, String qty, String user = "");

// API Handlers
void event(const char * payload, size_t length);
void onConnect(const char * payload, size_t length);
void onClaimBeer(const char * payload, size_t length);
void onLineChange(const char * payload, size_t length);
void onDisconnect(const char * payload, size_t length);
void onRemoteSell(const char * payload, size_t length);
void onInfoRecived(const char * payload, size_t length);
void validateClient(const char * payload, size_t length);
void validateResponse(const char * payload, size_t length);
void onNewEmergencyCard(const char * payload, size_t length);
void onDisconnectedLine(const char * payload, size_t length);
void requestDevice(const char * payload, size_t length);

//Logger
void DEBUG(const char *message);
// void ERROR(ErrorType error);

#endif // MAIN_H