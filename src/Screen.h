#ifndef MY_SCREEN_H
#define MY_SCREEN_H
#include <vector>
#include "Logger.h"
#include <Arduino.h>
#include <ArduinoJson.h>


//Screens that we have
enum screens {
  NON_SELECTED = -1 ,
  AP_SCR,
  NO_BEER_SCR,
  CONNECTING_SCR,
  DISCONNECTED_SCR,
  CONFIG_SCR,
  CALIBRATE_SCR,
  SAVE_SCR,
  OTA_UPDATE_SCR,
  RETRY_AP_SCR,
  LOCK_SCR,
  LOADING_SCR,
  SELECT_QTY,
  PINT_QTY,
  TASTER_QTY,
  GROWLER_QTY,
  OPEN_QTY,
  SERVING_SCR,
  ENTER_CALIBRATION_SCR,
};

#define AFFECT_INDEX      9

//Buttons that we have
enum buttons {
  NONE = -1,
  Vaso,           
  Taster,         
  Growler,        
  Mermar,         
  Cancelar,       
  Listo,          
  Oz_2,           
  Oz_5,           
  Oz_32,          
  Oz_64,          
  Oz_128,          
  cog,             
  Calibrar,        
  Back,            
  Guardar,         
  Descartar,       
  Oz_8,            
  Oz_10,           
  Oz_16,           
  Actualizar,      
  Retry,           
  CONFIGURAR_WIFI, 
  BOOT_WITH_FILE,  
  ENTER_CALIBRATION_FACTOR
};

//ERORS
#define WIFI_NOT_CONNECTED_MSG    "Sin conexion a WiFi"
#define NO_SERVER_CONNECTION_MSG  "Error en Servidor" 

enum ERRORS {
  NO_ERROR = -1,
  WIFI_NOT_CONNECTED = 1,
  NO_SERVER_CONNECTION = 2,    
};

struct newLine {  // this should be move to line class
  String keg_id;
  String beer_abv;        // this should be rename as "abv"
  String beer_name;       // this should be rename as "name"
  String beer_style;      // this should be rename as "style"
  uint16_t no_linea;      // this should be rename as "line_number"
};


class Screen {
  public:
    // TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

    // TFT_eFEX  fex = TFT_eFEX(&tft);
    float ppm;
    String kegId;
    String beerAbv;
    String beerName;
    String beerStyle;
    uint16_t noLinea;
    int actualScreen;
    bool isServing = false;
    bool isMermando = false;

    // TFT_eSPI_Button key[19];
    String emergencyCard = "";
    bool isCalibrating = false;
    char keyLabel[4][8] = {"Vaso", "Taster", "Growler", "Mermar"};

    // void splash();
    void noWifi();
    void tapCard();
    void Settings();
    void drawError();
    void retryOrAP();
    void showReady();
    void LockScreen();
    void connecting();
    void removeInfo();
    int8_t getError();
    uint8_t isPressed();
    void LoadingModal();
    bool isOnEmergency();
    bool isTouchEneable();
    void setError(int8_t);
    void NoBeerAssigned();
    void otaUpdateScreen();
    void hideLoadingModal();
    void mermar(uint16_t ml);
    void ServeOptionsGlass();
    String getErrorMessage();
    void ServeOptionsTaster();
    void notConnectedToSever();
    void ServeOptionsGrowler();
    void setEmergency(bool state);
    void enterCalibrationFactor(const std::function<void(float)>& cb);
    void setInfo(const char * info);
    void setTouchEneable(bool value);
    void drawMl(double ml, bool isMl);
    void calibrationScreen( uint16_t ml);
    void saveCalibration(uint16_t pulses);
// nada
// nada
    void AP(String name, bool retriable = false);
    void SelectQty(String user, String client = "" );
    void servingScreen( bool drawIt, uint8_t update, String type);
    void enterInputMode(const std::function<void(float)>& cb) {
      _inInputMode   = true;
      _inputCallback = cb;
    }
    void exitInputMode() {
      _inInputMode   = false;
      _inputCallback = nullptr;
    }
    bool isOnInputMode() const { return _inInputMode; }

  private:
    bool loading_modal = false;
    bool emergency_mode = false;
    bool touch_eneable = false;
    bool _inInputMode = false;
    int8_t error = NO_ERROR;

    std::vector<int> entryOptions;
    std::function<void(float)> _inputCallback;

    //Logger
    void DEBUG(const char *message);
    // void ERROR(ErrorType error);
};
#endif
