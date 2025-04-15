#include "line.h"

bool Line::theresNoInfo(){
  return line_status == NO_INFO;
}

bool Line::isConnected(){
  return line_status == CONNECTED;
}

bool Line::isDisconnected(){
  return line_status == DISCONNECTED;
}

lineStatus Line::getLineStatus(){
  return line_status;
}

void Line::setLineStatus(lineStatus status){
  line_status = status;
}

void Line::saveInfo(String data){
  File f = SPIFFS.open(INFO_FILE, "w");
  if (f) {
    f.print(data);
    f.close();
  }
}

float Line::getPPMFromMemory(){
  preferences.begin("my-app", false);
  float ppm = preferences.getFloat("ppm", DEFAULT_PPM);
  preferences.end();

  DEBUG(("PPM: " + String(ppm)).c_str());
  return ppm;
}

String Line::getInfoFromSF(){
  if (SPIFFS.exists(INFO_FILE)) {
    File f = SPIFFS.open(INFO_FILE, "r");
    if (f) {
      String stored_data;
      stored_data = f.readString();
      // DEBUG(stored_data);
      f.close();
      setLineStatus(CONNECTED);
      return stored_data;
    } else { // I should find the way to catch this.
      return "";
    }
  } else { // I should find the way to catch this.
      return "";
    } 
}

void Line::savePPM( float ppm ){
  DEBUG(("PPM: "+String(ppm)).c_str());
  preferences.begin("my-app", false);
  preferences.putFloat("ppm", ppm);
  preferences.end();
}

void Line::saveEmergencyCard(const char * emergency_card){
  preferences.begin("my-app", false);
  preferences.putString("emergency_card", emergency_card);
  preferences.end();
}

String Line::getEmergencyCardFromMemory(){
  preferences.begin("my-app", false);
  const String emergency_card_id = preferences.getString("emergency_card", "");
  preferences.end();
  return emergency_card_id;
}

bool Line::compareEmergencyCard(String card_id){
  const String emergency_card = getEmergencyCardFromMemory();
  return emergency_card.equals(card_id);
}

void Line::initPouringLog(String user, uint16_t pulses){
  deletePouringLog();
  
  logFile = SPIFFS.open("/fillingLog.txt", FILE_APPEND);
  logFile.println(user + ", " + pulses);
}

void Line::savePouredPulses(uint16_t pulses){
  logFile.println(pulses);
  logFile.flush();
}

pouringLog Line::getPouringLog(){
  pouringLog log;
  logFile = SPIFFS.open("/fillingLog.txt", FILE_READ);
  if (logFile) {
    String line = logFile.readStringUntil('\n');
    log.user = line.substring(0, line.indexOf(','));
    log.pulses = line.substring(line.indexOf(',') + 2).toInt();
    log.poured_pulses = 0;
    while (logFile.available()) {
      line = logFile.readStringUntil('\n');
      log.poured_pulses += line.toInt();
    }
    logFile.close();
  }
  return log;
} 

bool Line::theresNoFinishedLog(){
  // TODO THIS IS NOT FINISHED YET 

  logFile = SPIFFS.open("/fillingLog.txt", FILE_READ);
  if (logFile) {
    String line = logFile.readStringUntil('\n');
    logFile.close();
    return line.length() == 0;
  }
  return true;
}

void Line::closeLogFile(){
  logFile.close();
}


void Line::deletePouringLog(){
  SPIFFS.remove("/fillingLog.txt");
}

void Line::setPoruingOrder(String user, uint16_t ml, String concept){
  current_order.user = user;
  current_order.ml = ml;
  current_order.concept = concept;
}

void Line::removePouringOrder(){
  current_order.user = "";
  current_order.ml = 0;
  current_order.concept = "";
}

bool Line::theresPendingOrder(){
  return current_order.ml > 0;
}

PourOrder Line::getPouringOrder(){
  return current_order;
}

void Line::DEBUG(const char *message){
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "[Line]: %s", message);
  logger.println(buffer);
}
