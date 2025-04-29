#include "WIFI.h"
#include "WiFiType.h"

AsyncWebServer server(80);

static void recvMsg(uint8_t *data, size_t len){
  // WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  logger.setMessage(d);
  WebSerial.println(d);
}

#define MAX_REBOOT 3

bool WIFI::setUpWiFi(uint8_t max_tries, const char* ssid, const char* pass){
  preferences.begin("wifi", false);
  int rebootCount = preferences.getInt("reboots", 0);

  // 1) Intento de conexión (nuevas credenciales o guardadas)
  if (ssid && pass) {
    DEBUG("Connecting with provided creds");
    WiFi.begin(ssid, pass);
  } else {
    DEBUG("Connecting with saved creds");
    WiFi.begin();  // usa flash-stored SSID/PASS
  }

  // 2) Esperar hasta max_tries intentos
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && ++attempts <= max_tries) {
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    logger.print(".");
  }

  // 3a) Si conecta, limpiamos contador y seguimos
  if (isConnected()) {
    preferences.putInt("reboots", 0);
    preferences.end();
    DEBUG("WiFi connected!");
    String ipStr = WiFi.localIP().toString();  // crea un String
    DEBUG(ipStr.c_str());
    startMDNS();
    if (logger.currentOutput == Logger::WEBSERIAL)
      setUpWebServer(true);
    return true;
  }

  // 3b) Si falla:
  DEBUG("WiFi failed to connect.");

  // → si no superamos MAX_REBOOT, incrementamos contador y reiniciamos
  if (rebootCount < MAX_REBOOT) {
    DEBUG("Rebooting to retry WiFi...");
    preferences.putInt("reboots", rebootCount + 1);
    preferences.end();
    delay(200);
    ESP.restart();
    return false;  // (no llega aquí)
  }

  // → si ya pasamos MAX_REBOOT, limpiamos contador y levantamos AP
  DEBUG("Max retries reached. Starting AP mode.");
  preferences.putInt("reboots", 0);
  preferences.end();

  eneableAP();
  return false;
}

void WIFI::eneableAP(){
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP("test", "S=klogw2", /*channel=*/1, /*hidden=*/true);
  setUpWebServer(true);
  DEBUG("AP started");
}

void WIFI::setUpWebServer(bool serial){
  if (serial) {
    WebSerial.begin(&server);
    WebSerial.onMessage(recvMsg);
  }
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(200, "text/plain", "Test OK");
  });

  server.on("/wifisave", HTTP_GET, [this](AsyncWebServerRequest *req){
    if (!req->hasParam("s") || !req->hasParam("p")) {
      return req->send(400, "application/json",
                       "{\"error\":\"missing ssid or password\"}");
    }
    String ssid = req->getParam("s")->value();
    String pass = req->getParam("p")->value();

    // Intentar conexión inmediata
    WiFi.begin(ssid.c_str(), pass.c_str());
    uint8_t t = 0;
    while (WiFi.status() != WL_CONNECTED && t++ < 5) {
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      req->send(200, "application/json", "{\"id\":\""+WiFi.macAddress()+"\"}");
      delay(300);
      ESP.restart();
    } else {
      req->send(500, "application/json",
                "{\"error\":\"connection failed, AP still live\"}");
    }
  });

  server.begin();
}

void WIFI::setUpOTA(uint8_t tap_number){
    String formated_name = "Tap-o-meter_";
    formated_name += tap_number > 0 ? tap_number : random(255);
    ArduinoOTA.setHostname(formated_name.c_str());
    ArduinoOTA.onStart([&]() {
    String type;
    type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    DEBUG(("Start updating " + type).c_str());
    }).onEnd([&]() {
      DEBUG("\nEnd");
    }).onProgress([&](unsigned int progress, unsigned int total) {
      Serial2.printf("Progress: %u%%\r", (progress / (total / 100)));
    }).onError([&](ota_error_t error) {
      logger.print("Error: ");
      logger.println(String(error));

      if (error == OTA_AUTH_ERROR) DEBUG("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) DEBUG("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) DEBUG("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) DEBUG("Receive Failed");
      else if (error == OTA_END_ERROR) DEBUG("End Failed");
    });
    ArduinoOTA.begin();
}

void WIFI::loopOTA(){
  ArduinoOTA.handle();
}

void WIFI::startMDNS(){
  if (!MDNS.begin("tap-o-meter")) {
        DEBUG("Error setting up MDNS responder!");
        while(1) vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

String WIFI::getIP(){
  String ip = MDNS.queryHost("tom-server").toString();
  DEBUG(ip.c_str());
  return ip;
}

bool WIFI::theresValidSSID(){
  WiFi.mode(WIFI_STA);
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);
  const String SSID = String(reinterpret_cast<const char*>(conf.sta.ssid));
  return SSID.length() > 2;
}

void WIFI::resetWiFiSettings(){
  DEBUG("settings invalidated");
  DEBUG("THIS MAY CAUSE AP NOT TO START UP PROPERLY. YOU NEED TO COMMENT IT OUT AFTER ERASING THE DATA.");
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //load the flash-saved configs
    esp_wifi_init(&cfg); //initiate and allocate wifi resources (does not matter if connection fails)
    delay(2000); //wait a bit
    if(esp_wifi_restore()!=ESP_OK)
    {
        DEBUG("WiFi is not initialized by esp_wifi_init ");
      }else{
          DEBUG("WiFi Configurations Cleared!");
      }
      //continue
    delay(1000);
    esp_restart(); //just my reset configs routine...
  delay(300);
}

void WIFI::reconnect(bool not_blocking){
  if (not_blocking){
    logger.print(".");
    if (retry_connection_in < millis() || retry_connection_in == 0){
      WiFi.begin();
      DEBUG("Thick");
      retry_connection_in = millis() + RETRY_TIME;
    }
  }
  else {
    WiFi.begin();
    // uint8_t timeout = 0;
    // vTaskDelay( 2000 );

    while ( WiFi.status() != WL_CONNECTED ){
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      // vTaskDelay( 2000 );
      log_i(" waiting on wifi connection" );
      // timeout++;
      // if (timeout == 2) return;
    }
  }
  
}

void WIFI::stopOTA(){
  ArduinoOTA.end();
}

bool WIFI::isConnected(){
  return WiFi.status() == WL_CONNECTED;
}

String WIFI::macAddress(){
  return WiFi.macAddress();
}

void WIFI::DEBUG(const char *message){
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "[WIFI]: %s", message);
  logger.println(buffer);
}