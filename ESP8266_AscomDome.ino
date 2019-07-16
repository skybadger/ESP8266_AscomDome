/*
   This program uses an esp8266 board to host a web server providing a rest API to control an ascom Dome
   The dome hardware can come in up to two parts 
   In the first configuration, the dome controller consists of a motor, display, battery sensor and 
   position sensors mounted all together on the dome wall and using i2c to talk to the 'local' devices.
   The second configuration is where the motor and display controllers are attached to the dome wall and the rotation sensor
   is mounted on the rotating dome itself. This sensor is typically a digital compass and accessed via its REST API. (see ESP8266_Mag.ino).
   In either case the part fixed to the wall is the sole interface to ASCOM so the ASCOM driver only has one interface to deal with. 
   THis hardware supports the ALPACA rest API interface so there is no need for an installable driver on the observatory controller server. 
   Configure the rest interface to use this device's hostname (ESPDome01) and port (80)
   
   Implementing.
   Testing.
   Operating. 
   To do
   1, Add MQTT for Bz, temp and dome orientation
   2, dome setup to support calc of auto-track pointing
   3, Add list handler - done
   4, Update smd list to handle variable settings and fixup responses for async responses. Report to ascom community
   Test handler functions 

To test:
 Download curl for your operating system. Linux and winndows powershell should already have it.
 e.g. curl -X PUT -d 'homePosition=270' http://EspDom01/Dome/0/API/v1/FilterCount
 Use the ALPACA REST API docs at <> to validate the dome behaviour. At some point there will be a script

Dependencies
Arduino JSON library 5.13 ( moving to 6 is a big change) 
Expressif ESP8266 board library for arduino - configured for v2.5

ESP8266-12 Huzzah
  /INT - GPIO3
  [I2C SDA - GPIO4
  SCL - GPIO5]
  [SPI SCK = GPIO #14 (default)
  SPI MOSI = GPIO #13 (default)
  SPI MISO = GPIO #12 (default)]
  ENC_A
  ENC_B
  ENC_H
  Humidity sensor - 1-wire inteface
  Battery voltage Sensor- i2c
  Temp Sensor = i2c
  LCD Display - i2c
*/

#define _DEBUG
#ifdef _DEBUG
#define DEBUGS1(a)    Serial.print(a)
#define DEBUGS2(a,b)  Serial.print(a, b)
#define DEBUGSL1(a)   Serial.println(a)
#define DEBUGSL2(a,b) Serial.println(a, b)
#else
#define DEBUGS1 
#define DEBUGS2 
#define DEBUGSL1
#define DEBUGSL2 
#endif

#define __ESP8266_12
#define _USE_ENCODER_FOR_DOME_ROTATION

//Manage different Encoder pinout variants of the ESP8266
#ifdef __ESP8266_12 
#pragma GCC Warning "ESP8266-12 Device selected"
#else // __ESP8266_01
#pragma GCC Warning "ESP8266-01 Device selected"
#undef _USE_ENCODER_FOR_DOME_ROTATION
#endif

#ifdef _USE_ENCODER_FOR_DOME_ROTATION
#pragma GCC Warning "Using encoder for dome position encoding"
#else
#pragma GCC Warning "Using Rest api for dome position encoding"
#endif

#include <Esp.h> //used for restart
#include <ESP8266WiFi.h>
//https://links2004.github.io/Arduino/d3/d58/class_e_s_p8266_web_server.html
#include <ESP8266WebServer.h> //REST server handler
#include <ArduinoJson.h> //JSON response formatting
#include <Wire.h> //I2C dependencies
#include <EEPROM.h>
#include <LinkedList.h> //https://github.com/ivanseidel/LinkedList
//Ntp dependencies - available from v2.4
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>

#ifdef ESP8266
extern "C" {
#include "ets_sys.h" //Base timer and interrupt handling 
#include "osapi.h"   //re-cast ets_sys names  - might be missing hw_timer_t - cast to uint32_t  or _ETSTIMER_ ?
#include "user_interface.h"
}
#endif

/* Some of the code is based on examples in:
   "20A-ESP8266__RTOS_SDK__Programming Guide__EN_v1.3.0.pdf"
*/
#define TZ              1       // (utc+) TZ in hours
#define DST_MN          60      // use 60mn for summer time in some countries
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
time_t now; //use as 'gmtime(&now);'
//long int nowTime = system_get_time();
//long int indexTime = startTime - nowTime;

