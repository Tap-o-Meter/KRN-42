/**************************************************************
   WiFiManager32 is a library for the ESP8266/Arduino platform
   (https://github.com/esp8266/Arduino) to enable easy
   configuration and reconfiguration of WiFi credentials using a Captive Portal
   inspired by:
   http://www.esp8266.com/viewtopic.php?f=29&t=2520
   https://github.com/chriscook8/esp-arduino-apboot
   https://github.com/esp8266/Arduino/tree/master/libraries/DNSServer/examples/CaptivePortalAdvanced
   Built by AlexT https://github.com/tzapu
   Licensed under MIT license
 **************************************************************/

#include "WiFiManager32.h"

WiFiManager32Parameter::WiFiManager32Parameter(const char *custom) {
  _id = NULL;
  _placeholder = NULL;
  _length = 0;
  _value = NULL;

  _customHTML = custom;
}

WiFiManager32Parameter::WiFiManager32Parameter(const char *id, const char *placeholder, const char *defaultValue, int length) {
  init(id, placeholder, defaultValue, length, "");
}

WiFiManager32Parameter::WiFiManager32Parameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom) {
  init(id, placeholder, defaultValue, length, custom);
}

void WiFiManager32Parameter::init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom) {
  _id = id;
  _placeholder = placeholder;
  _length = length;
  _value = new char[length + 1];
  for (int i = 0; i < length + 1; i++) {
    _value[i] = 0;
  }
  if (defaultValue != NULL) {
    strncpy(_value, defaultValue, length);
  }

  _customHTML = custom;
}

WiFiManager32Parameter::~WiFiManager32Parameter() {
  if (_value != NULL) {
    delete[] _value;
  }
}
const char* WiFiManager32Parameter::getValue() {
  return _value;
}
const char* WiFiManager32Parameter::getID() {
  return _id;
}
const char* WiFiManager32Parameter::getPlaceholder() {
  return _placeholder;
}
int WiFiManager32Parameter::getValueLength() {
  return _length;
}
const char* WiFiManager32Parameter::getCustomHTML() {
  return _customHTML;
}


WiFiManager32::WiFiManager32() {
    _max_params = WIFI_MANAGER_MAX_PARAMS;
    _params = (WiFiManager32Parameter**)malloc(_max_params * sizeof(WiFiManager32Parameter*));
}

WiFiManager32::~WiFiManager32()
{
    if (_params != NULL)
    {
        DEBUG_WM(F("freeing allocated params!"));
        free(_params);
    }
}

String WiFiManager32::getSsid(){
  WiFi.mode(WIFI_STA);
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);
  return String(reinterpret_cast<const char*>(conf.sta.ssid));
}

bool WiFiManager32::addParameter(WiFiManager32Parameter *p) {
  if(_paramsCount + 1 > _max_params)
  {
    // rezise the params array
    _max_params += WIFI_MANAGER_MAX_PARAMS;
    DEBUG_WM(F("Increasing _max_params to:"));
    DEBUG_WM(_max_params);
    WiFiManager32Parameter** new_params = (WiFiManager32Parameter**)realloc(_params, _max_params * sizeof(WiFiManager32Parameter*));
    if (new_params != NULL) {
      _params = new_params;
    } else {
      DEBUG_WM(F("ERROR: failed to realloc params, size not increased!"));
      return false;
    }
  }

  _params[_paramsCount] = p;
  _paramsCount++;
  DEBUG_WM(F("Adding parameter"));
  DEBUG_WM(p->getID());
  return true;
}

void WiFiManager32::setupConfigPortal() {
  dnsServer.reset(new DNSServer());
  server.reset(new WebServer(80));

  DEBUG_WM(F(""));
  _configPortalStart = millis();

  DEBUG_WM(F("Configuring access point... "));
  DEBUG_WM(_apName);
  if (_apPassword != NULL) {
    if (strlen(_apPassword) < 8 || strlen(_apPassword) > 63) {
      // fail passphrase to short or long!
      DEBUG_WM(F("Invalid AccessPoint password. Ignoring"));
      _apPassword = NULL;
    }
    DEBUG_WM(_apPassword);
  }

  //optional soft ip config
  if (_ap_static_ip) {
    DEBUG_WM(F("Custom AP IP/GW/Subnet"));
    WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn);
  }

  if (_apPassword != NULL) {
    //WiFi.softAP(_apName, _apPassword);//password option
    WiFi.softAP(_apName, _apPassword, 1, true, 1);
  } else {
    WiFi.softAP(_apName);
  }

  delay(500); // Without delay I've seen the IP address blank
  DEBUG_WM(F("AP IP address: "));
  DEBUG_WM(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  //dnsServer->start(DNS_PORT, "*", WiFi.softAPIP()); -------------> aquí se desactiva le captive

  //server->on(String(F("/")), std::bind(&WiFiManager32::handleWifi, this, false));
  server->on(String(F("/wifisave")), std::bind(&WiFiManager32::handleWifiSave, this));
  server->on(String(F("/i")), std::bind(&WiFiManager32::handleInfo, this));
  server->on(String(F("/r")), std::bind(&WiFiManager32::handleReset, this));
  server->begin(); // Web server start
  DEBUG_WM(F("HTTP server started!"));
}

