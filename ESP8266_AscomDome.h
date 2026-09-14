#ifndef _ESP8266_ASCOMDOME_H_
#define _ESP8266_ASCOMDOME_H_

//Set to 0 (or override with build_flags) to use the original setup pages.
#ifndef DOME_MODERN_SETUP
#define DOME_MODERN_SETUP 0
#endif

//State what the target hardware is - determines use of pins for I2C for instance.
//#define _ESP8266_01_
#define _ESP8266_12_
//#define _ESP32_XX_
//Manage different Encoder pinout variants of the ESP8266
#if defined _ESP8266_12_ //Used to manage pinout.
#pragma GCC Warning "ESP8266-12 Device selected"
#elif defined _ESP8266_01_
#pragma GCC Warning "ESP8266-01 Device selected"
#undef USE_LOCAL_ENCODER_FOR_DOME_ROTATION
#elif defined _ESP32_XX_
#pragma GCC Warning "ESP32 Device selected"
#else
#pragma GCC Error "No Device pinout selected"
#endif

//Use for client detailed performance & functional testing
//#define DEBUG_ESP_HTTP_CLIENT
//#define DEBUG_ESP_HTTP_SERVER
#define _DEBUG
#define _DEBUG_ESP              //Enables basic debugging statements for ESP
#define HTTP_CLIENT_REUSE false //Re-use the existing connection or not for subsequent comms within a session
//Use for client testing
//#define _DISABLE_MQTT                     //Disable the MQTT handling segments.
//#define _DEBUG_MQTT                       //enable low-level MQTT connection debugging statements.
#include "DebugSerial.h"

//Flag to specify whether code checks for connected client ID on motion command requests or not
//added to support reboots of dome controller due to un-diagnosed power brownouts which Voyager doesn't get to see and therefore loses control of the dome.
//Doing this means the dome will come up and still accept calls from anyone... as long as it reads the encoder and the encoder stays running, we maintain our position knowledge
//If using a local direct attached device, have to assume positional knowledge will be lost too.
//#define ACCEPT_CONNECTED_CLIENT_ONLY

//remote debugging
//Manage the remote debug interface, it takes 6K of memory with all the strings even when not in use but loaded
//#define _DISABLE_REMOTE_DEBUG             //disables all debugX calls.
//#define DEBUG_DISABLED                    //put all debug to serial
#define DEBUG_DISABLE_AUTO_FUNC true //Turn on or off the auto function labelling feature .
#define WEBSOCKET_DISABLED true      //No impact to memory requirement
//#define MAX_TIME_INACTIVE 0             //to turn off the de-activation of a telnet session
#include <RemoteDebug.h> //https://github.com/JoaoLopesF/RemoteDebug

//Used to test for memory leaks.
//#define _TEST_RAM_ //turn on RAM check settings
//#define _MEMLEAK_CHECK
//#define _MEMLEAK_CHECK_DEBUG

//#define _ENABLE_BEARING                      //Turn off loop segment for bearing update if not set.
//#define _ENABLE_SHUTTER                    // Turn off loop segment for shutter handling if not set.
//#define _ENABLE_DOME                       //Turn off segment for dome handling if not set.

//Select a method of getting positional feedback on dome rotation.
//#define USE_REMOTE_COMPASS_FOR_DOME_ROTATION
//#define USE_LOCAL_ENCODER_FOR_DOME_ROTATION
#define USE_REMOTE_ENCODER_FOR_DOME_ROTATION

#include "SkybadgerStrings.h"

#include <Esp.h> //used for restart and cycle timer

#if defined _ESP32_XX_
#include <Wifi.h>
#include <WiFiClient.h>
#include <WebServer.h>

#else                                //ESP8266
#include <ESP8266WiFi.h>             //https://links2004.github.io/Arduino/d3/d58/class_e_s_p8266_web_server.html
#include <ESP8266WebServer.h>        //REST web server
#include "ESP8266HTTPUpdateServer.h" //REST web server handlers for OTA firmware update
#include <WiFiUdp.h>                 //Used for Alpaca Management
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#endif

#include <ArduinoJson.h>       //JSON response formatting
#include "ASCOMAPIDome_rest.h" //Shared Dome states and REST handler declarations
#include <Wire.h>              //I2C dependencies
#include <EEPROM.h>
#include <EEPROMAnything.h>

#include <PubSubClient.h> //https://pubsubclient.knolleary.net/api.html