#ifdef _USE_ENCODER_FOR_DOME_ROTATION
#define ENCODER_A_PIN 4
#define ENCODER_B_PIN 5
#define ENCODER_H_PIN 5
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#endif

//Program constants
const int nameLengthLimit = 25;
enum domeState { DOME_IDLE, DOME_SLEWING, DOME_ABORT };
enum domeCmd { CMD_DOME_ABORT=0, CMD_DOME_SLEW=1, CMD_DOME_PARK=2, CMD_DOME_HOME=3, CMD_DOMEVAR_SET };
enum shutterState { SHUTTER_OPEN, SHUTTER_CLOSED, SHUTTER_OPENING, SHUTTER_CLOSING, SHUTTER_ERROR }; //ASCOM defined constants.
enum shutterCmd  { CMD_SHUTTER_ABORT=0, CMD_SHUTTER_OPEN=4, CMD_SHUTTER_CLOSE=5, CMD_SHUTTERVAR_SET };
enum motorSpeed: uint8_t     { MOTOR_SPEED_OFF=0, MOTOR_SPEED_SLOW_SLEW=80, MOTOR_SPEED_FAST_SLEW=120 };
enum motorDirection: uint8_t { MOTOR_DIRN_CW=0, MOTOR_DIRN_CCW=1 };
enum I2CConst    { I2C_READ = 0x70, I2C_WRITE = 0x00 };  
typedef struct { //Using bit-packing - I hope
  short int shutterLock:1;
  short int shutterClosedSensor:1;
  short int shutterOpenSensor:1;
  short int shutterMotorEn:1;
  short int shutterMotorDirn:1;
} domeHWControl_t;
domeHWControl_t domeHW;

typedef struct {
  int cmd;
  String cmdName;
  int value;
  long int clientId;
  long int transId;
} cmdItem_t;
  
#ifdef _USE_ENCODER_FOR_DOME_ROTATION
const int defaultEncoderTicksPerRevolution = 2048;
const unsigned long int defaultEncoderCountsPerDomeRev = defaultEncoderTicksPerRevolution * (48/60/25.40);
#endif

//I2C device address list
const uint8_t compassAddr = 0x0d;
const uint8_t motorControllerAddr = 181;
const uint8_t LCDControllerAddr = 81;
//barometer
//rain
//temperature
//battery voltage  - use the internal ADC by preference

//Setup the network characteristics
const char ssid[nameLengthLimit] = "";
const char pw[nameLengthLimit] = "";
char hostname[nameLengthLimit] = "ESPDom01";

//Dome Encoder information
// azTick_value = encoder.read();
int encoderTicksPerRevolution = defaultEncoderTicksPerRevolution;

//Internal variables
bool abortFlag = false; 
int domeSyncOffsetDegrees = 0;
float targetAzimuth = 0.0F;
float currentAzimuth = 0.0F;
int currentAltitude = 0;
int targetAltitude = 0;
int domeStatus = DOME_IDLE;
int domeLastStatus = domeStatus;
int shutterStatus = SHUTTER_CLOSED;
volatile boolean coarseTimerFlag = false;
volatile boolean fineTimerFlag = false;
LinkedList <cmdItem_t*> domeCmdList;
LinkedList <cmdItem_t*> shutterCmdList;
int motorSpeed = MOTOR_SPEED_OFF; 
int motorDirection = MOTOR_DIRN_CCW;
//add to custom setup page
bool parkDomeOnDisconnect = false;
bool closeShutterOnDisconnect = false;

// Create an instance of the server
// specify the port to listen on as an argument
ESP8266WebServer server(80);

//REST API client
WiFiClient wClient;

//JSON data formatter
DynamicJsonBuffer jsonBuffer(256);

//Encoder interface 
#if defined _USE_ENCODER_FOR_DOME_ROTATION
Encoder encoder(ENCODER_A_PIN, ENCODER_B_PIN );
#endif

//Hardware device system functions - reset/restart etc
EspClass device;
long int nowTime, startTime, indexTime;
ETSTimer fineTimer, coarseTimer;
void onCoarseTimer();
void onFineTimer();

//Private web handler methods

//internal functions
void setupMotor();
#ifdef _USE_ENCODER_FOR_DOME_ROTATION
void setupEncoder();
#else
void setupCompass();
#endif
void saveToEeprom();
void readFromEeprom();
void fineTimerHandler(void);
void coarseTimerHandler(void);
cmdItem_t* addDomeCmd( uint32_t clientId, uint32_t transId, enum domeCmd, int value );
cmdItem_t* addShutterCmd( uint32_t clientId, uint32_t transId, enum shutterCmd, int value );

