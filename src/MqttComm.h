#ifndef MQTT_COMM_H
#define MQTT_COMM_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <functional>
#include <map>
#include "Logger.h"

class MqttIO {
  public:
    void loop();
    bool isConnected();
    void connect(const char *ip);
    void setConfigString(String line_id);
    String getConfigString();
    void on(const char *event, std::function<void(const char *, size_t)> func);
    void publishEvent(const char *event, const String &payload);

    void redeemBeer(String client_id, String keg_id);
    void fetchCardId(String card_id, bool client = false);
    void registerPurchase(String client_id, String worker_id, String type, String qty, String keg_id);
    void updateStatus(uint16_t ml, String line_id);
    void confirmOrder(String user);
    void rejectOrder();
    void requestLineData();

  private:
    WiFiClient wifiClient;
    PubSubClient mqttClient{wifiClient};
    std::map<String, std::function<void(const char *, size_t)>> handlers;
    String set_up = "";
    void handleMessage(char *topic, byte *payload, unsigned int len);
    void DEBUG(const char *message);
};

#endif