boolean WiFiManager32::autoConnect() {
  String ssid = "ESP" + String((uint32_t)ESP.getEfuseMac(), HEX);
  return autoConnect(ssid.c_str(), NULL);
}

boolean WiFiManager32::autoConnect(char const *apName, char const *apPassword) {
  DEBUG_WM(F(""));
  DEBUG_WM(F("AutoConnect"));

  // read eeprom for ssid and pass
  //String ssid = getSSID();
  //String pass = getPassword();

  // attempt to connect; should it fail, fall back to AP
  WiFi.mode(WIFI_STA);

  if (connectWifi("", "") == WL_CONNECTED)   {
    DEBUG_WM(F("IP Address:"));
    DEBUG_WM(WiFi.localIP());
    //connected
      DEBUG_WM("hasta aquí sigue vivo!");
    return true;
  }

  return startConfigPortal(apName, apPassword);
}

boolean WiFiManager32::configPortalHasTimeout(){
    if(_configPortalTimeout == 0 || WiFi.softAPgetStationNum() > 0){
      _configPortalStart = millis(); // kludge, bump configportal start time to skew timeouts
      return false;
    }
    return (millis() > _configPortalStart + _configPortalTimeout);
}

boolean WiFiManager32::startConfigPortal() {
  String ssid = "ESP" + String((uint32_t)ESP.getEfuseMac(), HEX);
  return startConfigPortal(ssid.c_str(), NULL);
}