//ASCOM variables 
unsigned int transactionId;
unsigned int clientId;
bool connected = false;
const String DriverName = F("Skybadger.ESPDome");
const String DriverVersion = F("0.0.1");
const String DriverInfo = F("Skybadger.ESPDome RESTful native device. ");
const String Description = F("Skybadger ESP2866-based wireless ASCOM Dome controller");
const String InterfaceVersion = F("2");
String Name = hostname;

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
const bool slaved = false;
const bool slewing = false;

#include "JSONHelperFunctions.h"
#include "ASCOMAPICommon_rest.h"
#include "ASCOMAPIDome_rest.h"
#include "ASCOM_DomeSetup.h"
#include "I2CLCD.h"
#include "i2cMotor.h"
I2CLCD myLCD( LCDControllerAddr, 16, 2);
i2cMotor myMotor( motorControllerAddr );

/////////////////////////////////////////////////////////////////////////////////
void setup()
{
  int i=0;
  // put your setup code here, to run once:
  //Minimise serial to one pin only. 
  Serial.begin( 115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println("/////////////////////////////////////////////////////////");
  Serial.println("ESP stepper starting:uses step and direction only.");

  //Read internal state
  readEeprom(); //Contains user-specified and saved hostname, hence read first.
  
  //Setup Wifi
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname);
  WiFi.begin( ssid, pw );
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);//This delay is essentially for the DHCP response. Shouldn't be required for static config.
    Serial.print(".");
  }
  Serial.print("\n Connected, IP address: ");  
  Serial.println(WiFi.localIP());
  Serial.println();

  //Start NTP client
  configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
  
  //Setup I2C
#if defined _ESP_8266_01_
  //Pins mode and direction setup for i2c on ESP8266-01
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, INPUT_PULLUP);
    
  //I2C setup SDA pin 0, SCL pin 2
  Wire.begin(0, 2);
  Serial.println("Configured pins for ESP8266-01");
#else
  //Pins mode and direction setup for i2c on ESP8266-12
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(3, INPUT_PULLUP);
  //setup pins 13, 14, 15 for encoder 
  //setup pin 11 to be analogue in for battery voltage sensing.   
  pinMode( 11, INPUT );
  //I2C setup SDA pin 0, SCL pin 2
  Wire.begin(4, 5);
  Serial.println("Configured pins for ESP8266-12");
#endif
  Wire.setClock(100000 );//100KHz target rate

  //Setup domeHW state 
  
  //Setup i2C to drive motor controller
  setupDomeMotor( );
  //remote device on the rotating dome part
  setupShutterMotor( );
  //Setup i2c to control LCD display
  //myLCD.checkLCD();
  myLCD.clearScreen();
  myLCD.setBacklight( true );  
  
  //Use one of these to track the dome position
 #ifdef _USE_ENCODER_FOR_DOME_ROTATION
  setupEncoder();
 #else
  //Magnetometer setup 
  setupCompass();
 #endif
  
  //Register Web server handler functions 
  //ASCOM dome-specific functions
  server.on("/api/v1/dome/0/altitude", HTTP_GET, handleAltitudeGet );
  server.on("/api/v1/dome/0/athome", HTTP_GET,handleAtHomeGet );
  server.on("/api/v1/dome/0/atpark", HTTP_GET,handleAtParkGet );
  server.on("/api/v1/dome/0/azimuth", HTTP_GET,handleAzimuthGet);
  server.on("/api/v1/dome/0/canfindhome", HTTP_GET, handleCanFindHomeGet);
  server.on("/api/v1/dome/0/canpark", HTTP_GET, handleCanParkGet);
  server.on("/api/v1/dome/0/cansetaltitude", HTTP_GET, handleCanSetAltitudeGet);
  server.on("/api/v1/dome/0/cansetazimuth", HTTP_GET, handleCanSetAzimuthGet);
  server.on("/api/v1/dome/0/cansetpark", HTTP_GET, handleCanSetParkGet);
  server.on("/api/v1/dome/0/cansetshutter", HTTP_GET, handleCanSetShutterGet);
  server.on("/api/v1/dome/0/canslave", HTTP_GET, handleCanSlaveGet);
  server.on("/api/v1/dome/0/cansyncazimuth", HTTP_GET, handleCanSyncAzimuthGet);
  server.on("/api/v1/dome/0/shutterstatus", HTTP_GET, handleShutterStatusGet);
  server.on("/api/v1/dome/0/slaved", HTTP_GET,handleSlavedGet);
  server.on("/api/v1/dome/0/slaved", HTTP_PUT,handleSlavedPut);
  server.on("/api/v1/dome/0/slewing", HTTP_GET,handleSlewingGet);
  server.on("/api/v1/dome/0/abortslew", HTTP_PUT,handleAbortSlewPut);
  server.on("/api/v1/dome/0/closeshutter", HTTP_PUT,handleCloseShutterPut);
  server.on("/api/v1/dome/0/findhome", HTTP_PUT,handleFindHomePut);
  server.on("/api/v1/dome/0/openshutter", HTTP_PUT,handleOpenShutterPut);
  server.on("/api/v1/dome/0/park", HTTP_PUT,handleParkPut);
  server.on("/api/v1/dome/0/setpark", HTTP_PUT,handleSetParkPut);
  server.on("/api/v1/dome/0/slewtoaltitude", HTTP_PUT,handleSlewToAltitudePut);
  server.on("/api/v1/dome/0/slewtoazimuth", HTTP_PUT,handleSlewToAzimuthPut);
  server.on("/api/v1/dome/0/synctoazimuth", HTTP_PUT,handleSyncToAzimuthPut);
  
