#include "WIFI.h"
#include "WiFiType.h"

bool WIFI::setUpWiFi(uint8_t max_tries){
  WiFi.begin();
  uint8_t notConnectedCounter = 0;
  bool connection_state = true;
  DEBUG("WiFi Connecting...");
  // EEPROM.begin(512); 
  preferences.begin("my-app", false);
  while (!isConnected() && connection_state) {
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    logger.print(".");  
    notConnectedCounter++;
    if(notConnectedCounter > 3) { // Reset board if not connected after 5s
      DEBUG("Resetting due to WiFi not connecting...");
      // uint8_t num_of_tries = EEPROM.readInt(WIFI_RETRIES_ADD);
      uint8_t num_of_tries = preferences.getInt("wifi_retries", 0);
      DEBUG(String(num_of_tries).c_str());
      if (num_of_tries == max_tries) connection_state = false;
      else {
        preferences.putInt("wifi_retries", num_of_tries + 1);
        preferences.end();
        // EEPROM.writeInt(WIFI_RETRIES_ADD, num_of_tries + 1);
        // EEPROM.commit();
        // EEPROM.end();
        ESP.restart();          
      }
    }
  }

  preferences.putInt("wifi_retries", 0);
  preferences.end();
  // EEPROM.writeInt(WIFI_RETRIES_ADD, 0);
  // EEPROM.commit();
  // EEPROM.end();

  if(connection_state) {
    startMDNS();
    DEBUG("Connected");
  }
  return connection_state;
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