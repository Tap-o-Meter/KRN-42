#ifndef MY_SOCKET_H
#define MY_SOCKET_H
#include <Arduino.h>
#include "Logger.h"
#include "secrets.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <SocketIoClient.h>


#define JSON_END        "\"}"

// ------------------------------>Socket Handlers <------------------------------
#define VALIDATED_USER      "validated user"
#define CONNECT             "connect"
#define DISCONNECT          "disconnect"
#define DEVICE_INFO         "device info"
#define CHANGE_LINE         "changeLine"
#define DISCONNECTED_LINE   "disconnectedLine"
#define ADD_EMERGENCY_CARD  "addEmergencyCard"
#define VALIDATED_CLIENT    "validated client"
#define CLAIM_BEER          "claimBeer"
#define REMOTE_SELL         "remoteSell"
#define START_POUR          "start_pour"
#define STOP_POUR           "stop_pour"
#define REQUEST_DEVICE      "request_device"

// ------------------------------>  Socket events <------------------------------
#define REDEEM_BEER         "redeemBeer"
#define SET_UP              "setUp"
#define GET_WORKER          "getWorker"
#define GET_CLIENT          "getClient"
#define SALE_COMPLETE       "sale_complete"
#define UPDATE_STATUS       "update_status"
#define FINISHED_POUR       "finished_pour"
#define LINE_NOT_AVAILABLE  "line_not_available"
#define CONFIRM_ORDER       "confirm_order"


class SocketIO {
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
    void connect(const char * ip);
    void updateStatus(uint16_t ml, String line_id);
    void setConfigString(String line_id);
    JsonObject decodeJson(const char * payload);
    void redeemBeer(String client_id, String keg_id);
    bool validateJsonResponse(JsonObject json_response);
    void fetchCardId(String card_id, bool client = false);
    void on(const char* event, std::function<void (const char * payload, size_t length)> func);
    // void finishedPour(String line_id, String worker_id, String keg_id, String qty, String concept);
    void registerPurchase(String client_id, String worker_id, String type, String qty, String keg_id);
  private:
    SocketIoClient webSocket;
    String set_up = "";

    //Logger
    void DEBUG(const char *message);
    // void ERROR(ErrorType error);
};
#endif