#include "Screen.h"
// #include <JPEGDecoder.h>


bool Screen::isTouchEneable(){
  return touch_eneable;
}

void  Screen::setTouchEneable(bool value){
  touch_eneable = value;
}


void Screen::LockScreen(){
  actualScreen=LOCK_SCR;
  DEBUG("Lock Screen.  Pasar tarjeta");
  entryOptions.clear();
  entryOptions.push_back(cog);
  DEBUG("1.- Configuracion");
}
void Screen::setInfo(const char * info) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, info);
  JsonObject obj = doc.as<JsonObject>();
  beerName = obj["name"].as<String>();
  beerStyle = obj["style"].as<String>();
  beerAbv =  obj["abv"].as<String>();
  kegId = obj["idKeg"].as<String>();
  noLinea = obj["noLinea"].as<uint16_t>();
  emergencyCard = obj["emergencyCard"].as<String>();
}

void Screen::SelectQty(String user, String client) {
  entryOptions.clear();

  entryOptions.push_back(Vaso);
  entryOptions.push_back(Taster);
  entryOptions.push_back(Growler);
  entryOptions.push_back(Mermar);
  entryOptions.push_back(Cancelar);
  DEBUG("Seleccionar qty");
  DEBUG("1.- Vaso");
  DEBUG("2.- Taster");
  DEBUG("3.- Growler");
  DEBUG("4.- Merma");
  DEBUG("5.- Cancelar");
  // currentScreen = "SEL_QTY";
  actualScreen = SELECT_QTY;
}


void Screen::removeInfo(){
  String beerName = "";
  String beerStyle = "";
  String beerAbv = "";
  String kegId = "";
}

uint8_t Screen::isPressed(){
  if (!logger.available()) return NONE;

  if (isOnInputMode()){
    String line = logger.readString();
    if (line == "esc") {
      exitInputMode();
      LockScreen();
      return NONE;
    }

    float value = line.toFloat();
    if (value <= 0.0f) {
      DEBUG("Valor inválido, inténtalo de nuevo:");
      return NONE;
    }

    if (_inputCallback != nullptr) _inputCallback(value);
    exitInputMode();

    return NONE;
  }
  
  // reading incoming integer from serial port
  const uint8_t index = logger.parseInt() -1;
  if (index  >= entryOptions.size()) {
    DEBUG((String(index)+": Invalid option").c_str());
    return NONE;
  }
  
  const int8_t option = entryOptions[index]; 

  entryOptions.clear();
  return option;
}

  


void Screen::connecting(){
  actualScreen = CONNECTING_SCR;

  DEBUG("Connectando nigga! ");
}

void Screen::AP(String name, bool retriable){
  DEBUG("This shit it's on AP mode");
  actualScreen = AP_SCR;
}

void Screen::showReady(){
  entryOptions.clear();
  entryOptions.push_back(Listo);
  DEBUG("1.- Listo");
}

void Screen::notConnectedToSever(){
  DEBUG("Error en el servidor");
}

void Screen::setEmergency(bool state){
  if (!state) error = NO_ERROR;
  emergency_mode = state;
}

bool Screen::isOnEmergency(){
  return emergency_mode;
}

String Screen::getErrorMessage(){
  if (error == WIFI_NOT_CONNECTED){
    return WIFI_NOT_CONNECTED_MSG;
  }
  else if (error == NO_SERVER_CONNECTION){
    return NO_SERVER_CONNECTION_MSG;
  }
  return "Otra cosa sucedio";
}

int8_t Screen::getError(){
  return error;
}

void Screen::setError(int8_t err){
  error = err;
}

void Screen::noWifi(){
  DEBUG("Paga el wifi jodido");
}

void Screen::tapCard(){
  DEBUG("Acercar tarjeta");
}

void Screen::servingScreen(boolean drawIt, uint8_t update, String type){
  actualScreen = SERVING_SCR;
  if (drawIt) {
    entryOptions.clear();
    isServing = true;
    entryOptions.push_back(Cancelar);
  }
    DEBUG((String(update) + "%").c_str());
}


void Screen::NoBeerAssigned(){
  DEBUG("No beer assigned");
  actualScreen = NO_BEER_SCR;
}

void Screen::mermar(uint16_t ml){
  if (ml == 0) {
    DEBUG("Mermar");
    entryOptions.clear();
    isMermando = true;
    entryOptions.push_back(Listo);
  }
    DEBUG((String(ml,0)+" ml").c_str());

}

