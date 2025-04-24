#include "main.h"
#include <esp_log.h>

// ——— Global Objects ———
WIFI wifi;
Line line;
Screen screen;
SocketIO api;
WiFiManager32 wifiManager;
Reader reader(SS_PIN, RST_PIN);


int8_t    remote_concept   = NONE;
volatile uint16_t pulse_counter = 0;
bool      reset            = false,
          selecting_opt   = false,
          loading          = false,
          redeem_beer     = false,
          remote_sell     = false;

// ——— State Machine ———
enum class State {
  INIT,          // Configurar Wi-Fi + solicitar datos de línea
  IDLE,          // Espera de eventos: tarjeta, reconexión, orden pendiente
  SERVE_PENDING, // Servir orden recibida del servidor
  ERROR          // Error crítico
};
static State    currentState = State::INIT;
static uint8_t  initTries    = 0;
static uint32_t initTimeout  = 0;

portMUX_TYPE muxCounter = portMUX_INITIALIZER_UNLOCKED;

void setup() {
  // Inicialización hardware/periféricos
  logger.init();
  SPIFFS.begin(true);
  pinMode(VALVE_PIN, OUTPUT);
  pinMode(FLOWMETER_PIN, OUTPUT);
  digitalWrite(VALVE_PIN, LOW);

  // Módulos lógicos
  api.setConfigString(wifi.macAddress());
  reader.init();
  screen.ppm = line.getPPMFromMemory();

  // Arrancamos en INIT
  currentState = State::INIT;
}

void loop() {
  switch (currentState) {
    // — INIT: conecto Wi-Fi y pido datos de línea sin bloquear —
    case State::INIT:
      if (initTries == 0) {
        setUpWiFi();                  // tu función existente
        api.requestLineData();
        initTimeout = millis() + 10000;
        initTries++;
      }
      if (!line.theresNoInfo()) {
        initTries = 0;
        currentState = State::IDLE;
      }
      else if (millis() > initTimeout && initTries < 5) {
        api.requestLineData();
        initTimeout = millis() + 10000;
        initTries++;
      }
      else if (initTries >= 5) {
        bootOptions();  // tu pantalla de retry/AP
        initTries = 0;
      }
      break;

    // — IDLE: chequeos de conexión, lector y menú —
    case State::IDLE:
      if (line.isDisconnected()) {
        screen.NoBeerAssigned();
        break;
      }

      // Wi-Fi
      if (!wifi.isConnected()) {
        wifi.reconnect(true);
        if (!screen.isOnEmergency()) {
          screen.noWifi();
          screen.setEmergency(true);
        }
      } else if (screen.isOnEmergency() && screen.getError() == WIFI_NOT_CONNECTED) {
        screen.setEmergency(false);
      }

      // WebSocket / SocketIO
      if (!api.isConnected()) {
        if (!screen.isOnEmergency()) {
          screen.notConnectedToSever();
          screen.setEmergency(true);
        }
      } else if (screen.isOnEmergency()) {
        screen.setEmergency(false);
        api.requestLineData();
        screen.tapCard();
      }

      // Servir orden pendiente
      if (line.theresPendingOrder()) {
        currentState = State::SERVE_PENDING;
        break;
      }

      // Interacción usuario + lector
      handleTouch();
      if (reader.on()) {
        if (line.compareEmergencyCard(reader.getCardString()) && !api.isConnected())
          reader.setEmergency();
        else {
          api.fetchCardId(reader.getCardString());
          screen.LoadingModal();
          if (!reader.theresUser()) screen.LockScreen();
        }
      }
      if (remote_sell) handleTouch(true);
      if (reader.theresUser()) {
        screen.SelectQty(reader.getUser());
        lineUnlocked();
      }
      break;

    // — SERVE_PENDING: servir orden del servidor —
    case State::SERVE_PENDING:
      screen.isServing = true;
      {
        auto order = line.getPouringOrder();
        bool finished = countQty("", order.ml);
        commitPurchase(
          order.concept,
          String(pulse_counter / (screen.ppm * 1000)),
          order.user
        );
        line.removePouringOrder();
        screen.LockScreen();
        pulse_counter = 0;
      }
      currentState = State::IDLE;
      break;

    // — ERROR: reinicio —
    case State::ERROR:
      ESP.restart();
      break;
  }

  // Cede tiempo a otras tareas FreeRTOS
  vTaskDelay(25 / portTICK_PERIOD_MS);
}

//-------------------------------------->Socket Handlers

void event(const char* payload, size_t length) {
  DEBUG(("got message:"+ String(payload)).c_str());
}

