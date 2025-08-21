#include "SocketComm.h"

// Static instance for callback
MqttComm* MqttComm::instance = nullptr;

void MqttComm::loop(){
  mqttClient.loop();
  
  // Reconnect if connection lost
  if (!mqttClient.connected()) {
    DEBUG("MQTT disconnected, attempting to reconnect...");
    // Note: Reconnection logic could be added here
  }
}

void MqttComm::connect(const char *broker_ip){
  mqttClient.setClient(wifiClient);
  mqttClient.setServer(broker_ip, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  
  // Set static instance for callback
  instance = this;
  
  // Generate device ID if not set
  if (deviceId.length() == 0) {
    deviceId = WiFi.macAddress();
    deviceId.replace(":", "");
  }
  
  // Connect to MQTT broker
  String clientId = "TapOMeter_" + deviceId;
  if (mqttClient.connect(clientId.c_str())) {
    DEBUG(("Connected to MQTT broker as " + clientId).c_str());
  } else {
    DEBUG("Failed to connect to MQTT broker");
  }
}

void MqttComm::redeemBeer(String client_id, String keg_id){
  String payload = "{ \"clientId\":\"" + client_id + "\", \"kegId\":\"" + keg_id + JSON_END;
  publish(TOPIC_REDEEM_BEER, payload.c_str());
}

bool MqttComm::isConnected(){
  return mqttClient.connected();
}

void MqttComm::requestLineData(){
  publish(TOPIC_SET_UP, set_up.c_str());
}

void MqttComm::on(const char* topic, std::function<void (const char * payload, size_t length)> func){
  topicHandlers[String(topic)] = func;
  subscribe(topic);
}

void MqttComm::registerPurchase(String client_id, String worker_id, String type, String qty, String keg_id){
  const String data = "{ \"workerId\":\"" + worker_id + "\", \"kegId\":\"" + keg_id + "\", \"concept\":\"" + type + "\", \"qty\": \"" + qty;
  const String client = (client_id.length() > 0 ? "\", \"clientId\": \"" + client_id : "");
  String payload = data + client + JSON_END;
  publish(TOPIC_FINISHED_POUR, payload.c_str());
  DEBUG(payload.c_str());
}

void MqttComm::rejectOrder(){
  publish(TOPIC_LINE_NOT_AVAILABLE, set_up.c_str());
}

void MqttComm::confirmOrder(String user){
  String payload = "{\"user\":\"" + user + JSON_END;
  publish(TOPIC_CONFIRM_ORDER, payload.c_str());
}


void MqttComm::setConfigString(String line_id){
  set_up = "{ \"id\": \"" + line_id + JSON_END;
  DEBUG(set_up.c_str());
}

String MqttComm::getConfigString(){
  return set_up;
}

void MqttComm::fetchCardId(String card_id, bool client){
  String payload = "{\"cardId\":\"" + card_id + JSON_END;
  publish(client ? TOPIC_GET_CLIENT : TOPIC_GET_WORKER, payload.c_str());
}

void MqttComm::updateStatus(uint16_t ml, String line_id){
  String payload = "{\"pouredVolume\": " + String(ml) + ", \"lineId\": \"" + line_id + JSON_END;
  publish(TOPIC_UPDATE_STATUS, payload.c_str());
}

// MQTT callback function
void MqttComm::mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (instance != nullptr) {
    instance->handleMessage(topic, payload, length);
  }
}

void MqttComm::handleMessage(char* topic, byte* payload, unsigned int length) {
  // Convert payload to null-terminated string
  char* message = new char[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  String topicStr = String(topic);
  
  // Remove topic prefix if present
  String topicPrefix = getTopicPrefix();
  if (topicStr.startsWith(topicPrefix)) {
    topicStr = topicStr.substring(topicPrefix.length());
  }
  
  // Find and call the appropriate handler
  auto handler = topicHandlers.find(topicStr);
  if (handler != topicHandlers.end()) {
    handler->second(message, length);
  } else {
    DEBUG(("No handler found for topic: " + topicStr).c_str());
  }
  
  delete[] message;
}

void MqttComm::publish(const char* topic, const char* payload) {
  String fullTopic = getTopicPrefix() + String(topic);
  if (mqttClient.connected()) {
    mqttClient.publish(fullTopic.c_str(), payload);
    DEBUG(("Published to " + fullTopic + ": " + String(payload)).c_str());
  } else {
    DEBUG("Cannot publish: MQTT not connected");
  }
}

void MqttComm::subscribe(const char* topic) {
  String fullTopic = getTopicPrefix() + String(topic);
  if (mqttClient.connected()) {
    mqttClient.subscribe(fullTopic.c_str());
    DEBUG(("Subscribed to " + fullTopic).c_str());
  } else {
    DEBUG("Cannot subscribe: MQTT not connected");
  }
}

String MqttComm::getTopicPrefix() {
  return "tapometer/" + deviceId + "/";
}

void MqttComm::DEBUG(const char *message){
  char buffer[250];
  snprintf(buffer, sizeof(buffer), "[MqttComm]: %s", message);
  logger.println(buffer);
}