boolean  WiFiManager32::startConfigPortal(char const *apName, char const *apPassword) {

  if(!WiFi.isConnected()){
    WiFi.persistent(false);
    // disconnect sta, start ap
    WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
    WiFi.mode(WIFI_AP);
    WiFi.persistent(true);
  }
  else {
    //setup AP
    WiFi.mode(WIFI_AP_STA);
    DEBUG_WM(F("SET AP STA"));
  }


  _apName = apName;
  _apPassword = apPassword;

  //notify we entered AP mode
  if ( _apcallback != NULL) {
    _apcallback(this);
  }

  connect = false;
  setupConfigPortal();

  while(1){

    // check if timeout
    if(configPortalHasTimeout()) break;

    //DNS
    dnsServer->processNextRequest();
    //HTTP
    server->handleClient();


    if (connect) {
      connect = false;
      delay(2000);
      DEBUG_WM(F("Connecting to new AP"));

      // using user-provided  _ssid, _pass in place of system-stored ssid and pass
      if (connectWifi(_ssid, _pass) != WL_CONNECTED) {
        DEBUG_WM(F("Failed to connect."));

        WiFi.disconnect(true);
      } else {
        //connected
        WiFi.mode(WIFI_STA);
        //notify that configuration has changed and any optional parameters should be saved
        if ( _savecallback != NULL) {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
        break;
      }

      if (_shouldBreakAfterConfig) {
        //flag set to exit after config after trying to connect
        //notify that configuration has changed and any optional parameters should be saved
        if ( _savecallback != NULL) {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
        break;
      }
    }
    yield();
  }

  server.reset();
  dnsServer.reset();

  return  WiFi.status() == WL_CONNECTED;
}

boolean  WiFiManager32::startConfigPortal(char const *apName, char const *apPassword, std::function<void (bool remote)> loop) {
  if(!WiFi.isConnected()){
    WiFi.persistent(false);
    // disconnect sta, start ap
    WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
    WiFi.mode(WIFI_AP);
    WiFi.persistent(true);
  }
  else {
    //setup AP
    WiFi.mode(WIFI_AP_STA);
    DEBUG_WM(F("SET AP STA"));
  }


  _apName = apName;
  _apPassword = apPassword;

  //notify we entered AP mode
  if ( _apcallback != NULL) {
    _apcallback(this);
  }

  connect = false;
  setupConfigPortal();

  while(1){

    loop(false);

    // check if timeout
    if(configPortalHasTimeout()) break;

    //DNS
    dnsServer->processNextRequest();
    //HTTP
    server->handleClient();


    if (connect) {
      connect = false;
      delay(2000);
      DEBUG_WM(F("Connecting to new AP"));

      // using user-provided  _ssid, _pass in place of system-stored ssid and pass
      if (connectWifi(_ssid, _pass) != WL_CONNECTED) {
        DEBUG_WM(F("Failed to connect."));

        WiFi.disconnect(true);
      } else {
        //connected
        WiFi.mode(WIFI_STA);
        //notify that configuration has changed and any optional parameters should be saved
        if ( _savecallback != NULL) {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
        break;
      }

      if (_shouldBreakAfterConfig) {
        //flag set to exit after config after trying to connect
        //notify that configuration has changed and any optional parameters should be saved
        if ( _savecallback != NULL) {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
        break;
      }
    }
    yield();
  }

  server.reset();
  dnsServer.reset();

  return  WiFi.status() == WL_CONNECTED;
}


int WiFiManager32::connectWifi(String ssid, String pass) {
  DEBUG_WM(F("Connecting as wifi client..."));

  // check if we've got static_ip settings, if we do, use those.
  if (_sta_static_ip) {
    DEBUG_WM(F("Custom STA IP/GW/Subnet"));
    WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
    DEBUG_WM(WiFi.localIP());
  }
  //fix for auto connect racing issue
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_WM(F("Already connected. Bailing out."));
    return WL_CONNECTED;
  }
  //check if we have ssid and pass and force those, if not, try with last saved values
  if (ssid != "") {
    int intents = 0;
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid.c_str(), pass.c_str()); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial2.print(".");
      intents++;
      if (intents > 30) break;
      delay(2000);
    }
  } else {
    if (getSsid()) {
      DEBUG_WM(F("Using last saved values, should be faster"));
      //trying to fix connection in progress hanging
      int intents = 0;
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
        Serial2.print(".");
        intents++;
        if (intents > 30) break;
        delay(2000);
      }
    } else {
      DEBUG_WM(F("No saved credentials"));
    }
  }

  int connRes = WiFi.status();;
  DEBUG_WM ("Connection result: ");
  DEBUG_WM ( connRes );
  //not connected, WPS enabled, no pass - first attempt
  #ifdef NO_EXTRA_4K_HEAP
  if (_tryWPS && connRes != WL_CONNECTED && pass == "") {
    startWPS();
    //should be connected at the end of WPS
    connRes = waitForConnectResult();
  }
  #endif
  return connRes;
}

uint8_t WiFiManager32::waitForConnectResult() {
  long timeOutTime = 20000;
    DEBUG_WM (F("Waiting for connection result with time out"));
    unsigned long start = millis();
    boolean keepConnecting = true;
    uint8_t status;
    while (keepConnecting) {
      status = WiFi.status();
      if (millis() > start + timeOutTime) {
        keepConnecting = false;
        DEBUG_WM (F("Connection timed out"));
      }
      if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
        keepConnecting = false;
      }
      delay(100);
    }
    return status;

}

void WiFiManager32::startWPS() {
  DEBUG_WM(F("START WPS"));
  //WiFi.beginWPSConfig();
  DEBUG_WM(F("END WPS"));
}

String WiFiManager32::getConfigPortalSSID() {
  return _apName;
}
String WiFiManager32::getUserId() {
  return _idUser;
}
String WiFiManager32::getDeviceName() {
  return _deviceName;
}
String WiFiManager32::getLastSSID(){
	return getSsid();
}
String WiFiManager32::getMac(){
  return WiFi.macAddress();
}

void WiFiManager32::resetSettings() {
  DEBUG_WM(F("settings invalidated"));
  DEBUG_WM(F("THIS MAY CAUSE AP NOT TO START UP PROPERLY. YOU NEED TO COMMENT IT OUT AFTER ERASING THE DATA."));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //load the flash-saved configs
        esp_wifi_init(&cfg); //initiate and allocate wifi resources (does not matter if connection fails)
        delay(2000); //wait a bit
        if(esp_wifi_restore()!=ESP_OK)
        {
            DEBUG_WM(F("WiFi is not initialized by esp_wifi_init "));
         }else{
             DEBUG_WM(F("WiFi Configurations Cleared!"));
         }
         //continue
        delay(1000);
        esp_restart(); //just my reset configs routine...
  delay(300);
}
void WiFiManager32::setTimeout(unsigned long seconds) {
  setConfigPortalTimeout(seconds);
}