void onNewEmergencyCard(const char* payload, size_t length) {
  line.saveEmergencyCard(payload);
}

void onConnect(const char* payload, size_t length) {
  if (!line.isConnected()) api.requestLineData();
}

void onDisconnect(const char* payload, size_t length) {
  // no-op
}

void onRemoteSell(const char* payload, size_t length) {
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed");
    DEBUG(error.c_str());
  }
  JsonObject json_response = doc.as<JsonObject>();
  if (json_response["confirmation"].as<String>() == "success") {
    JsonObject userData = json_response["data"].as<JsonObject>();
    String user     = userData["nombre"].as<String>() + " " + userData["apellidos"].as<String>();
    String workerId = userData["_id"].as<String>();
    uint8_t concept = json_response["concept"].as<int>();
    reader.setUser(user, workerId);
    remote_sell   = true;
    remote_concept = concept;
  }
}

void onDisconnectedLine(const char* payload, size_t length) {
  screen.removeInfo();
  reset = true;
  line.setLineStatus(DISCONNECTED);
  DEBUG("tiene que estar desconectada");
}

void onInfoRecived(const char* payload, size_t length) {
  screen.setInfo(payload);
  line.saveInfo(payload);
  line.setLineStatus(CONNECTED);
  String new_card = screen.emergencyCard;
  String old_card = line.getEmergencyCardFromMemory();
  DEBUG(old_card.c_str());
  if (!line.compareEmergencyCard(new_card))
    line.saveEmergencyCard(new_card.c_str());
  reset = true;
}

void onClaimBeer(const char* payload, size_t length) {
  reader.setClient("N/A", "N/A");
  reader.setClient("", String(payload));
  DEBUG("esto valio re quete verga");
  remote_sell = redeem_beer = true;
}

void onLineChange(const char* payload, size_t length) {
  api.requestLineData();
}

void validateResponse(const char* payload, size_t length) {
  DEBUG(payload);
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed");
    DEBUG(error.c_str());
  }
  JsonObject json_response = doc.as<JsonObject>();
  if (json_response["confirmation"].as<String>() == "success") {
    JsonObject userData = json_response["data"].as<JsonObject>();
    String user     = userData["nombre"].as<String>() + " " + userData["apellidos"].as<String>();
    String workerId = userData["_id"].as<String>();
    reader.setUser(user, workerId);
  }
}

void validateClient(const char* payload, size_t length) {
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed");
    DEBUG(error.c_str());
  }
  JsonObject json_response = doc.as<JsonObject>();
  if (json_response["confirmation"].as<String>() == "success") {
    JsonObject userData = json_response["data"].as<JsonObject>();
    String client   = userData["name"].as<String>() + " " + userData["lastName"].as<String>();
    String clientId = userData["_id"].as<String>();
    reader.setClient(client, clientId);
  }
}

void requestDevice(const char* payload, size_t length) {
  if (screen.isServing) {
    api.rejectOrder();
    return;
  }
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed");
    DEBUG(error.c_str());
  }
  JsonObject json_response = doc.as<JsonObject>();
  uint16_t volume_ml = json_response["volume"].as<uint16_t>();
  String   user      = json_response["userId"].as<String>();
  String   concept   = json_response["concept"].as<String>();
  api.confirmOrder(user);
  line.setPoruingOrder(user, volume_ml, concept);
}

void startPour(const char* payload, size_t length) {
  if (screen.isServing) return;
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  screen.hideLoadingModal();
  if (error) {
    DEBUG("deserializeJson() failed");
    DEBUG(error.c_str());
  }
  JsonObject json_response = doc.as<JsonObject>();
  uint16_t volume_ml = json_response["volume"].as<uint16_t>();
  String   user      = json_response["userId"].as<String>();
  String   concept   = json_response["concept"].as<String>();
  line.setPoruingOrder(user, volume_ml, concept);
}

void stopPour(const char* payload, size_t length) {
  if (!screen.isServing) return;
  screen.isServing = false;
}

