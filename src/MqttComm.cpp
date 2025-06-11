#include "MqttComm.h"
#include "secrets.h"

void MqttIO::loop() {
  mqttClient.loop();
}

bool MqttIO::isConnected() {
  return mqttClient.connected();
}

void MqttIO::connect(const char *ip) {
  mqttClient.setServer(ip, MQTT_PORT);
  mqttClient.setCallback([this](char *topic, byte *payload, unsigned int len) {
    handleMessage(topic, payload, len);
  });
  mqttClient.connect(ID.c_str());
  for (auto &h : handlers) {
    mqttClient.subscribe(h.first.c_str());
  }
}

void MqttIO::handleMessage(char *topic, byte *payload, unsigned int len) {
  String t = String(topic);
  String msg;
  for (unsigned int i = 0; i < len; ++i) msg += (char)payload[i];
  auto it = handlers.find(t);
  if (it != handlers.end()) {
    it->second(msg.c_str(), msg.length());
  }
}

void MqttIO::on(const char *event, std::function<void(const char *, size_t)> func) {
  handlers[String(event)] = func;
  if (mqttClient.connected()) {
    mqttClient.subscribe(event);
  }
}

void MqttIO::publishEvent(const char *event, const String &payload) {
  mqttClient.publish(event, payload.c_str());
}

void MqttIO::setConfigString(String line_id) {
  set_up = "{ \"id\": \"" + line_id + "\" }";
  DEBUG(set_up.c_str());
}

String MqttIO::getConfigString() {
  return set_up;
}

void MqttIO::requestLineData() {
  publishEvent(SET_UP, set_up);
}

void MqttIO::redeemBeer(String client_id, String keg_id) {
  publishEvent(REDEEM_BEER, "{ \"clientId\":\"" + client_id + "\", \"kegId\":\"" + keg_id + "\" }");
}

void MqttIO::confirmOrder(String user) {
  publishEvent(CONFIRM_ORDER, "{\"user\":\"" + user + "\" }");
}

void MqttIO::registerPurchase(String client_id, String worker_id, String type, String qty, String keg_id) {
  String data = "{ \"workerId\":\"" + worker_id + "\", \"kegId\":\"" + keg_id + "\", \"concept\":\"" + type + "\", \"qty\": \"" + qty;
  String client = (client_id.length() > 0 ? "\", \"clientId\": \"" + client_id + "\"}" : "}" );
  publishEvent(FINISHED_POUR, data + client);
}

void MqttIO::updateStatus(uint16_t ml, String line_id) {
  publishEvent(UPDATE_STATUS, "{\"pouredVolume\": " + String(ml) + ", \"lineId\": \"" + line_id + "\" }");
}

void MqttIO::fetchCardId(String card_id, bool client) {
  publishEvent(client ? GET_CLIENT : GET_WORKER, "{\"cardId\":\"" + card_id + "\" }");
}

void MqttIO::rejectOrder() {
  publishEvent(LINE_NOT_AVAILABLE, set_up);
}

void MqttIO::DEBUG(const char *message) {
  char buffer[250];
  snprintf(buffer, sizeof(buffer), "[MQTT]: %s", message);
  logger.println(buffer);
}