//Common ASCOM function handlers
  server.on("/api/v1/dome/0/Dome/0/Action", HTTP_PUT, handleAction );
  server.on("/api/v1/dome/0/Dome/0/CommandBlind", HTTP_PUT, handleCommandBlind );
  server.on("/api/v1/dome/0/Dome/0/CommandBool",HTTP_PUT, handleCommandBool );
  server.on("/api/v1/dome/0/Dome/0/CommandString",HTTP_GET, handleCommandString );
  server.on("/api/v1/dome/0/Dome/0/Connected", handleConnected );
  server.on("/api/v1/dome/0/Dome/0/Description", HTTP_GET, handleDescriptionGet );
  server.on("/api/v1/dome/0/Dome/0/DriverInfo", HTTP_GET, handleDriverInfoGet );
  server.on("/api/v1/dome/0/Dome/0/DriverVersion", HTTP_GET, handleDriverVersionGet );
  server.on("/api/v1/dome/0/Dome/0/Name", HTTP_GET, handleNameGet );
  server.on("/api/v1/dome/0/Dome/0/Actions", HTTP_GET, handleSupportedActionsGet );
  
  //Custom and setup handlers used by the custom setup form 
  //ASCOM ALPACA doesnt support any way to tell the REST device some of its basic setup constants so you have to 
  //do something like this. Or a serial interface .. ITs a legacy of expecting a windows exe driver thing.
  server.on("/", HTTP_GET, handleSetup);
  //HTML forms don't support PUT - use GET instead.
  server.on("/Dome/0/Sync", HTTP_GET, handleSyncOffsetPut );
  //  server.on("/Dome/0/SetHome", HTTP_GET, handleFilterNamesPut );
  server.on("/Dome/0/Hostname",    HTTP_GET, handleHostnamePut );
  server.on("/Dome/0/DomeName",   HTTP_GET, handleNamePut );
  server.onNotFound(handlerNotFound);

  //setup interrupt-based 'soft' alarm handler for dome state update and async commands
  ets_timer_setfn( &fineTimer, onFineTimer, NULL ); 
  ets_timer_setfn( &coarseTimer, onCoarseTimer, NULL ); 

  //Setup sleep parameters
  wifi_set_sleep_type(LIGHT_SLEEP_T);

  //Start web server
  server.begin();

  DEBUGSL1( "setup complete");
  
  //Show welcome message
  
  //Start timers last
  ets_timer_arm_new( &coarseTimer, 10000000, 1/*repeat*/, 0);
  ets_timer_arm_new( &fineTimer, 100000, 1/*repeat*/, 0);
}

void onFineTimer( void* pArg )
{
  //Read command list and apply. 
  fineTimerFlag = true;
}

void onCoarseTimer( void* pArg )
{
  //Read command list and apply. 
  coarseTimerFlag = true;
}

