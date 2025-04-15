#include "Logger.h"

Logger logger; 

Logger::Logger() : currentOutput(HW_SERIAL) {} // Inicializa con SERIAL por defecto

void Logger::init(unsigned long baudRate, uint16_t no_linea) {
    if (currentOutput == HW_SERIAL) {
        Serial2.begin(baudRate);
        Serial2.setDebugOutput(false);
        Serial2.setDebugOutput(true);
    } else {
        BTSerial.begin("Tap-o-Meter_"+no_linea);

        esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, 4, (uint8_t*)"235813");
  
        // Configuramos el modo de escaneo:
        // ESP_BT_CONNECTABLE: permite que el dispositivo acepte conexiones.
        // ESP_BT_NON_DISCOVERABLE: lo hace invisible en los escaneos.
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE);
    }
}

void Logger::setOutput(OutputType output) {
    currentOutput = output;
}

void Logger::print(const String &message) {
    if (currentOutput == HW_SERIAL) {
        Serial2.print(message);
    } else {
        BTSerial.print(message);
    }
}

void Logger::println(const String &message) {
    if (currentOutput == HW_SERIAL) {
        Serial2.println(message);
    } else {
        BTSerial.println(message);
    }
}

void Logger::printError(uint8_t errorType) {
    const String message = errorMessages[errorType];
    if (currentOutput == WEBSERIAL) {
        BTSerial.println("[ERROR -> LOGGER]: " + message);
    } 
    Serial2.println("[ERROR -> LOGGER]: " + message);
}

void Logger::printError(const String &message) {
    if (currentOutput == WEBSERIAL) {
        BTSerial.println("[ERROR " + message);
    } 
    Serial2.println("[ERROR " + message);
}

void Logger::printValue(const String &key, const String &value) {
    if (currentOutput == HW_SERIAL) {
        Serial2.println(key + ": " + value);
    } else {
        BTSerial.println(key + ": " + value);
    }
}

bool Logger::available(){
    if (currentOutput == HW_SERIAL) {
        return Serial2.available();
    } else {
        BTSerial.available();
    }
}

long Logger::parseInt(){
    if (currentOutput == HW_SERIAL) {
        return Serial2.parseInt();
    } else {
        BTSerial.parseInt();
    }
}