//#include <GDBStub.h> //Debugging stub for GDB

int bootCount = 0;

//Create a remote debug object
#ifndef _DISABLE_REMOTE_DEBUG
RemoteDebug Debug;
#endif

//Ntp dependencies - available from v2.4
#include <time.h>
#include <sys/time.h>
//#include <coredecls.h>
#ifdef ESP8266
extern "C"
{
#include "ets_sys.h" //Base timer and interrupt handling
#include "osapi.h"   //re-cast ets_sys names  - might be missing hw_timer_t - cast to uint32_t  or _ETSTIMER_ ?
#include "user_interface.h"
}
#endif

/* Some of the code is based on examples in:
   "20A-ESP8266__RTOS_SDK__Programming Guide__EN_v1.3.0.pdf"
*/
#define TZ 0      // (utc+) TZ in hours
#define DST_MN 00 // use 60mn for summer time in some countries
#define TZ_MN ((TZ) * 60)
#define TZ_SEC ((TZ) * 3600)
#define DST_SEC ((DST_MN) * 60)
time_t now; //use as 'gmtime(&now);'

//Program constants
#define BUILDSTRING __FILE__ + __DATE__
#if !defined _DISABLE_REMOTE_DEBUG
const char *BuildVersionName PROGMEM = " LWIPv2 Higher Bandwidth, RDebug enabled \n";
#else
const char *BuildVersionName PROGMEM = " LWIPv2 Higher Bandwidth, RDebug disabled \n";
#endif

#define MAX_NAME_LENGTH 40
const int nameLengthLimit = MAX_NAME_LENGTH; //Default max length of names in char[]
const int acceptableAzimuthError = 1;        //Indicates how close to target we want to get before we say its done.
const int slowAzimuthRange = 10;             //Indicates how close to target we want to get before we slow down to a crawl.
const char *domeStateNames[] = {"DOME_IDLE", "DOME_SLEWING", "DOME_ABORT", "DOME_ABORTED", "DOME_HALTED"};
enum domeCmd
{
  CMD_DOME_ABORT = 0,
  CMD_DOME_SLEW = 1,
  CMD_DOME_PARK = 2,
  CMD_DOME_HOME = 3,
  CMD_DOMEVAR_SET
};
const char *shutterStateNames[] = {"SHUTTER_OPEN", "SHUTTER_CLOSED", "SHUTTER_OPENING", "SHUTTER_CLOSING", "SHUTTER_ERROR", "SHUTTER_ABORTING", "SHUTTER_ABORTED", "SHUTTER_HALTED"};
const char *connectionStateNames[] = {"CONNECTION_DISCONNECTED", "CONNECTION_CONNECTING", "CONNECTION_CONNECTED", "CONNECTION_DISCONNECTING"};
enum shutterCmd
{
  CMD_SHUTTER_ABORT = 0,
  CMD_SHUTTER_OPEN = 4,
  CMD_SHUTTER_CLOSE = 5,
  CMD_SHUTTERVAR_SET
};
const char *shutterCmdNames[] = {"SHUTTER_ABORT", "SHUTTER_OPEN", "SHUTTER_CLOSE", "SHUTTERVAR_SET"};
//enum motorSpeed: uint8_t     { MOTOR_SPEED_OFF=0, MOTOR_SPEED_SLOW_SLEW=120, MOTOR_SPEED_FAST_SLEW=180 };
//enum motorDirection: uint8_t { MOTOR_DIRN_CW=0, MOTOR_DIRN_CCW=1 };
enum I2CConst
{
  I2C_READ = 0x80,
  I2C_WRITE = 0x00
};
#define SHUTTER_MAX_ALTITUDE 110
#define SHUTTER_MIN_ALTITUDE 0

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
const float defaultWheelDiameter = 61.3;              //Encoder jockey wheel
const int defaultDomeDiameter = 2700;                 //My Dome diameter - change for yours.
const int defaultEncoderTicksPerRevolution = 400 * 4; //Encoder PPR is 400 but we detect every edge so *4
const int defaultEncoderCountsPerDomeRev = defaultEncoderTicksPerRevolution * (defaultDomeDiameter / defaultWheelDiameter);
int encoderTicksPerRevolution = defaultEncoderTicksPerRevolution;
int position = 0;
int lastPosition = 0;
#endif //Local encoder
#elif defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION
#pragma GCC Warning "Using Compass Rest api for dome position encoding"
#endif

