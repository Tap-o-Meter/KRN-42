#ifndef LOGGER_H
#define LOGGER_H

#include "SD.h"
#include <Arduino.h>
#include <esp_wifi.h>
#include <MycilaWebSerial.h>


//Defining messages of ERRORS
#define ERR_NOT_SDCARD "No SD card found"

class Logger {
public:
    enum OutputType { HW_SERIAL, WEBSERIAL };

private:
    // BluetoothSerial BTSerial;
    enum ErrorType { NOT_SDCARD, NUM_ERRORS };
    bool theresSD = false;
    String message;
    const String errorMessages[NUM_ERRORS] = {ERR_NOT_SDCARD};

protected:

public:
    OutputType currentOutput;
    
    Logger();
    void init(unsigned long baudRate = 115200, uint16_t no_linea = 0);
    void setOutput(OutputType output);
    void printError(uint8_t errorType);
    void printError(const String &message);
    void print(const String &message);
    void println(const String &message);
    void printValue(const String &key, const String &value);
    bool available();
    long parseInt();
    float parseFloat();
    String readString();
    void setMessage(const String &msg);
    
};

extern Logger logger; 

#endif // LOGGER_H