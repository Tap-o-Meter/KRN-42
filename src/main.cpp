#include "main.h"
#include <esp_log.h>


WIFI wifi;
Line line;
Screen screen;
SocketIO api;

WiFiManager32 wifiManager;
Reader reader(SS_PIN, RST_PIN);

int8_t remote_concept = NONE;
volatile uint16_t pulse_counter = 0;
bool reset = false, selecting_opt = false, loading = false, redeem_beer = false, remote_sell;

//-------------------------------------->Set UP
void setup() {
  logger.init();
  // Serial2.begin(115200);
  // Serial2.setDebugOutput(false);
  // Serial2.setDebugOutput(true);
  SPIFFS.begin(true);

  pinMode(VALVE_PIN, OUTPUT);
  pinMode(FLOWMETER_PIN, OUTPUT);

  digitalWrite(VALVE_PIN, LOW);

  api.setConfigString(wifi.macAddress());

  setUpWiFi();                                                       
  reader.init();
  screen.ppm = line.getPPMFromMemory();

  uint8_t no_tries = 0;

  while (line.theresNoInfo() && no_tries < 5){
    api.requestLineData();
    const uint32_t time_out = 10*1000 + millis();
    while (millis() < time_out && line.theresNoInfo()) vTaskDelay(100 / portTICK_PERIOD_MS);
    no_tries++;
  }

  if (no_tries > 4) bootOptions();

  pinMode(FLOWMETER_PIN, INPUT);
}

void loop() {
  reset = false;
  if (line.isDisconnected()) {                       //---------------------> Line not connected !!!!!!! ------------------------
    screen.NoBeerAssigned();
    while (line.isDisconnected()) { vTaskDelay(100 / portTICK_PERIOD_MS);}
  }

  screen.LockScreen();
  detachInterrupt(FLOWMETER_PIN);
  // screen.setTouchEneable(true);

  while (!reset) {
    if (!wifi.isConnected()) {
      wifi.reconnect(true);
    }

    if (!wifi.isConnected() && !screen.isOnEmergency()) {
      screen.noWifi();
      screen.setEmergency(true);
    }

    else if (wifi.isConnected()  && screen.isOnEmergency() && screen.getError() == WIFI_NOT_CONNECTED) {
      screen.setEmergency(false);
      reset = true;
    }

     // else if (!webSocket.connected && !screen.isOnEmergency()) {  //  --------------------> webSocket not Connected !!!!!!! --------------------
    else if (!api.isConnected() && !screen.isOnEmergency()) {  //  --------------------> webSocket not Connected !!!!!!! --------------------
      screen.notConnectedToSever();
      screen.setEmergency(true);
      // screen.screenServer();       // ---------------------> Server for the screenshots
    }

    else if (api.isConnected() && screen.isOnEmergency()){
      screen.setEmergency(false);
      // webSocket.emit(SET_UP, set_up.c_str());
      api.requestLineData();
      screen.tapCard();
    }

    else if (line.theresPendingOrder()) {
      screen.isServing = true;
      const bool finished = countQty("", line.getPouringOrder().ml);

      const String user = line.getPouringOrder().user;
      const String concept = line.getPouringOrder().concept;
      const String pouredVolume = (String)(pulse_counter/(screen.ppm*1000));

      commitPurchase(concept, pouredVolume, user);
      line.removePouringOrder();
      screen.LockScreen();
    }

    handleTouch();
    if (reader.on()) {
      if(line.compareEmergencyCard(reader.getCardString()) && !api.isConnected()) reader.setEmergency();
      else {
        api.fetchCardId(reader.getCardString());
        // fetchCardId();
        screen.LoadingModal();
        if (!reader.theresUser()) screen.LockScreen();
      }       
    }

    if (remote_sell) handleTouch(true); 

    if (reader.theresUser()) { 
      screen.SelectQty(reader.getUser());
      lineUnlocked();
      return;
    }
    vTaskDelay(25 / portTICK_PERIOD_MS);
    }
}