//Struct to describe cmd items, used to add cmds to async processing cmd list for completion in request order.
typedef struct
{
  int cmd;
  char *cmdName;
  int value;
  long int clientId;
  long int transId;
} cmdItem_t;

//defaults for setup before replacing with values read from eeprom
//Should be const but compiler barfs when copying into an array for later use
static const char *defaultHostname PROGMEM = "espdom01";
static const char *defaultShutterHostname PROGMEM = "espdsh00.i-badger.co.uk";
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION
static const char *defaultSensorHostname PROGMEM = "espsen01.i-badger.co.uk"; //Remote Compass host
#elif defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
static const char *defaultSensorHostname PROGMEM = "espenc01.i-badger.co.uk"; //Remote Encoder host
#elif defined USE_LOCAL_COMPASS_FOR_DOME_ROTATION
//Nada
#endif //Encoder source host selection

//nullptr is pre-req for setup default function;
char *myHostname = nullptr;
char *shutterHostname = nullptr;
char *thisID = nullptr;
char *MQTTServerName = nullptr;
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
char *sensorHostname = nullptr;
#endif

bool abortFlag = false;
float azimuthSyncOffset = 0.0F; //+ve values means the dome is further round N through E than returned from the raw reading.
float targetAzimuth = 0.0F;
float currentAzimuth = 0.0F; //+ve values are 0 N thro 90 E to 360
float bearing = 0.0F;
float currentAltitude = 0.0F;
float targetAltitude = 0.0F;

//State tracking
enum domeState domeStatus = DOME_IDLE;
enum domeState domeTargetStatus = domeStatus;
enum shutterState shutterStatus = SHUTTER_CLOSED;
enum shutterState targetShutterStatus = SHUTTER_CLOSED;
enum connectionState connectionStatus = CONNECTION_DISCONNECTED;
uint32_t pendingConnectionClientID = 0;
uint32_t connectionStateChangedAt = 0;
extern void publishFnStatus(void);

//Timer flags
volatile boolean coarseTimerFlag = false;
volatile boolean fineTimerFlag = false;
volatile boolean timeoutTimerFlag = false;
volatile boolean watchdogTimerFlag = false;
static int blockedTarget;

//Cmd lists used to queue commands https://github.com/ivanseidel/LinkedList
#include <LinkedList.h>
LinkedList<cmdItem_t *> *domeCmdList;
LinkedList<cmdItem_t *> *shutterCmdList;
LinkedList<cmdItem_t *> *cmdStatusList; // use to track async completion state.

//Functions added to custom setup page
bool parkDomeOnDisconnect = true;
bool closeShutterOnDisconnect = true;

// Create an instance of the server - specify the port to listen on as an argument
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updater;

//REST API client
WiFiClient mClient; //Client for MQTT

//MQTT client - use separate WiFi client as an attempt to separate issues.
PubSubClient client(mClient);
volatile bool callbackFlag = false;
bool timeoutFlag = false;
volatile bool timerSet = false;

//Encoder interface
#if defined USE_LOCAL_ENCODER_FOR_DOME_ROTATION
Encoder myEnc(ENCODER_A_PIN, ENCODER_B_PIN);
#endif

//Hardware device system functions - reset/restart etc
EspClass device;
uint32_t originalRam;
uint32_t lastRam;
long int nowTime, startTime, indexTime;
unsigned long int debugId = 0; //Seed a debug log ID

ETSTimer fineTimer;
ETSTimer coarseTimer;
ETSTimer timeoutTimer;
ETSTimer minuteTimer;
ETSTimer watchdogTimer; //watchdog for dome stuck.

//Use different status polling periods based on whether the dome is meant to be active or not.
const uint32_t ACTIVE_STATUS_PERIOD_MS = 500;
const uint32_t IDLE_STATUS_PERIOD_MS = 2500;
uint32_t statusPollingPeriodMs = ACTIVE_STATUS_PERIOD_MS;
const uint32_t restQueryTimeout = 5000;

void onCoarseTimer(void *ptr);
void onFineTimer(void *ptr);
void onTimeoutTimer(void *ptr);
void onWatchdogTimer(void *ptr);
void manageConnectionState(void);
void updateStatusPollingPeriod(void);
void appendDomeStatusFields(JsonObject root);

//Private web handler methods
;

//Internal functions
#ifdef USE_LOCAL_ENCODER_FOR_DOME_ROTATION
void setupEncoder();
#else // USE_REMOTE_ENCODER_FOR_DOME_ROTATION or REMOTE_COMPASS
bool setupCompass(String url);
#endif

