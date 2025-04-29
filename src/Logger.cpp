#include "Logger.h"

Logger logger; 

Logger::Logger() : currentOutput(WEBSERIAL) {} // Inicializa con SERIAL por defecto

void Logger::init(unsigned long baudRate, uint16_t no_linea) {
    if (currentOutput == HW_SERIAL) {
        Serial.begin(baudRate);
        Serial.setDebugOutput(false);
        Serial.setDebugOutput(true);
    } else {
        // BTSerial.begin("Tap-o-Meter_"+no_linea);

        // esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, 4, (uint8_t*)"235813");
  
        // // Configuramos el modo de escaneo:
        // // ESP_BT_CONNECTABLE: permite que el dispositivo acepte conexiones.
        // // ESP_BT_NON_DISCOVERABLE: lo hace invisible en los escaneos.
        // esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE);
    }
}

void Logger::setMessage(const String &msg) {
    if (currentOutput == HW_SERIAL) {
        // Serial.println(message);
    } else {
       message = msg;
    }
}

void Logger::setOutput(OutputType output) {
    currentOutput = output;
}

void Logger::print(const String &message) {
    if (currentOutput == HW_SERIAL) {
        Serial.print(message);
    } else {
        WebSerial.print(message);
        // BTSerial.print(message);
    }
}

void Logger::println(const String &message) {
    if (currentOutput == HW_SERIAL) {
        Serial.println(message);
    } else {
        WebSerial.println(message);
        // BTSerial.println(message);
    }
}

void Logger::printError(uint8_t errorType) {
    const String message = errorMessages[errorType];
    if (currentOutput == WEBSERIAL) {
        // BTSerial.println("[ERROR -> LOGGER]: " + message);
        WebSerial.println("[ERROR -> LOGGER]: " + message);
    } 
    Serial.println("[ERROR -> LOGGER]: " + message);
}

void Logger::printError(const String &message) {
    if (currentOutput == WEBSERIAL) {
        WebSerial.println("[ERROR " + message);
        // BTSerial.println("[ERROR " + message);
    } 
    Serial.println("[ERROR " + message);
}

void Logger::printValue(const String &key, const String &value) {
    if (currentOutput == HW_SERIAL) {
        Serial.println(key + ": " + value);
    } else {
        WebSerial.println(key + ": " + value);
        // BTSerial.println(key + ": " + value);
    }
}

bool Logger::available(){
    if (currentOutput == HW_SERIAL) {
        return Serial.available();
    } else {
        return message.length();
    }
}

long Logger::parseInt(){
    if (currentOutput == HW_SERIAL) {
        return Serial.parseInt();
    } else {
        const long value = message.toInt();
        message = "";
        return value;
    }
}

float Logger::parseFloat(){
    if (currentOutput == HW_SERIAL) {
        return Serial.parseFloat();
    } else {
        // Convierte la cadena completa a número de punto flotante
        float v = message.toFloat();
        message = "";
        return v;
    }
}

String Logger::readString(){
    if (currentOutput == HW_SERIAL) {
        // Lee hasta timeout o '\n'
        return Serial.readString();
    } else {
        // Devuelve todo lo recibido por WebSerial
        String s = message;
        message = "";
        return s;
    }
}