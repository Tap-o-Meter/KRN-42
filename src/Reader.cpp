#include "Reader.h"
#include <SocketIoClient.h>
#include <WiFiClient.h>

Reader::Reader(byte SS_PIN, byte RST_PIN){
  mfrc522 = MFRC522(SS_PIN, RST_PIN);
}

void Reader::init() {
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
}

String Reader::getUser(){
  return user;
}

String Reader::getClient(){
  return client;
}

String Reader::getClientId(){
  return clientId;
}

String Reader::getWorkerId(){
  return workerId;
}

void Reader::setUser(String userString, String Id){
  user = userString;
  workerId = Id;
}

void Reader::setClient(String clientString, String Id){
  client = clientString;
  clientId = Id;
}

void Reader::setEmergency(){
  emergency = true;
  user = "Modo de emergencia";
}

bool Reader::isOnEmergency(){
  return emergency;
 }

bool Reader::theresUser(){
  return user.length() > 1;
}

bool Reader::theresClient(){
  return client.length() > 1 || emergency;
}

void Reader::removeUser(){
  user = "";
  if (emergency) emergency = false;
  else workerId = client = clientId = "";
}

String Reader::getCardString(){
  String id;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    id+=mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ";
    id+= String(mfrc522.uid.uidByte[i], HEX);
  }
  id.toUpperCase();
  id.remove(0,1);
  return id;
}

String Reader::getEmergencyCard(const char * payload){
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);
  JsonObject obj = doc.as<JsonObject>();
  return(obj["data"].as<String>());
}

boolean Reader::on() {
  if (  mfrc522.PICC_IsNewCardPresent()){
    if ( mfrc522.PICC_ReadCardSerial()){
      mfrc522.PICC_HaltA();
      return true;
    }
  }
  return false;
}


void Reader::DEBUG(const char *message){
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "[Reader]: %s", message);
  logger.println(buffer);
}