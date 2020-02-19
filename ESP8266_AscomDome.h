#ifndef _ESP8266_ASCOMDOME_H_
#define _ESP8266_ASCOMDOME_H_

#define _DEBUG

#define _ESP8266_01_
//_ESP8266_12_

//Use for client testing
#define _DISABLE_MQTT_

//#define _USE_ENCODER_FOR_DOME_ROTATION
//Manage different Encoder pinout variants of the ESP8266
#ifdef _ESP8266_12_ 
#pragma GCC Warning "ESP8266-12 Device selected"
#else // _ESP8266_01_
#define _ESP8266_01_
#pragma GCC Warning "ESP8266-01 Device selected"
#undef _USE_ENCODER_FOR_DOME_ROTATION
#endif

#include <Esp.h>                 //used for restart
#include <ESP8266WiFi.h>         //https://links2004.github.io/Arduino/d3/d58/class_e_s_p8266_web_server.html
#include <WiFiUdp.h>             //check whether required for NTP and DNS successful operation
#include <ESP8266WebServer.h>    //REST web server
//#include "ESP8266WebServerEx.h"  //REST web server case-insensitive arguments handler
#include "ESP8266HTTPUpdateServer.h"  //REST web server handllers for OTA firmware update

//These two are for REST calls out. 
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>     //JSON response formatting
#include <Wire.h>            //I2C dependencies
#include <EEPROM.h>
#include <EEPROMAnything.h>

#include <LinkedList.h>      //https://github.com/ivanseidel/LinkedList
#include <PubSubClient.h> //https://pubsubclient.knolleary.net/api.html
#include "Skybadger_debug_serial.h" 

//Ntp dependencies - available from v2.4
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>

#ifdef ESP8266
extern "C" {
#include "ets_sys.h"         //Base timer and interrupt handling 
#include "osapi.h"           //re-cast ets_sys names  - might be missing hw_timer_t - cast to uint32_t  or _ETSTIMER_ ?
#include "user_interface.h"
}
#endif

/* Some of the code is based on examples in:
   "20A-ESP8266__RTOS_SDK__Programming Guide__EN_v1.3.0.pdf"
*/
#define TZ              0       // (utc+) TZ in hours
#define DST_MN          60      // use 60mn for summer time in some countries
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
time_t now; //use as 'gmtime(&now);'

//Program constants
#define MAX_NAME_LENGTH 25
const int nameLengthLimit = 25; //Default max length of names in char[]
const int acceptableAzimuthError = 2; //Indicates how close to target we want to get before we say its done. 
const int slowAzimuthRange = 10; //Indicates how close to target we want to get before we slow down to a crawl.
enum domeState               { DOME_IDLE, DOME_SLEWING, DOME_ABORT };
enum domeCmd                 { CMD_DOME_ABORT=0, CMD_DOME_SLEW=1, CMD_DOME_PARK=2, CMD_DOME_HOME=3, CMD_DOMEVAR_SET };
enum shutterState            { SHUTTER_OPEN, SHUTTER_CLOSED, SHUTTER_OPENING, SHUTTER_CLOSING, SHUTTER_ERROR }; //ASCOM defined constants.
enum shutterCmd              { CMD_SHUTTER_ABORT=0, CMD_SHUTTER_OPEN=4, CMD_SHUTTER_CLOSE=5, CMD_SHUTTERVAR_SET };
enum motorSpeed: uint8_t     { MOTOR_SPEED_OFF=0, MOTOR_SPEED_SLOW_SLEW=80, MOTOR_SPEED_FAST_SLEW=120 };
enum motorDirection: uint8_t { MOTOR_DIRN_CW=0, MOTOR_DIRN_CCW=1 };
enum I2CConst                { I2C_READ = 0x70, I2C_WRITE = 0x00 };  

//Dome Encoder information
#ifdef _USE_ENCODER_FOR_DOME_ROTATION
#pragma GCC Warning "Using encoder for dome position encoding"
#ifdef _USE_ENCODER_FOR_DOME_ROTATION
#define ENCODER_A_PIN 4
#define ENCODER_B_PIN 5
#define ENCODER_H_PIN 5
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
const int defaultEncoderTicksPerRevolution = 2048;
const unsigned long int defaultEncoderCountsPerDomeRev = defaultEncoderTicksPerRevolution * (48/60/25.40);
int encoderTicksPerRevolution = defaultEncoderTicksPerRevolution;
#endif
#else
#pragma GCC Warning "Using Compass Rest api for dome position encoding"
#endif