void loop()
{
	String outbuf;
  long int nowTime = system_get_time();
  long int indexTime = startTime - nowTime;
  
  //Grab a copy in case it changes while we are running.
  bool coarseTimer = coarseTimerFlag;
  bool fineTimer = fineTimerFlag;
  
  // Main code here, to run repeatedly:
  //frequency timing test goes here - use system clock counter

  //Handle state changes
  //in dome  
  switch ( domeStatus )
  {
    case DOME_IDLE:
      onDomeIdle();
      break;
    case DOME_SLEWING:
      onDomeSlew();    
    case DOME_ABORT:
      onDomeAbort();     
    break;
  }
  
  //in shutter
  switch( shutterStatus )
  {
    case SHUTTER_CLOSED:
      onShutterIdle();
         break;
    case SHUTTER_OPENING:
    case SHUTTER_CLOSING:    
       onShutterSlew(); 
          break;
    case SHUTTER_OPEN:
      onShutterIdle(); 
         break;
          break;
  }

  //If there are any client connections - handle them.
  server.handleClient();

  //Clear down flags
  if( fineTimer )
    fineTimerFlag = false;
  else if ( coarseTimer )
    coarseTimerFlag = false;
  }

  /*
   * Handle the command list 
   */
  void onDomeIdle( void )
  {
    cmdItem_t* pCmd = NULLPTR;
    if ( domeCmdList.size() > 0 ) 
      pCmd = domeCmdList.pop();
    
    switch ( pCmd->cmd )
    {
      case CMD_DOME_SLEW:
      if ( ) //CW
      (
       )
      else
      {
        
      }
      case CMD_DOME_ABORT:
        //clear down command list
        domeCmdList.clear();
        //turn off motor
        myMotor.init();
        //message to LCD
        myLCD.write( 2, 0, "ABORT received - dome halted." );
        // ? close shutter ?
        break;
      case CMD_DOMEVAR_SET:
        //Did this so that dome variables aren''t updated while waiting for commands that preceeded them to execute. 
        
      default:
      break;
    }
    return;
  }
  
  void onDomeSlew( void )
  {
    return;
  }
  
  void onDomeAbort( void )
  {
    //stop motor. 
    //Check motor stopped
    //Update LCD
    return;
  }

  void onShutterIdle()
  {
    //Get next command if there is one. 
    ;
  }
  
  void onShutterSlew()
  {
    ;;
  }

  void onShutterAbort( void )
  {
    ;;
  } 
  
  void setupEncoder()
  {
    return;
  }

  void setupDomeMotor( void )
  {
    myMotor.checkMotorDevice();
    myMotor.initMotorDevice();
  }
  
  void setupShutterMotor( void )
  {
    
  }
  
  bool setupCompass( String targetHost )
  {
    //Assumes use of a QMC5883L which has a different address and register layout from HMC5883L
    //Assumes wire.begin has set the right pins to use for i2c comms. 
    bool value = false;;
    //client compass.begin();
    
    return value;
  }

  void readEeprom()
  {
    return;
  }

  void saveEeprom()
  {
  }
  
  void coarseTimerHandler(void)
  {
    coarseTimerFlag = true;
  }
  
  void fineTimerHandler(void)
  {
    fineTimerFlag = true;
  }

  cmdItem_t* addDomeCmd( uint32_t clientId, uint32_t transId, enum domeCmd newCmd, int value )
  {
      //Create new command
      cmdItem_t* pCmd = (cmdItem_t*) calloc( sizeof( cmdItem_t ), 1); 
      //Add to list 
      pCmd->cmd = (int) newCmd;
      pCmd->cmdArg = value;
      pCmd->clientId = clientId;
      pCmd->transId = transId; 
      domeCmdList.add( pCmd );
      return pCmd;
  }

  cmdItem_t* addShutterCmd( uint32_t clientId, uint32_t transId, enum shutterCmd newCmd, int value )
  {
      //Create new command
      cmdItem_t* pCmd = (cmdItem_t*) calloc( sizeof( cmdItem_t ), 1); 
      //Add to list 
      pCmd->cmd = (int) newCmd;
      pCmd->cmdArg = value;
      pCmd->clientId = clientId;
      pCmd->transId = transId; 
      shutterCmdList.add( pCmd );
      return pCmd;
  }

  //Wrapper for web server args handing that identifies arguments that differ by case. The ALPACA API allows this!
  bool hasArgic( String& check, ESP8266WebServer& ref, bool caseSensitive )
  {
    String matchName;
    if ( caseSensitive )
    {
      int max = server.args();
      for ( int i = 0; i < max; i++ )
      {
        if( server.argName(i).equalsIgnoreCase( check ) )  
        {
          check = server.arg[i];          
          return true;
        }
      }
      return false;
    }
    else
      return ( server.hasArg( check ) )
  }
