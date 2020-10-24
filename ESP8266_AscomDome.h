#ifndef _ESP8266_ASCOMDOME_H_
#define _ESP8266_ASCOMDOME_H_

#define _DEBUG
// #define DEBUG_HTTPCLIENT(...) Serial.printf( __VA_ARGS__ ) // 

#define _ESP8266_01_
//define _ESP8266_12_

//Use for client testing
//#define _DISABLE_MQTT_

//#define USE_REMOTE_COMPASS_FOR_DOME_ROTATION
//#define USE_LOCAL_ENCODER_FOR_DOME_ROTATION
#define USE_REMOTE_ENCODER_FOR_DOME_ROTATION

//Manage different Encoder pinout variants of the ESP8266
#ifdef _ESP8266_12_ 
#pragma GCC Warning "ESP8266-12 Device selected"
#else // _ESP8266_01_
#define _ESP8266_01_
#pragma GCC Warning "ESP8266-01 Device selected"
#undef USE_LOCAL_ENCODER_FOR_DOME_ROTATION
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

#include <PubSubClient.h> //https://pubsubclient.knolleary.net/api.html
#include "DebugSerial.h" 
#include <GDBStub.h> //Debugging stub for GDB
#include "RemoteDebug.h"  //https://github.com/JoaoLopesF/RemoteDebug
//Create a remote debug object
RemoteDebug Debug;

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
#define DST_MN          00      // use 60mn for summer time in some countries
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
time_t now; //use as 'gmtime(&now);'

//Program constants
#define MAX_NAME_LENGTH 100
const int nameLengthLimit = MAX_NAME_LENGTH; //Default max length of names in char[]
const int acceptableAzimuthError = 2; //Indicates how close to target we want to get before we say its done. 
const int slowAzimuthRange = 10; //Indicates how close to target we want to get before we slow down to a crawl.
enum domeState               { DOME_IDLE, DOME_SLEWING, DOME_ABORT };
const char* domeStateNames[] = {"DOME_IDLE","DOME_SLEWING","DOME_ABORT"};
enum domeCmd                 { CMD_DOME_ABORT=0, CMD_DOME_SLEW=1, CMD_DOME_PARK=2, CMD_DOME_HOME=3, CMD_DOMEVAR_SET };
enum shutterState            { SHUTTER_OPEN, SHUTTER_CLOSED, SHUTTER_OPENING, SHUTTER_CLOSING, SHUTTER_ERROR }; //ASCOM defined constants.
const char* shutterStateNames[] = {"SHUTTER_OPEN","SHUTTER_CLOSED","SHUTTER_OPENING", "SHUTTER_CLOSING", "SHUTTER_ERROR" };
enum shutterCmd              { CMD_SHUTTER_ABORT=0, CMD_SHUTTER_OPEN=4, CMD_SHUTTER_CLOSE=5, CMD_SHUTTERVAR_SET };
//enum motorSpeed: uint8_t     { MOTOR_SPEED_OFF=0, MOTOR_SPEED_SLOW_SLEW=120, MOTOR_SPEED_FAST_SLEW=240 };
//enum motorDirection: uint8_t { MOTOR_DIRN_CW=0, MOTOR_DIRN_CCW=1 };
enum I2CConst                { I2C_READ = 0x80, I2C_WRITE = 0x00 };  
#define SHUTTER_MAX_ALTITUDE 110.0F

//Dome Encoder information
#ifdef USE_REMOTE_ENCODER_FOR_DOME_ROTATION
#pragma GCC Warning "Using remote encoder for dome position encoding"
#ifdef USE_LOCAL_ENCODER_FOR_DOME_ROTATION
#ifdef _ESP8266_01_
#pragma GCC Error "Using local dome with ESP-01 incompatible with hardware pins available."
#endif

#define ENCODER_A_PIN 4
#define ENCODER_B_PIN 5
#define ENCODER_H_PIN 6
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
const int defaultWheelDiameter = 68; //Encoder jockey wheel 
const int defaultDomeDiameter = 2700; //My Dome diameter - change for yours. 
const int defaultEncoderTicksPerRevolution = 400*4; //Encoder PPR is 400 but we detect every edge so *4 
const int defaultEncoderCountsPerDomeRev = defaultEncoderTicksPerRevolution * (defaultDomeDiameter/defaultWheelDiameter);
int encoderTicksPerRevolution = defaultEncoderTicksPerRevolution;
int position = 0;
int lastPosition = 0;
#endif //Local encoder
#elif defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION
#pragma GCC Warning "Using Compass Rest api for dome position encoding"
#endif

//Struct to describe cmd items, used to add cmds to async processing cmd list for completion in request order. 
typedef struct {
  int cmd;
  String cmdName; //Is this legit or does it need managing more ? From a calloc/free pointof view? 
  int value;
  long int clientId;
  long int transId;
} cmdItem_t;
  