//-------------------------------------->Async Task for Socket Loop
void socketManager(void* pvParameters) {
  while (1) {
    api.loop();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
 
//-------------------------------------->UI for Wi-Fi Setup
void SetConnectedScreen(bool retriable) {
  screen.AP(wifi.ap_name, retriable);
  DEBUG(wifi.ap_name.c_str());
  if (retriable)
    wifiManager.startConfigPortal(wifi.ap_name.c_str(), SECRET_PASS, handleTouch);
  else
    wifiManager.startConfigPortal(wifi.ap_name.c_str(), SECRET_PASS);
}

//-------------------------------------->Helper Functions
void setUpWiFi() {
  if (wifi.theresValidSSID()) {
    vTaskDelay(random(2000) / portTICK_PERIOD_MS);
    screen.connecting();
    if (wifi.setUpWiFi()) setUpSocketConnection();
    else bootOptions();
  } else {
    SetConnectedScreen();
  }
  DEBUG("Salio");
  vTaskDelay(2000 / portTICK_PERIOD_MS);
}

void bootOptions() {
  screen.retryOrAP();
  while (screen.actualScreen == RETRY_AP_SCR && line.theresNoInfo())
    handleTouch();
}

void setUpSocketConnection() {
  api.on(CONNECT,            onConnect);
  api.on(CLAIM_BEER,         onClaimBeer);
  api.on(DISCONNECT,         onDisconnect);
  api.on(CHANGE_LINE,        onLineChange);
  api.on(REMOTE_SELL,        onRemoteSell);
  api.on(DEVICE_INFO,        onInfoRecived);
  api.on(VALIDATED_CLIENT,   validateClient);
  api.on(VALIDATED_USER,     validateResponse);
  api.on(DISCONNECTED_LINE,   onDisconnectedLine);
  api.on(ADD_EMERGENCY_CARD, onNewEmergencyCard);
  api.on(START_POUR,         startPour);
  api.on(STOP_POUR,          stopPour);
  api.on(REQUEST_DEVICE,     requestDevice);

  api.connect(wifi.getIP().c_str());
  xTaskCreatePinnedToCore(socketManager, "Socket loop", 16384, NULL, 1, NULL, CORE0);
}

void filling(uint16_t pulses) {
  // —— Reset del contador de forma atómica ——
  portENTER_CRITICAL(&muxCounter);
  pulse_counter = 0;
  portEXIT_CRITICAL(&muxCounter);

  uint8_t last_percent = 0;
  const uint8_t incrementor = (pulses / screen.ppm) < 200 ? 5 : 2;
  uint32_t time_out = millis() + SERVING_TIME_OUT;

  while (screen.isServing && (safeReadCounter() < pulses) && millis() < time_out) {
    uint16_t count = safeReadCounter();

    // permite cancelar o navegar menú
    if (count < screen.ppm * 20 || last_percent > 69) {
      handleTouch();
    }

    // cálculo de porcentaje
    uint8_t percent = (uint8_t)((100 * count) / pulses);
    if (percent > last_percent + incrementor) {
      api.updateStatus(count / screen.ppm, wifi.macAddress());
      if (percent == 70) screen.showReady();
      last_percent = percent;
      time_out = millis() + SERVING_TIME_OUT;
      screen.servingScreen(false, percent, "");
    }
  }

  // tras el bucle, asegúrate de cerrar válvula fuera de aquí
  selecting_opt = false;
  if (last_percent > 70 && !screen.isServing) {
    screen.isServing = true;
  }
}

// Lectura atómica del contador
uint16_t safeReadCounter() {
  uint16_t val;
  portENTER_CRITICAL(&muxCounter);
  val = pulse_counter;
  portEXIT_CRITICAL(&muxCounter);
  return val;
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

void IRAM_ATTR flowCounter() {
  portENTER_CRITICAL_ISR(&muxCounter);
  pulse_counter++;
  portEXIT_CRITICAL_ISR(&muxCounter);
}

uint16_t mermando() {
  unsigned int last_val = 0;
  pulse_counter = 0;
  bool merma = screen.isMermando;
  uint32_t time_out = millis() + SERVING_TIME_OUT;

  while ((screen.isMermando || screen.isCalibrating) && time_out > millis()) {
    handleTouch();
    unsigned int val = merma ? pulse_counter / screen.ppm : pulse_counter;
    if (val > last_val) {
      last_val = val;
      time_out = millis() + SERVING_TIME_OUT;
      screen.drawMl(val, merma);
    }
  }
  selecting_opt = false;
  return pulse_counter;
}

void commitPurchase(String concept, String qty, String user) {
  if (!reader.isOnEmergency()) {
    String worker_id = reader.getWorkerId();
    String client_id = reader.getClientId();
    String fixed_user = user.length() > 1 ? user : worker_id;
    api.registerPurchase(client_id, fixed_user, concept, qty, screen.kegId);
  }
  screen.isServing = false;
  reader.removeUser();
}

void handleTouch(bool remote) {
  int8_t btn = remote ? remote_concept : screen.isPressed();
  if (remote) {
    reset = true;
    remote_sell = false;
    remote_concept = NON_SELECTED;
  }
  if (btn != NON_SELECTED) {
    if      (btn == Vaso)      screen.ServeOptionsGlass();
    else if (btn == Taster)    screen.ServeOptionsTaster();
    else if (btn == Growler)   screen.ServeOptionsGrowler();
    else if (btn == Mermar) {
      uint16_t merma_pulses = countQty(false);
      if (merma_pulses > 50)
        commitPurchase("MERMA", String(merma_pulses/(screen.ppm*1000)));
      else
        reader.removeUser();
    }
    else if (btn == Cancelar) {
      bool remove = reader.getWorkerId().length() < 4 && !reader.isOnEmergency();
      if (!screen.isServing) reader.removeUser();
      screen.isServing = selecting_opt = false;
      if (remove) screen.LockScreen();
    }
    else if (btn == Listo) {
      if (selecting_opt) screen.isMermando = selecting_opt = false;
      else               screen.isCalibrating = false;
    }
    else if (btn == Oz_2 ) if (countQty("Taster 2oz", 60))  commitPurchase("TASTER", ".06");
    else if (btn == Oz_5 ) if (countQty("Taster 5oz",142)) commitPurchase("TASTER", ".142");
    else if (btn == Oz_8 ) if (countQty("Medio Vaso 8oz",236)) commitPurchase("PINT", ".236");
    else if (btn == Oz_10) if (countQty("Vaso 10oz",296)) commitPurchase("PINT", ".296");
    else if (btn == Oz_16) if (countQty("Vaso 16oz",473)) commitPurchase("PINT", ".473");
    else if (btn == Oz_32) if (countQty("Growler 32oz",1000)) commitPurchase("GROWLER", "1");
    else if (btn == Oz_64) if (countQty("Growler 64oz",2000)) commitPurchase("GROWLER", "2");
    else if (btn == Oz_128) if (countQty("Growler 128oz",4000)) commitPurchase("GROWLER", "4");
    else if (btn == cog)        screen.Settings();
    else if (btn == Calibrar) {
      uint16_t merma_pulses = countQty(true);
      screen.saveCalibration(merma_pulses);
    }
    else if (btn == Actualizar) otaUpdating();
    else if (btn == Retry)     { DEBUG("puto"); ESP.restart(); }
    else if (btn == Back || btn == Descartar) screen.LockScreen();
    else if (btn == Guardar) {
      float new_ppm = pulse_counter / 300.0;
      DEBUG(("Guardando ppm: " + String(new_ppm)).c_str());
      line.savePPM(new_ppm);
      screen.ppm = new_ppm;
      screen.LockScreen();
    }
    else if (btn == CONFIGURAR_WIFI) { DEBUG("CONFIGURAR_WIFI"); SetConnectedScreen(true); }
    else if (btn == BOOT_WITH_FILE) {
      DEBUG("BOOT_WITH_FILE");
      const char* line_data = line.getInfoFromSF().c_str();
      screen.setInfo(line_data);
    }
  }
  else if (redeem_beer) {
    redeem_beer = reset = false;
    if (countQty("Vaso 16oz", 473)) {
      api.redeemBeer(reader.getClientId(), screen.kegId);
      reader.removeUser();
      screen.LockScreen();
    }
  }
}

bool countQty(String screen_msg, uint16_t ml) {
  attachInterrupt(FLOWMETER_PIN, flowCounter, RISING);
  digitalWrite(VALVE_PIN, HIGH);
  screen.servingScreen(true, 0, screen_msg);
  filling(screen.ppm * ml);
  digitalWrite(VALVE_PIN, LOW);
  return screen.isServing;
}

uint16_t countQty(bool calibrate) {
  attachInterrupt(FLOWMETER_PIN, flowCounter, RISING);
  digitalWrite(VALVE_PIN, HIGH);
  if (calibrate) screen.calibrationScreen(0);
  else           screen.mermar(0);
  uint16_t merma_pulses = mermando();
  digitalWrite(VALVE_PIN, LOW);
  return merma_pulses;
}

void otaUpdating() {
  screen.otaUpdateScreen();
  wifi.setUpOTA(screen.noLinea);
  while (screen.actualScreen == OTA_UPDATE_SCR) {
    handleTouch();
    wifi.loopOTA();
  }
  wifi.stopOTA();
}

JsonObject decodeJson(const char* payload) {
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, payload);
  if (error) {
    DEBUG("deserializeJson() failed");
    DEBUG(error.c_str());
  }
  JsonObject json_response = doc.as<JsonObject>();
  doc.clear();
  return json_response;
}

bool validateJsonResponse(JsonObject json_response) {
  return strcmp(json_response["confirmation"], "success") == 0;
}

void DEBUG(const char* message) {
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "[Main]: %s", message);
  logger.println(buffer);
}