void WiFiManager32::setConfigPortalTimeout(unsigned long seconds) {
  _configPortalTimeout = seconds * 1000;
}

void WiFiManager32::setConnectTimeout(unsigned long seconds) {
  _connectTimeout = seconds * 1000;
}

void WiFiManager32::setDebugOutput(boolean debug) {
  _debug = debug;
}

void WiFiManager32::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
  _ap_static_ip = ip;
  _ap_static_gw = gw;
  _ap_static_sn = sn;
}

void WiFiManager32::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
  _sta_static_ip = ip;
  _sta_static_gw = gw;
  _sta_static_sn = sn;
}

void WiFiManager32::setMinimumSignalQuality(int quality) {
  _minimumQuality = quality;
}

void WiFiManager32::setBreakAfterConfig(boolean shouldBreak) {
  _shouldBreakAfterConfig = shouldBreak;
}

/** Wifi config page handler */
void WiFiManager32::handleWifi(boolean scan) {

}

/** Handle the WLAN save form and redirect to WLAN config page again */
void WiFiManager32::handleWifiSave() {
  DEBUG_WM(F("WiFi save"));

  //SAVE/connect here
  _ssid = server->arg("s").c_str();
  _pass = server->arg("p").c_str();
  _idUser = server->arg("u").c_str();
  _deviceName = server->arg("n").c_str();


  //parameters
  for (int i = 0; i < _paramsCount; i++) {
    if (_params[i] == NULL) {
      break;
    }
    //read parameter
    String value = server->arg(_params[i]->getID()).c_str();
    //store it in array
    value.toCharArray(_params[i]->_value, _params[i]->_length + 1);
    DEBUG_WM(F("Parameter"));
    DEBUG_WM(_params[i]->getID());
    DEBUG_WM(value);
  }

  if (server->arg("ip") != "") {
    DEBUG_WM(F("static ip"));
    DEBUG_WM(server->arg("ip"));
    //_sta_static_ip.fromString(server->arg("ip"));
    String ip = server->arg("ip");
    optionalIPFromString(&_sta_static_ip, ip.c_str());
  }
  if (server->arg("gw") != "") {
    DEBUG_WM(F("static gateway"));
    DEBUG_WM(server->arg("gw"));
    String gw = server->arg("gw");
    optionalIPFromString(&_sta_static_gw, gw.c_str());
  }
  if (server->arg("sn") != "") {
    DEBUG_WM(F("static netmask"));
    DEBUG_WM(server->arg("sn"));
    String sn = server->arg("sn");
    optionalIPFromString(&_sta_static_sn, sn.c_str());
  }

  String page = "{\"id\":\""+WiFi.macAddress()+"\"}";

  server->sendHeader("Content-Length", String(page.length()));
  server->send(200, "text/html", page);

  DEBUG_WM(F("Sent wifi save page"));

  connect = true; //signal ready to connect/reset
}

/** Handle the info page */
void WiFiManager32::handleInfo() {


  DEBUG_WM(F("Sent info page"));
}

/** Handle the reset page */
void WiFiManager32::handleReset() {
  DEBUG_WM(F("Reset"));


  DEBUG_WM(F("Sent reset page"));
  delay(5000);
  ESP.restart();
  delay(2000);
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */


//start up config portal callback
void WiFiManager32::setAPCallback( void (*func)(WiFiManager32* myWiFiManager32) ) {
  _apcallback = func;
}

//start up save config callback
void WiFiManager32::setSaveConfigCallback( void (*func)(void) ) {
  _savecallback = func;
}

//sets a custom element to add to head, like a new style tag
void WiFiManager32::setCustomHeadElement(const char* element) {
  _customHeadElement = element;
}

//if this is true, remove duplicated Access Points - defaut true
void WiFiManager32::setRemoveDuplicateAPs(boolean removeDuplicates) {
  _removeDuplicateAPs = removeDuplicates;
}



template <typename Generic>
void WiFiManager32::DEBUG_WM(Generic text) {
  if (_debug) {
    Serial2.print("*WM: ");
    Serial2.println(text);
  }
}

int WiFiManager32::getRSSIasQuality(int RSSI) {
  int quality = 0;

  if (RSSI <= -100) {
    quality = 0;
  } else if (RSSI >= -50) {
    quality = 100;
  } else {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}

/** Is this an IP? */
boolean WiFiManager32::isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String WiFiManager32::toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