//Internal variables
#include <SkybadgerStrings.h>

//defaults for setup before replacing with values read from eeprom
//Should be const but compiler barfs when copying into an array for later use
const char* defaultHostname        =   "espDOM01";
const char* defaultShutterHostname =   "espDSH00.i-badger.co.uk";
#if   defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION
const char* defaultSensorHostname  =   "espSEN01.i-badger.co.uk"; //Remote Compass host
#elif defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
const char* defaultSensorHostname  =   "espENC01.i-badger.co.uk"; //Remote Encoder host
#elif defined USE_LOCAL_COMPASS_FOR_DOME_ROTATION
//Nada
#endif //Encoder source host selection 
 
//nullptr is pre-req for setup default function; 
char* myHostname         = nullptr;
char* shutterHostname    = nullptr;
char* thisID             = nullptr;
char* MQTTServerName     = nullptr;
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
char* sensorHostname     = nullptr;
#endif

bool abortFlag = false; 
float azimuthSyncOffset = 0.0F; //+ve values means the dome is further round N through E than returned from the raw reading. 
float targetAzimuth = 0.0F;
float currentAzimuth = 0.0F;    //+ve values are 0 N thro 90 E
float bearing = 0.0F;
float currentAltitude = 0.0F;
float targetAltitude = 0.0F;
int domeStatus = DOME_IDLE;
int domeTargetStatus  = domeStatus;
int domeLastStatus = domeStatus;
int shutterStatus = SHUTTER_CLOSED;
int shutterTargetStatus = shutterStatus;
volatile boolean coarseTimerFlag = false;
volatile boolean fineTimerFlag = false;
volatile boolean timeoutTimerFlag = false;

#include <LinkedList.h>      //https://github.com/ivanseidel/LinkedList
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
WiFiClient wClient;  //Client for web requests
WiFiClient mClient;  //Client for MQTT

//MQTT client - use separate WiFi client as an attempt to separate issues. 
PubSubClient client(mClient);
volatile bool callbackFlag = false;
bool timeoutFlag = false;
bool timerSet = false;

//Encoder interface 
#if defined USE_LOCAL_ENCODER_FOR_DOME_ROTATION
Encoder myEnc(ENCODER_A_PIN, ENCODER_B_PIN );
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
#ifdef USE_LOCAL_ENCODER_FOR_DOME_ROTATION
void setupEncoder();
#else// USE_REMOTE_ENCODER_FOR_DOME_ROTATION or REMOTE_COMPASS
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
const long int NOT_CONNECTED = -1;
int connectionCtr = 0;
long int connected = NOT_CONNECTED;
const String DriverName = "Skybadger.ESPDome";
const String DriverVersion = "0.9";
const String DriverInfo = "Skybadger.ESPDome RESTful native device. ";
const String Description = "Skybadger ESP2866-based wireless ASCOM Dome controller";
const String InterfaceVersion = "2";
//ALPACA support additions
const int INSTANCE_NUMBER = 0001;
const int GUID = 001002003004;

//setup later since we are allowing this to be dynamic. 
//pre-req for setup default function; 
const char* defaultAscomName = "Skybadger Dome 01"; 
char* ascomName = nullptr;

//ASCOM variables
const int defaultHomePosition = 180;
const int defaultParkPosition = 180;
bool atPark = false;
bool atHome = false;
int homePosition = defaultHomePosition;
int parkPosition = defaultParkPosition;
int altitude = 0;
int azimuth = 0; 
const bool canFindHome = true; 
const bool canPark = true;
const bool canSetAzimuth = true;
const bool canSetAltitude = false;
const bool canSetPark = true;
const bool canSetShutter = false;
bool canSlave = false;
const bool canSyncAzimuth = true;
bool slaved = false;
bool slewing = false;

//Attached I2C device address list
//const uint8_t compassAddr = 0x0d; - no longer an attached device - remove in the fullness.
const uint8_t PCFAControllerAddr  = 0x10;//160 Waveshare expander board
//const uint8_t PCFAControllerAddr  = 0x70;//TI8574a 
const uint8_t PCFControllerAddr   = 0x20;//32  
const uint8_t motorControllerAddr = 88;//7-bit == 0xB0;//176
const uint8_t LCDControllerAddr   = 99;//0x61 (C2)0x63 (C6) 7-bit == 0xC6;//198
const uint8_t ADCControllerAddr   = 0x48;//72

#include "I2CLCD.h"
I2CLCD myLCD( LCDControllerAddr, 4, 16 );
bool lcdPresent = false;

#include "i2cMotor.h"
i2cMotor myMotor( motorControllerAddr );
bool motorPresent = false;
uint8_t motorSpeed = MOTOR_SPEED_OFF; 
uint8_t motorDirection = MOTOR_DIRN_CW;

#endif //_ESP8266_ASCOMDOME_H_