void lineUnlocked(){
  selecting_opt = true;
  while (selecting_opt) {
    if (!reader.theresClient() && reader.on() && !reader.isOnEmergency()) {
      api.fetchCardId(reader.getCardString(), true);
      // fetchCardId(true);
      screen.LoadingModal();
      screen.SelectQty(reader.getUser(), reader.getClient());
    }
    handleTouch();
  }
}

//-------------------------------------->Socket Handlers
void event(const char * payload, size_t length) {
  //THIS IS FOR DEBUGGING PURPOSES
  DEBUG(("got message:"+ String(payload)).c_str());
}

void onNewEmergencyCard(const char * payload, size_t length) {
line.saveEmergencyCard(payload);
}

void onConnect(const char * payload, size_t length) {
  if (!line.isConnected()) api.requestLineData();
}

void onDisconnect(const char * payload, size_t length) {
  // DEBUG("Valio madre");
}

void onRemoteSell(const char * payload, size_t length) {
  JsonObject json_response = JsonObject();
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  // loading = false;
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed with code ");
    DEBUG(error.c_str());
  }
  json_response = doc.as<JsonObject>();
  if (json_response["confirmation"].as<String>().equals("success")) {
    JsonObject userData = json_response["data"].as<JsonObject>();
    const String user = (userData["nombre"].as<String>()+" "+userData["apellidos"].as<String>());
    const String workerId = userData["_id"].as<String>();
    const uint8_t concept = json_response["concept"].as<int>();

    reader.setUser(user, workerId);
    remote_sell = true;
    remote_concept = concept;
  }
}

void onDisconnectedLine(const char * payload, size_t length){
  screen.removeInfo();
  reset = true;
  line.setLineStatus(DISCONNECTED);
  DEBUG("tiene que estar desconectada");
}

void onInfoRecived(const char * payload, size_t length) {
  screen.setInfo(payload);
  line.saveInfo(payload);
  line.setLineStatus(CONNECTED);
  const String new_emergency_card = screen.emergencyCard;
  const String old_emergency_card = line.getEmergencyCardFromMemory();
  DEBUG(old_emergency_card.c_str());
  if (!line.compareEmergencyCard(new_emergency_card)) line.saveEmergencyCard(new_emergency_card.c_str());
  reset = true;
}

void onClaimBeer(const char * payload, size_t length) {
  reader.setClient("N/A", "N/A");
  reader.setClient("", String(payload));
  DEBUG("esto valio re quete verga");
  remote_sell = redeem_beer = true;
}

void onLineChange(const char * payload, size_t length) {
  api.requestLineData();
}

void validateResponse(const char * payload, size_t length){
  DEBUG(payload);
  JsonObject json_response = JsonObject();
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  // loading = false;
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed with code ");
    DEBUG(error.c_str());
  }
  json_response = doc.as<JsonObject>();
  if (json_response["confirmation"].as<String>().equals("success")) {
    JsonObject userData = json_response["data"].as<JsonObject>();
    const String user = (userData["nombre"].as<String>()+" "+userData["apellidos"].as<String>());
    const String workerId = userData["_id"].as<String>();
    reader.setUser(user, workerId);
  }
}

void validateClient(const char * payload, size_t length){
  JsonObject json_response = JsonObject();
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed with code ");
    DEBUG(error.c_str());
  }
  json_response = doc.as<JsonObject>();
  if (json_response["confirmation"].as<String>().equals("success")) {
    JsonObject userData = json_response["data"].as<JsonObject>();
    const String client = (userData["name"].as<String>()+" "+userData["lastName"].as<String>());
    const String clientId = userData["_id"].as<String>();
    reader.setClient(client, clientId);
  }
}

void requestDevice(const char * payload, size_t length){
  if (screen.isServing){
    api.rejectOrder();
    return;
  }

  JsonObject json_response = JsonObject();
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  // loading = false;
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed with code ");
    DEBUG(error.c_str());
  }

  json_response = doc.as<JsonObject>();
  const uint16_t volume_ml = json_response["volume"].as<uint16_t>();
  const String user = json_response["userId"].as<String>();
  const String concept = json_response["concept"].as<String>();

  api.confirmOrder(user);
  
  line.setPoruingOrder(user, volume_ml, concept);  
}