//Struct to describe cmd items, used to add cmds to async processing cmd list for completion in request order. 
typedef struct {
  int cmd;
  String cmdName;
  int value;
  long int clientId;
  long int transId;
} cmdItem_t;
  
//Internal variables
#include <SkybadgerStrings.h>

//defaults for setup before replacing with values read from eeprom
const char* defaultHostname        =   "espDOM01";
const char* defaultSensorHostname  =   "espSEN01";
const char* defaultShutterHostname =   "espDSH01";

//nullptr is pre-req for setup default function; 
char* myHostname         = nullptr;
char* sensorHostname     = nullptr;
char* shutterHostname    = nullptr;
char* thisID             = nullptr;

bool abortFlag = false; 
float azimuthSyncOffset = 0.0F; //+ve values means tthe dome is further round N through E than returned from the raw reading. 
float targetAzimuth = 0.0F;
float currentAzimuth = 0.0F;    //+ve values are 0 N thro 90 E
float bearing;
int currentAltitude = 0;
int targetAltitude = 0;
int domeStatus = DOME_IDLE;
int domeLastStatus = domeStatus;
int shutterStatus = SHUTTER_CLOSED;
int motorSpeed = MOTOR_SPEED_OFF; 
int motorDirection = MOTOR_DIRN_CW;
volatile boolean coarseTimerFlag = false;
volatile boolean fineTimerFlag = false;
volatile boolean timeoutTimerFlag = false;

LinkedList <cmdItem_t*> *domeCmdList;
LinkedList <cmdItem_t*> *shutterCmdList;
LinkedList <cmdItem_t*> *cmdStatusList; // use to track async completion state. 

//Functions added to custom setup page
bool parkDomeOnDisconnect = false;
bool closeShutterOnDisconnect = false;

// Create an instance of the server - specify the port to listen on as an argument
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updater;

//REST API client
WiFiClient wClient;
WiFiClient mClient;
HTTPClient hClient;

//MQTT client
PubSubClient client(mClient);
volatile bool callbackFlag = 0;

//JSON data formatter
//DynamicJsonBuffer jsonBuffer(256);

//Encoder interface 
#if defined _USE_ENCODER_FOR_DOME_ROTATION
Encoder encoder(ENCODER_A_PIN, ENCODER_B_PIN );
#endif

//Hardware device system functions - reset/restart etc
EspClass device;
long int nowTime, startTime, indexTime;
ETSTimer fineTimer, coarseTimer, timeoutTimer;
void onCoarseTimer( void* ptr );
void onFineTimer( void* ptr );
void onTimeoutTimer( void* ptr );

//Private web handler methods
;

//Internal functions
void setupMotor();
#ifdef _USE_ENCODER_FOR_DOME_ROTATION
void setupEncoder();
#else
bool setupCompass(String url);
#endif
void saveToEeprom();   //Started - incomplete
void readFromEeprom(); //Started - incomplete
void fineTimerHandler(void);
void coarseTimerHandler(void);
void timeoutTimerHandler(void);

//ASCOM-dependent variables 
unsigned int transactionId;
unsigned int clientId;
bool connected = false;
const String DriverName = "Skybadger.ESPDome";
const String DriverVersion = "0.0.1";
const String DriverInfo = "Skybadger.ESPDome RESTful native device. ";
const String Description = "Skybadger ESP2866-based wireless ASCOM Dome controller";
const String InterfaceVersion = "2";
//setup later since we are allowing this to be dynamic. 
//pre-req for setup default function; 
char* Name = nullptr; 

//ASCOM variables
const int defaultHomePosition = 180;
const int defaultParkPosition = 180;
bool atPark = false;
bool atHome = false;
int homePosition = defaultHomePosition;
int parkPosition = defaultParkPosition;
int altitude = 0;
int azimuth = 0; 
int azimuthOffset = 0;
const bool canFindHome = true; 
const bool canPark = true;
const bool canSetAzimuth = true;
const bool canSetAltitude = false;
const bool canSetPark = true;
const bool canSetShutter = false;
bool canSlave = false;
const bool canSyncAzimuth = true;
const bool slaved = false;
const bool slewing = false;

//Attached I2C device address list
//const uint8_t compassAddr = 0x0d; - no longer an attached device - remove in the fullness.
const uint8_t motorControllerAddr = 176;
const uint8_t LCDControllerAddr = 0xC6;
const uint8_t ADCControllerAddr = 0x48;

#include "I2CLCD.h"
I2CLCD myLCD( LCDControllerAddr, 4, 16 );
bool LCDPresent = false;
#include "i2cMotor.h"
i2cMotor myMotor( motorControllerAddr );
bool motorPresent = false;

#endif //_ESP8266_ASCOMDOME_H_