void Screen::drawMl(double ml, bool isMl){
  const String unit = isMl ? " mL" : " pul";
  DEBUG((String(ml,0)+unit).c_str());
}

void Screen::hideLoadingModal(){
  loading_modal = false;
}

void Screen::LoadingModal(){
  DEBUG("Ejperate!");
  actualScreen = LOADING_SCR;
}

void Screen::drawError(){
  DEBUG(getErrorMessage().c_str());
}

void Screen::ServeOptionsTaster(){
  DEBUG("Selecciona Taster");
  actualScreen = TASTER_QTY;
  entryOptions.clear();
  entryOptions.push_back(Oz_2);
  entryOptions.push_back(Oz_5);
  entryOptions.push_back(Cancelar);
  DEBUG("1.- 2 Oz");
  DEBUG("2.- 5 Oz");
  DEBUG("3.- Cancelar");
}

void Screen::ServeOptionsGlass(){
  DEBUG("Selecciona vaso");
  actualScreen = PINT_QTY;
  entryOptions.clear();
  entryOptions.push_back(Oz_8);
  entryOptions.push_back(Oz_10);
  entryOptions.push_back(Oz_16);
  entryOptions.push_back(Cancelar);
  DEBUG("1.- 8 Oz");
  DEBUG("2.- 10 Oz");
  DEBUG("3.- 16 Oz");
  DEBUG("4.- Cancelar");
}

void Screen::ServeOptionsGrowler(){
  DEBUG("Selecionar Growler");
  actualScreen = GROWLER_QTY;
  entryOptions.clear();
  entryOptions.push_back(Oz_32);
  entryOptions.push_back(Oz_64);
  entryOptions.push_back(Oz_128);
  entryOptions.push_back(Cancelar);
  DEBUG("1.- 32 Oz");
  DEBUG("2.- 64 Oz");
  DEBUG("3- 128 Oz");
  DEBUG("4.- Cancelar");
}

void Screen::Settings(){
  actualScreen = CONFIG_SCR;
  entryOptions.clear();
  DEBUG("Habemus configuracion, Selecciona:");
  entryOptions.push_back(Calibrar);
  entryOptions.push_back(Actualizar);
  entryOptions.push_back(ENTER_CALIBRATION_FACTOR);
  entryOptions.push_back(Back);
  DEBUG("1.- Calibra");
  DEBUG("2.- Update OTA");
  DEBUG("3.- Enter Calibration Factor");
  DEBUG("4.- Back");
}

void Screen::calibrationScreen(uint16_t ml){
  actualScreen = CALIBRATE_SCR;
  if (ml == 0) {
    isCalibrating = true;
    entryOptions.clear();
    DEBUG("calibarcion acutal");
    DEBUG(String(ppm).c_str());
    DEBUG("LLenar 300ml y presionar listo.");
    entryOptions.push_back(Listo);
  }
    DEBUG((String(ml,0)+" pul").c_str());
}

void Screen::saveCalibration(uint16_t pulses){
  actualScreen = SAVE_SCR;
  const float newPpm = pulses/300.0;
  DEBUG("Guardar");
  DEBUG((String(newPpm)+" p/ml").c_str());
  entryOptions.clear();
  DEBUG("Nuevo valor");
  entryOptions.push_back(Descartar);
  entryOptions.push_back(Guardar);
  DEBUG("1- Descartar");
  DEBUG("2.- Guardar");
}

void Screen::enterCalibrationFactor(const std::function<void(float)>& cb){
  actualScreen = ENTER_CALIBRATION_SCR;
  entryOptions.clear();
  DEBUG("Enter Calibration Factor o 'esc' para cancelar");
  enterInputMode(cb);
}

void Screen::otaUpdateScreen(){
  actualScreen = OTA_UPDATE_SCR;
  DEBUG("Actualizando OTA ...");
  entryOptions.clear();
  entryOptions.push_back(Cancelar);
  DEBUG("1.- Cancelar");

}

void Screen::retryOrAP(){
  actualScreen = RETRY_AP_SCR;
  DEBUG("No se pudo conectar al WiFi, desea?");
  entryOptions.clear();
  entryOptions.push_back(Retry);
  entryOptions.push_back(CONFIGURAR_WIFI);
  entryOptions.push_back(BOOT_WITH_FILE);
  DEBUG("1.- Re-intentar");
  DEBUG("1.- Conf. WiFi");
  DEBUG("3.- Modo de Emergencia");
}

void Screen::DEBUG(const char *message){
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "[Screen]: %s", message);
  logger.println(buffer);
}