bool saveToEeprom();
void readFromEeprom();
void fineTimerHandler(void);     //sensor loop
void coarseTimerHandler(void);   //command loop
void timeoutTimerHandler(void);  //MQTT reconnect try loop
void watchdogTimerHandler(void); //rotation lockup timeout handler - currently not used.

//ASCOM-dependent variables
unsigned int transactionId;
unsigned int clientId;
int connectionCtr = 0; //variable to count number of times something has connected compared to disconnected.
extern const unsigned int NOT_CONNECTED;
unsigned int connected = NOT_CONNECTED;
static const char *DriverName PROGMEM = "Skybadger.ESPDome";
static const char *DriverVersion PROGMEM = "1.2";
static const char *DriverInfo PROGMEM = "Skybadger.ESPDome RESTful native device. ";
static const char *Description PROGMEM = "Skybadger ESP2866-based wireless ASCOM Dome controller";
static const int32 InterfaceVersion PROGMEM = 1;
static const char *DriverType PROGMEM = "dome"; //Must be a valid ASCOM type to be recognised by UDP discovery - lower case required

//ALPACA support additions
//UDP Port can be edited in setup page
#define ALPACA_DISCOVERY_PORT 32227
int udpPort = ALPACA_DISCOVERY_PORT;
WiFiUDP Udp;
//espdom00 GUID - "0011-0000-0000-0000"; prototype
//espdom01 GUID - "0011-0000-0000-0001"; working
static const char *GUID PROGMEM = "0011-0000-0000-0001";

//Use when there are multple instances for this device - not very likely for a dome.
const int defaultInstanceNumber = 1;
int instanceNumber = defaultInstanceNumber;

//setup later since we are allowing this to be dynamic via EEprom.
//pre-req for setup default function;
static const char *defaultAscomName PROGMEM = "Skybadger Dome 01";
char *ascomName = nullptr;

//Mgmt Api Constants
//The iteration version identifier for this driver. Update every major change - relate to your repo versioning
//Alpaca Mgmt Api defines this as a String !. Currently using String( itoa( int) ) in responses.
const int instanceVersion = 3;
char deviceLocation[MAX_NAME_LENGTH] = "";
char *Location = deviceLocation;

//ASCOM variables
const int defaultHomePosition = 180; //change as you need
const int defaultParkPosition = 356; //change as you need.
int homePosition = defaultHomePosition;
int parkPosition = defaultParkPosition;
//bool atPark = false; Dynamically detected from current position.
//bool atHome = false;
int altitude = 0;
int azimuth = 0;
const bool canFindHome = true;
const bool canPark = true;
const bool canSetAzimuth = true;
const bool canSetAltitude = false;
const bool canSetPark = true;
const bool canSetShutter = true;
bool canSlave = false;
const bool canSyncAzimuth = true;
bool slaved = false;
bool slewing = false; //Not used - uses domeStatus instead.

//Attached I2C device address list
//const uint8_t compassAddr = 0x0d; - no longer an attached device - remove in the fullness.
const uint8_t PCFAControllerAddr = 0x10; //160 Waveshare expander board
//const uint8_t PCFAControllerAddr  = 0x70;//TI8574a
const uint8_t PCFControllerAddr = 0x20;  //32
const uint8_t motorControllerAddr = 88;  //7-bit == 0xB0;//176
const uint8_t LCDControllerAddr = 99;    //0x61 (C2)0x63 (C6) 7-bit == 0xC6;//198
const uint8_t ADCControllerAddr = 0x48;  //72
const uint8_t FRAMControllerAddr = 0x50; //7-bit FRAM address with address pins low

#include <FRAM_FM24CXX_I2C.h>
FRAM_FM24CXX_I2C myFRAM(FRAMControllerAddr, DEFAULT_WP_STATUS, DEFAULT_WP_PIN, 64);
bool framPresent = false;

#include "I2CLCD.h"
I2CLCD myLCD(LCDControllerAddr, 4, 16);
bool lcdPresent = false;

#include "i2cMotor.h"
i2cMotor myMotor(motorControllerAddr);
bool motorPresent = false;
uint8_t motorSpeed = MOTOR_SPEED_OFF;
uint8_t motorDirection = MOTOR_DIRN_CW;

#endif //_ESP8266_ASCOMDOME_H_