void startPour(const char * payload, size_t length){
  // decoding this json : const { volume } = msg;

  if (screen.isServing){
    /* Should return something like busy line or bla bla */
    return;
  }
  
  JsonObject json_response = JsonObject();
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  // loading = false;
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed with code ");
    DEBUG(error.c_str());
  }
  json_response = doc.as<JsonObject>();
  const uint16_t volume_ml = json_response["volume"].as<uint16_t>();
  const String user = json_response["userId"].as<String>();
  const String concept = json_response["concept"].as<String>();

  line.setPoruingOrder(user, volume_ml, concept);  
  
  // const bool finished = countQty("", volume_ml);
  // if (finished) commitPurchase("GROWLER", "4");
}

void stopPour(const char * payload, size_t length){
  if(!screen.isServing) {
    /* Should return something like busy line or bla bla */
    return;
  }

  screen.isServing = false;
  // screen.servingScreen(false, 0, "");
}

//-------------------------------------->Async Funtions
void socketManager( void * pvParameters ){
  while(1){
    // webSocket.loop();
    api.loop();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}


void SetConnectedScreen(bool retriable){
  // if retriable should should give 2 options, Retry or AP
  screen.AP(wifi.ap_name, retriable); 
  DEBUG(wifi.ap_name.c_str());
  if (retriable)  wifiManager.startConfigPortal(wifi.ap_name.c_str(), SECRET_PASS, handleTouch);
  else            wifiManager.startConfigPortal(wifi.ap_name.c_str(), SECRET_PASS); 
}

//-------------------------------------->Helper Funtions
void setUpWiFi(){
  if (wifi.theresValidSSID()) { // if valid SSID saved
    vTaskDelay(random(2000) / portTICK_PERIOD_MS);
    screen.connecting();
    if (wifi.setUpWiFi()) setUpSocketConnection();
    else bootOptions();
  }
  else SetConnectedScreen();
  DEBUG("Salio");
  vTaskDelay(2000 / portTICK_PERIOD_MS);
}

void bootOptions(){
  // DEBUG(line.getLineStatus());
  screen.retryOrAP();
  while (screen.actualScreen == RETRY_AP_SCR && line.theresNoInfo()) handleTouch(); 
}

void setUpSocketConnection(){

  api.on(CONNECT, onConnect);
  api.on(CLAIM_BEER, onClaimBeer);
  api.on(DISCONNECT, onDisconnect);
  api.on(CHANGE_LINE, onLineChange);
  api.on(REMOTE_SELL, onRemoteSell);
  api.on(DEVICE_INFO, onInfoRecived);
  api.on(VALIDATED_CLIENT, validateClient);
  api.on(VALIDATED_USER, validateResponse);
  api.on(DISCONNECTED_LINE, onDisconnectedLine);
  api.on(ADD_EMERGENCY_CARD, onNewEmergencyCard);
  api.on(START_POUR, startPour);
  api.on(STOP_POUR, stopPour);
  api.on(REQUEST_DEVICE, requestDevice);
  
  api.connect(wifi.getIP().c_str());
  xTaskCreatePinnedToCore(socketManager, "Socket loop", 16384,  NULL, 1, NULL, CORE0); // THIS SHOULD BE MOVED TO SOCKETCOMM.H 
}

void filling(uint16_t pulses){
  pulse_counter = 0;
  uint8_t last_percent = 0;
  const uint8_t incrementor = pulses /screen.ppm < 200 ? 5: 2;
  uint32_t time_out = millis() + SERVING_TIME_OUT;
  const uint8_t min_qty = (uint8_t)((100*screen.ppm*20)/pulses);

  // line.initPouringLog(reader.getWorkerId(), pulses);

  while ((pulse_counter < pulses) && screen.isServing && time_out > millis()) {
    if ((pulse_counter < screen.ppm * 20) || (last_percent > 69)) handleTouch();
    const uint8_t percent = (uint8_t)((100*pulse_counter)/pulses);
    if (percent >( last_percent + incrementor) ) {
      // line.savePouredPulses(pulse_counter);
      api.updateStatus(pulse_counter/screen.ppm, wifi.macAddress());

      // if      (percent == min_qty) screen.hideCancell(); //hide cancel
      if (percent == 70) screen.showReady(); // show listo

      last_percent = percent;
      time_out = millis() + SERVING_TIME_OUT;
      screen.servingScreen(false, percent, "");
    }
  }
  // line.closeLogFile();

  selecting_opt = false;
  if (last_percent > 70 && !screen.isServing) screen.isServing = true;
}

void IRAM_ATTR flowCounter() {
  pulse_counter++;
}

uint16_t mermando(){
  unsigned int last_val = 0;
  pulse_counter = 0;
  const bool mermando = screen.isMermando;
  uint32_t time_out = millis() + SERVING_TIME_OUT;
  while ((screen.isMermando || screen.isCalibrating) && time_out > millis()) {
    handleTouch();
    const unsigned int val = mermando ? pulse_counter/screen.ppm : pulse_counter;
    if (val > last_val ) {
      last_val = val;
      time_out = millis() + SERVING_TIME_OUT;
      screen.drawMl(val, mermando);
    }
  }
  selecting_opt = false;
  return pulse_counter;
}

void commitPurchase( String concept, String qty, String user){
  if(!reader.isOnEmergency()){
    const String worker_id = reader.getWorkerId();
    const String client_id = reader.getClientId();

    const String fixed_user = user.length() > 1 ? user : worker_id;
    api.registerPurchase(client_id, fixed_user, concept, qty, screen.kegId);
  }
  screen.isServing = false;
  reader.removeUser();
}

void handleTouch(bool remote){
  const int8_t btn = remote ? remote_concept : screen.isPressed();
  if (remote) {
    reset = true;
    remote_sell = false;
    remote_concept = NON_SELECTED;
  }
  if (btn != NON_SELECTED){
    if      (btn == Vaso) {
      screen.ServeOptionsGlass();
    }
    else if (btn == Taster) {
      screen.ServeOptionsTaster();
    }
    else if (btn == Growler) {
      screen.ServeOptionsGrowler();
    }
    else if (btn == Mermar) {
      const uint16_t merma_pulses = countQty(false);
      if (merma_pulses > 50)
        commitPurchase("MERMA", (String)(merma_pulses/(screen.ppm*1000)));
      else reader.removeUser();
    }
    else if (btn == Cancelar) {
      const bool remove = reader.getWorkerId().length() < 4 && !reader.isOnEmergency();
      if (!screen.isServing) reader.removeUser(); // This is for the Listo on Cancel btn !!!!!!
      screen.isServing = selecting_opt = false;
      if (remove) screen.LockScreen();
    }
    else if (btn == Listo) {
      if (selecting_opt) {
        screen.isMermando = selecting_opt =false;
      } else screen.isCalibrating = false;
    }
    else if (btn == Oz_2){
      const bool finished = countQty("Taster 2oz", 60);
      if (finished) commitPurchase("TASTER", ".06");
    }
    else if (btn == Oz_5){
      const bool finished = countQty("Taster 5oz", 142);
      if (finished) commitPurchase("TASTER", ".142");
    }
    else if (btn == Oz_8){
      const bool finished = countQty("Medio Vaso 8oz", 236);
      if (finished) commitPurchase("PINT", ".236");
    }
    else if (btn == Oz_10){
      const bool finished = countQty("Vaso 10oz", 296);
      if (finished) commitPurchase("PINT", ".296");
    }
    else if (btn == Oz_16){
      const bool finished = countQty("Vaso 16oz", 473);
      if (finished) commitPurchase("PINT", ".473");
    }
    else if (btn == Oz_32){
      const bool finished = countQty( "Growler 32oz", 1000);
      if (finished) commitPurchase("GROWLER", "1");
    }
    else if (btn == Oz_64){
      const bool finished = countQty("Growler 64oz", 2000);
      if (finished) commitPurchase("GROWLER", "2");
    }
    else if (btn == Oz_128){
      const bool finished = countQty("Growler 128oz", 4000);
      if (finished) commitPurchase("GROWLER", "4");
    }
    else if (btn == cog) {
      screen.Settings();
    }
    else if (btn == Calibrar) {
      const uint16_t merma_pulses = countQty(true);
      screen.saveCalibration(merma_pulses);
      //commitPurchase("MERMA", merma_pulses/PPM );
    }
    else if (btn == Actualizar) {
      otaUpdating();
    }
    else if (btn == Retry) {
      DEBUG("puto");
      ESP.restart();
    }
    else if ((btn == Back) || (btn == Descartar)) {
      // screen.tft.fillScreen(TFT_BLACK);
      screen.LockScreen();
    }
    else if (btn == Guardar) {
      const float new_ppm = pulse_counter/300.00;
      DEBUG(("Guardando ppm: " + (String)new_ppm + "").c_str());
      line.savePPM(new_ppm);
      screen.ppm = new_ppm;
      // screen.tft.fillScreen(TFT_BLACK);
      screen.LockScreen();
    }
    else if (btn == CONFIGURAR_WIFI) {
      DEBUG("CONFIGURAR_WIFI");
      SetConnectedScreen(true);
    }
    else if (btn == BOOT_WITH_FILE) {
      DEBUG("BOOT_WITH_FILE");
      const char * line_data = line.getInfoFromSF().c_str();
      screen.setInfo(line_data);
    }
  } 
  else if(redeem_beer){
    redeem_beer = reset = false;
    const bool finished = countQty("Vaso 16oz", 473);
    if (finished) {
      api.redeemBeer(reader.getClientId(), screen.kegId);
      reader.removeUser();
      screen.LockScreen();
    }
  }
}

bool countQty(String screen_msg, uint16_t ml){
  attachInterrupt(FLOWMETER_PIN, flowCounter, RISING); // THIS SHOULD BE AT LINE CLASS AND NAMED AS OPEN VALVE
  digitalWrite(VALVE_PIN, HIGH);
  screen.servingScreen(true, 0, screen_msg);
  filling(screen.ppm * ml);
  digitalWrite(VALVE_PIN, LOW);
  return screen.isServing;
}

uint16_t countQty(bool calibrate){
  attachInterrupt(FLOWMETER_PIN, flowCounter, RISING);  // THIS SHOULD BE AT LINE CLASS AND NAMED AS OPEN VALVE
  digitalWrite(VALVE_PIN, HIGH);
  calibrate ? screen.calibrationScreen(0) : screen.mermar(0);
  const uint16_t merma_pulses = mermando();
  digitalWrite(VALVE_PIN, LOW);
  return merma_pulses;
}

void otaUpdating(){
  screen.otaUpdateScreen();
  wifi.setUpOTA(screen.noLinea); 
  while (screen.actualScreen == OTA_UPDATE_SCR) {
    handleTouch();
    wifi.loopOTA();
  }
  wifi.stopOTA();
}

JsonObject decodeJson(const char * payload){
  JsonObject json_response = JsonObject();
  DynamicJsonDocument doc (1024);
  auto error = deserializeJson(doc, payload);
  if (error) {
    DEBUG("deserializeJson() failed with code ");
    DEBUG(error.c_str());
  }
  json_response = doc.as<JsonObject>();
  doc.clear();
  return json_response;
}

bool validateJsonResponse(JsonObject json_response){
  const char* confirmation = json_response["confirmation"];
  return strcmp(confirmation, "success") == 0;
}

void DEBUG(const char *message){
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "[Main]: %s", message);
  logger.println(buffer);
}