#ifndef MY_MQTT_H
#define MY_MQTT_H
#include <Arduino.h>
#include "Logger.h"
#include "secrets.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <map>
#include <functional>


#define JSON_END        "\"}"

// ------------------------------>MQTT Topics <------------------------------
#define TOPIC_VALIDATED_USER      "validated_user"
#define TOPIC_CONNECT             "connect"
#define TOPIC_DISCONNECT          "disconnect"
#define TOPIC_DEVICE_INFO         "device_info"
#define TOPIC_CHANGE_LINE         "change_line"
#define TOPIC_DISCONNECTED_LINE   "disconnected_line"
#define TOPIC_ADD_EMERGENCY_CARD  "add_emergency_card"
#define TOPIC_VALIDATED_CLIENT    "validated_client"
#define TOPIC_CLAIM_BEER          "claim_beer"
#define TOPIC_REMOTE_SELL         "remote_sell"
#define TOPIC_START_POUR          "start_pour"
#define TOPIC_STOP_POUR           "stop_pour"
#define TOPIC_REQUEST_DEVICE      "request_device"

// ------------------------------>MQTT Topics (outgoing) <------------------------------
#define TOPIC_REDEEM_BEER         "redeem_beer"
#define TOPIC_SET_UP              "set_up"
#define TOPIC_GET_WORKER          "get_worker"
#define TOPIC_GET_CLIENT          "get_client"
#define TOPIC_SALE_COMPLETE       "sale_complete"
#define TOPIC_UPDATE_STATUS       "update_status"
#define TOPIC_FINISHED_POUR       "finished_pour"
#define TOPIC_LINE_NOT_AVAILABLE  "line_not_available"
#define TOPIC_CONFIRM_ORDER       "confirm_order"


class MqttComm {
  struct response{
    String worker_id;
    String worker_name;
  };
  
  public:
    void loop();
    bool isConnected();
    void rejectOrder();
    void requestLineData();
    void confirmOrder(String user);
    String getConfigString();
    void connect(const char * broker_ip);
    void updateStatus(uint16_t ml, String line_id);
    void setConfigString(String line_id);
    void redeemBeer(String client_id, String keg_id);
    void fetchCardId(String card_id, bool client = false);
    void on(const char* topic, std::function<void (const char * payload, size_t length)> func);
    void registerPurchase(String client_id, String worker_id, String type, String qty, String keg_id);
    
  private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    String set_up = "";
    String deviceId = "";
    std::map<String, std::function<void (const char * payload, size_t length)>> topicHandlers;
    
    // MQTT callback for incoming messages
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    static MqttComm* instance; // Static instance for callback
    void handleMessage(char* topic, byte* payload, unsigned int length);
    
    // Helper methods
    void publish(const char* topic, const char* payload);
    void subscribe(const char* topic);
    String getTopicPrefix();
    
    //Logger
    void DEBUG(const char *message);
};
#endif