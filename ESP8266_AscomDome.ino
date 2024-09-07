/*
   This program uses an esp8266 board to host a web server providing a rest API to control an ascom Dome
   The dome hardware can come in up to two parts 
   In the first configuration, the dome controller consists of a motor, display, battery sensor and 
   position sensors mounted all together on the dome wall and using i2c to talk to the 'local' devices.
   The second configuration is where the motor and display controllers are attached to the dome wall and the rotation sensor
   is mounted on the rotating dome itself. This sensor is typically a digital compass and accessed via its REST API. (see ESP8266_Mag.ino).
   In either case the part fixed to the wall is the sole interface to ASCOM so the ASCOM driver only has one interface to deal with. 
   This hardware supports the ALPACA rest API interface so there is no need for an installable driver on the observatory controller server. 
   Configure the rest interface to use this device's hostname (ESPDome01) and port (80)
   
   Implementing.
   EEprom partially in place - needs ping -t testing
   Add MQTT callbacks for health - done - need to measure and add publish for Dome voltage
   
   Testing.
   Curl resting of command handlers for ALPACA appears good. Used with DeviceHub for get/can calls testing 
   Need to test with ALPACA - works fine
   Need to test with ALPACA Management
   Bug fixed when shutter and encoder not available - last bearing is set to static lastBearing = 0. Fixed. I think.
      
   Operating. 
   Check power demand and manage
   
   To do
   1, Add MQTT for dome orientation - not done 
   2, dome setup to support calc of auto-track pointing - maybe
   3, Add list handler - done
   4, Update smd list to handle variable settings and fixup responses for async responses. Report to ascom community
   Test handler functions 
   5, Add url to query for status of async operations. Consider whether user just needs to call for status again. 
   6, Fix EEPROM handling - done. 
   7, Add ALPACA mgmt API and UDP discovery call handling - mostly done - all in plce but still not recognised.!
   8, zero-justify seconds and minutes in sprintf time string - done
   9, Fix LCD detection even when not present.. 
   10, Fix atPark and atHome to handle wrapped locations - done
   11, Fix error handling for failed remote calls to shutter - done
   12, Provide option to not use connected client id for use in brownouts. 
   13, Adjust slow rate to almost max rate. 
   14, add call to set slow rates and max rates via API. 
   15, Add action to backup crawling over lockups and attempt at full speed. 

To test:
 Download curl for your operating system. Linux variants and Windows powershell should already have it.
 e.g. curl -v -X PUT -d "homePosition=270&ClientID=99&ClientTransactionID=99" http://EspDom01/API/v1/Dome/0/setHome
 Use the ALPACA REST API docs at <> to validate the dome behaviour. At some point there will be a script

External Dependencies:
Liked List https://github.com/ivanseidel/LinkedList
ArduinoJSON library 5.13 ( moving to 6 is a big change) 
pubsub library for MQTT 
ALPACA for ASCOM 6.5
Expressif ESP8266 board library for arduino - configured for v2.7
EEPROMAnything library - added string handler since it doesn't do char* strings well
GDB - not really used
RemoteDebug - used more and more. 
+
ESP8266-12 Huzzah
  /INT - GPIO3
  [I2C SDA - GPIO4  SCL - GPIO5]
  [SPI SCK = GPIO #14 (default)  SPI MOSI = GPIO #13 (default)  SPI MISO = GPIO #12 (default)]
  ENC_A
  ENC_B
  ENC_H
  
  Motor controller - i2c
  LCD Display - i2c

Change Log
21/05/2021 Added last will and testament to MQTT and use of dirty sessions rather than fresh ones. 
31/05/2021 Updated the setup handler page presentation to chunk the content  - needs testing. SPlits down page into a s etup of content transfers rather than 
tries to send a whole page. 
01/06/2021 Added last reset reason and binary version info (date) to first publishHealth response
01/06/2021 Updated handleSlavedGet to return value of slaved setting. PUT command always returns NotImplementedException.
01/06/2021 handleAbortSlewPut updated to remove InvalidOperation exception when Abort is called and system is not slewing
01/06/2021 Updated slewToAltitudePut to respect canSetAltitude for Conform compliance. 
           Also added exception handling for out of range values.
01/06/2021 Updated handleSlewToAzimuthPut to enforce value limits by invalidValue Exception
           Causes actual azimuth to be updated asynchronously - this may yet be too slow for Conform. 
01/06/2021 Amended OnSlew to manage the zero hunting issue where the fast speed is selected for a small reversal slew. 
29/09/2021 Amended onSlew to manage detection of atHome and atPark and speed determination when near to zero/360
02/10/2021 Fixed a bug where a call to shutter may fail and return error rather than keeping last state. Now tracks response state. Error state upsets Voyager. 
26/10/2021 Changed free(pCmd) to freeCmd( pCmd) since it appears to be leaking memory by not releasing Strings properly. chnaged Strings to char* & free xplicitly.
26/10/2021 //TODO do similar while loop to detect encoder readiness. - done
16/11/2021 Added lots of debugV statements to get to bottom of memory leak. 
Bugs
Theres a bug in here somewhere which causes a reboot and a client clost connection occasionally enough to be a problem, causing Voyager to lose 'connected' state
remedy is to re-issue call to connect using curl by hand using Voyager's last clientID. Or fix at source. 
checkout : https://arduino-esp8266.readthedocs.io/en/latest/PROGMEM.html#declare-a-flash-string-within-code-block for moving strings into PROGMEM to reduce the memory footprint. 
F("myString") stores the string into PROGMEM and makes accessible to string functions that can manage access to that memory
Serial.println( F("myString") ) is ok
Serial.printf_P( PSTR("myString") ) is needed as too the progmem functions for more extensive string loading into heap memory from flash. 
22/11/2021 Memleak bug apparently fixed by moving hClient.end() to end of statements in restQuery - it was occassionally being missed caused by a certain combination of errors.
leads to memory in rest queries not being released and memory decreasing by about 1300 bytes per miss. 
*/

/////////////////////////////////////////////////////////////////////////////////

//Internal variables
#include "ESP8266_AscomDome.h"   //App variables - pulls in the other include files - its all in there.
#include "Skybadger_common_funcs.h"
#include "ASCOMAPICommon_rest.h"
#include "ASCOMAPIDome_rest.h"
#include "JSONHelperFunctions.h"
#include "AlpacaManagement.h"
#include "ASCOM_DomeCmds.h"
#include "ASCOM_Domehandler.h"
#include "ASCOM_DomeSetup.h"
#include "ASCOM_DomeEeprom.h"

void setup( void );
void setupWifi( void );
void publishFnStatus( void );
uint32_t reportRam(char* );

void setup()
{
  // put your setup code here, to run once:
  String outbuf;
  String path;
  int response = 403;
  
  //Minimise serial to one pin only. 
  Serial.begin( 115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println(F("ESP starting."));
  //gdbstub_init();
  delay(500); 

  //Start time
  configTime(TZ_SEC, DST_MN, timeServer1, timeServer2, timeServer3 );
  Serial.println( F("Time Services setup") );
      
  //Read internal state, apply defaults if we can't find user-set values in Eeprom.
  EEPROM.begin(eepromSize);
  setupDefaults();
  readFromEeprom();    
  //delay(5000);
    
  //Setup WiFi
  Serial.printf_P( PSTR("Entering Wifi setup for host %s\n") , myHostname );
  setupWifi();

#if !defined DEBUG_DISABLED
  //Debugging over telnet setup
  // Initialize the server (telnet or web socket) of RemoteDebug
  Debug.begin( WiFi.hostname().c_str(), Debug.ERROR ); 
  Debug.setSerialEnabled(true);//until set false 
  // Options
  // Debug.setResetCmdEnabled(true); // Enable the reset command
  // Debug.showProfiler(true); // To show profiler - time between messages of Debug
  //In practice still need to use serial commands until debugger is up and running.. 
  DEBUGSL1( F("Remote debugger enabled and operating") );
#endif

  //for use in debugging reset - may need to move 
  Serial.printf_P( PSTR( "Device reset reason: %s\n" ), device.getResetReason().c_str() );
  Serial.printf_P( PSTR( "device reset info: %s\n" ),   device.getResetInfo().c_str() );

  //Setup I2C
#if defined _ESP8266_01_
  //Pins mode and direction setup for i2c on ESP8266-01
  pinMode(0, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
    
  //I2C setup SDA pin 0, SCL pin 2
  //Normally Wire.begin(0, 2);
  Wire.begin( 2,0 /*0, 2*/);  
  Serial.println( F( "Configured pins for ESP8266-01\n") );
#else //__ESP8266_12_
  //Pins mode and direction setup for i2c on ESP8266-12
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  //I2C setup SDA GPIO 5, SCL GPIO4 4
  Wire.begin(4, 5);
  
  //setup pins 13, 14, 15 for encoder (only need two + home if available.)
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  Serial.println( F("Configured pins for ESP8266-12\n") );
#endif
  Wire.setClock(100000 );//100KHz target rate is a bit hopeful for this device 
////////////////////////////////////////////////////////////////////////////////////////

  outbuf = scanI2CBus();
  debugD( "I2CScan: %s", outbuf.c_str() );

////////////////////////////////////////////////////////////////////////////////////////
  //Open a connection to MQTT
  //MQTTServerName is in EEProm, as too ThisID.
  Serial.printf_P( PSTR( ("Configuring MQTT connection to :%s\n") ), MQTTServerName );
  client.setServer( MQTTServerName, 1883 );
  Serial.printf_P( PSTR(" MQTT settings id: %s user: %s pwd: %s\n"), thisID, pubsubUserID, pubsubUserPwd );

  //want to connect to a 'dirty' session not a clean one, that way we receive any outstanding messages waiting
  //Also setup our last will message 
  String lastWillTopic = String(outHealthTopic);
  lastWillTopic.concat( myHostname ); 
  client.connect( thisID, pubsubUserID, pubsubUserPwd, lastWillTopic.c_str(), 1, true, "Offline", false ); 
    
  //Create a callback that causes this device to publish its health asnd sesnor data over MQTT to the centre
  client.setCallback( callback );
  client.subscribe( inTopic );
  Serial.printf_P(PSTR("Configured MQTT subscription connection: %s\n"), inTopic );
  publishHealth();
     
  //Setup the sensors
  motorPresent = myMotor.check();
  if ( motorPresent )
  {
    myMotor.init();
    myMotor.setSpeedDirection( MOTOR_SPEED_OFF, MOTOR_DIRN_CW );  
    motorSpeed = MOTOR_SPEED_OFF;
    motorDirection = MOTOR_DIRN_CW;
    myMotor.getSpeedDirection();
    Serial.printf_P( PSTR("motor initialised - speed: %u, direction: %u\n"), myMotor.getSpeed(), myMotor.getDirection() );
  }
  else
    Serial.println( F("No motor found on i2c bus\n") );

  //Setup i2c to control LCD display
  debugI("Starting to configure LCD connection");
  lcdPresent = ( myLCD.checkLCD() == 0 )? true: false;
  lcdPresent = false;//above check doesnt work - LCD currently not fitted. 
  if( lcdPresent )
  { 
    myLCD.clearScreen();
    myLCD.setCursor( 1, 1, I2CLCD::CURSOR_UNDERLINE );
    myLCD.setBacklight( true ); 
    myLCD.writeLCD( 4, 1 , "ASCOMDome ready" );
    debugI("LCD found and configured");
  }
  else
  {
    Serial.printf_P( PSTR("No LCD found on i2c bus.\n" ));  
  }
  
  //Use one of these to track the dome position
#ifdef USE_LOCAL_ENCODER_FOR_DOME_ROTATION
  setupEncoder();
#elif defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  //
#elif defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION
  //
#endif
 
  //Register Web server handler functions 
  //ASCOM dome-specific functions
  Serial.printf_P( PSTR("Registering web handlers.\n" ));  
  server.on(F("/api/v1/dome/0/altitude"),                   HTTP_GET, handleAltitudeGet ); //tested - 0
  server.on(F("/api/v1/dome/0/athome"),                     HTTP_GET, handleAtHomeGet );   //tested - returns false ?
  server.on(F("/api/v1/dome/0/atpark"),                     HTTP_GET, handleAtParkGet );   //tested - returns false ?
  server.on(F("/api/v1/dome/0/azimuth"),                    HTTP_GET, handleAzimuthGet);
  server.on(F("/api/v1/dome/0/canfindhome"),                HTTP_GET, handleCanFindHomeGet);
  server.on(F("/api/v1/dome/0/canpark"),                    HTTP_GET, handleCanParkGet);
  server.on(F("/api/v1/dome/0/cansetaltitude"),             HTTP_GET, handleCanSetAltitudeGet);
  server.on(F("/api/v1/dome/0/cansetazimuth"),              HTTP_GET, handleCanSetAzimuthGet);
  server.on(F("/api/v1/dome/0/cansetpark"),                 HTTP_GET, handleCanSetParkGet);
  server.on(F("/api/v1/dome/0/cansetshutter"),             HTTP_GET, handleCanSetShutterGet);
  server.on(F("/api/v1/dome/0/canslave"),                  HTTP_GET, handleCanSlaveGet);
  server.on(F("/api/v1/dome/0/cansyncazimuth"),            HTTP_GET, handleCanSyncAzimuthGet);
  server.on(F("/api/v1/dome/0/shutterstatus"),             HTTP_GET, handleShutterStatusGet);
  server.on(F("/api/v1/dome/0/slaved"),                    HTTP_GET,handleSlavedGet);
  server.on(F("/api/v1/dome/0/slaved"),                    HTTP_PUT,handleSlavedPut);
  server.on(F("/api/v1/dome/0/slewing"),                   HTTP_GET,handleSlewingGet);
  server.on(F("/api/v1/dome/0/abortslew"),                 HTTP_PUT,handleAbortSlewPut);
  server.on(F("/api/v1/dome/0/closeshutter"),              HTTP_PUT,handleCloseShutterPut);
  server.on(F("/api/v1/dome/0/findhome"),                  HTTP_PUT,handleFindHomePut);
  server.on(F("/api/v1/dome/0/openshutter"),               HTTP_PUT,handleOpenShutterPut);
  server.on(F("/api/v1/dome/0/park"),                      HTTP_PUT,handleParkPut);
  server.on(F("/api/v1/dome/0/setpark"),                   HTTP_PUT,handleSetParkPut);
  server.on(F("/api/v1/dome/0/slewtoaltitude"),            HTTP_PUT,handleSlewToAltitudePut);
  server.on(F("/api/v1/dome/0/slewtoazimuth"),             HTTP_PUT,handleSlewToAzimuthPut);
  server.on(F("/api/v1/dome/0/synctoazimuth"),             HTTP_PUT,handleSyncToAzimuthPut);
 
//Common ASCOM function handlers
  server.on(F("/api/v1/dome/0/action"),        HTTP_PUT, handleAction );              //Tested: Not implemented
  server.on(F("/api/v1/dome/0/commandblind"),  HTTP_PUT, handleCommandBlind );        //These two have different responses
  server.on(F("/api/v1/dome/0/commandbool"),   HTTP_PUT, handleCommandBool );         //These two have different responses
  server.on(F("/api/v1/dome/0/commandstring"), HTTP_GET, handleCommandString );       //tested
  server.on(F("/api/v1/dome/0/connected"),               handleConnected );           //tested  
  server.on(F("/api/v1/dome/0/description"),   HTTP_GET, handleDescriptionGet );      //tested
  server.on(F("/api/v1/dome/0/driverinfo"),    HTTP_GET, handleDriverInfoGet );       //freezes/times out
  server.on(F("/api/v1/dome/0/driverversion"), HTTP_GET, handleDriverVersionGet );    //tested
  server.on(F("/api/v1/dome/0/interfaceversion"), HTTP_GET, handleInterfaceVersionGet );    //tested
  server.on(F("/api/v1/dome/0/name"),          HTTP_GET, handleNameGet ); //tested    //tested - doesnt return hostname
  server.on(F("/api/v1/dome/0/supportedactions"),       HTTP_GET, handleSupportedActionsGet ); //tested

  /* ALPACA Management and setup interfaces
   * The main browser setup URL would be http://192.168.1.89:7843/setup
The JSON list of supported interface versions would be available through a GET to http://192.168.1.89:7843/management/apiversions
The JSON list of configured ASCOM devices would be available through a GET to http://192.168.1.89:7843/management/v1/configureddevices
   */
  //Management API
  server.on(F("/management/description"),              HTTP_GET, handleMgmtDescription );
  server.on(F("/management/apiversions"),              HTTP_GET, handleMgmtVersions );
  server.on(F("/management/v1/configureddevices"),     HTTP_GET, handleMgmtConfiguredDevices );
  
  //Custom and setup handlers used by the custom setup form - currently there is no collection of devices.
  server.on(F("/setup"),            HTTP_GET, handleSetup);       //Primary browser web page for the overall collection of devices
  server.on(F("api/v1​/dome/0​/setup"), HTTP_GET, handleSetup); //Browser web page for the instance - 0-indexed 
  
  //HTML forms don't support PUT -  they typically transform them to use GET instead.
  //form commands
  server.on(F("/Hostname"),    handleHostnamePut );
  server.on(F("/ShutterName"), handleShutterNamePut );
  server.on(F("/SensorName"),  handleSensorNamePut );
  server.on(F("/ParkSet"),     handleParkPositionPut );
  server.on(F("/ParkAction"),  handleParkActionPut );
//  server.on(F("/ShutterAction"),handleShutterActionPut );
  server.on(F("/Home"),        handleHomePositionPut );
  server.on(F("/Goto"),        handleDomeGoto );
  server.on(F("/Sync"),        handleSyncOffsetPut );
  server.on(F("/restart"),     handlerRestart );
  server.on(F("/status"),      handlerStatus );
  server.on(F("/"),            handlerStatus);
  
  server.onNotFound(handlerNotFound);
  Serial.println( F("Web handlers registered") );
  
  //setup interrupt-based 'soft' alarm handler for dome state update and async commands
  ets_timer_setfn( &fineTimer,      onFineTimer,     NULL ); 
  ets_timer_setfn( &coarseTimer,    onCoarseTimer,   NULL ); //Used for onIdle processing
  ets_timer_setfn( &timeoutTimer,   onTimeoutTimer,  NULL ); //Used for callback timer. 
  ets_timer_setfn( &watchdogTimer,  onWatchdogTimer, NULL ); //Used for slew failure watchdog
  
  domeCmdList    = new LinkedList <cmdItem_t*>();
  shutterCmdList = new LinkedList <cmdItem_t*>();
  cmdStatusList  = new LinkedList <cmdItem_t*>(); // use to track async completion state. 
  Serial.println( F("LinkedList setup complete") );
  
  //Start web server
  updater.setup( &server );
  server.begin();
  Serial.println( F("webserver setup complete") );
   
  //Get startup values
  domeStatus = DOME_IDLE;
  int requestStatus = 0;
  int attemptCount = 0;
  Serial.printf_P( PSTR("Waiting for shutter\n") );
  do 
  {
    //there's a chance of a watchdog timer timeout here. 
    requestStatus = getShutterStatus ( shutterHostname, shutterStatus );
    Serial.printf( ".");
    delay( 500 );
    yield();
  }
  while ( requestStatus != HTTP_CODE_OK && ++attemptCount < 10 );
  if ( attemptCount < 10  )
    Serial.printf_P( PSTR("Shutter NOT found\n") );
  else
    Serial.printf_P( PSTR("Shutter found OK\n") );
  
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  Serial.printf_P( PSTR("Searching for remote compass/encoder\n") );
  path = String( "http://" );
  path += sensorHostname;
  path += "/bearing";
  attemptCount =0;

  do
  {
    response = restQuery( path, "", outbuf, HTTP_GET );
    debugI("Waiting for remote encoder/compass\n");
    delay(500);
    yield();
  }while ( response != HTTP_CODE_OK && ++attemptCount < 10 );
  if ( attemptCount >= 10 )
    Serial.printf_P( PSTR("Remote compass/encoder NOT FOUND \n") );
  else 
    Serial.printf_P( PSTR("Found remote compass/encoder\n") );

#endif

  //TODO consider some mech of reading actual position and storing last position betwwen power cycles. 
  bearing = getBearing( sensorHostname );
  currentAzimuth = getAzimuth( bearing);
  targetAzimuth = currentAzimuth;
  Serial.printf_P( PSTR("Updated position from encoder\n") );
     
  //Start timers last
  ets_timer_arm_new( &coarseTimer, 2500,        1/*repeat*/, 1);//millis    2.5 seconds 
  ets_timer_arm_new( &fineTimer,   1000,        1/*repeat*/, 1);//millis   1 second - bearing reading. 
  ets_timer_arm_new( &watchdogTimer, 5000,      1/*repeat*/, 1);//millis 5 seconds - watchdog check of dome stuck. 
  ets_timer_arm_new( &timeoutTimer, 2500,       0/*one-shot*/, 1); //MQTT background reconnection timer. 

  //Show welcome message

#if defined _TEST_RAM_
  originalRam = device.getFreeHeap();
  lastRam = originalRam;
  debugV("Starting RAM: %i ", originalRam );
#endif
  
  Serial.println( FPSTR(BuildVersionName) );
  debugI( "setup complete" );
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

//Used to complete timeout actions. 
void onTimeoutTimer( void* pArg )
{
   timeoutFlag = true;
}

void onWatchdogTimer( void* pArg )
{
   watchdogTimerFlag = true; 
}

inline uint32_t checkRam( const char* location )
{
    //Check heap for memory bugs 
    uint32_t ram = 0;
    ram = ESP.getFreeHeap();
    if( lastRam != ( ram - originalRam ) )
    {
      lastRam = ram - originalRam;
#if defined DEBUG_DISABLED
      Serial.printf_P( PSTR( "%s RAM: %d \n"), location, ram );
#else 
      debugV( "%s RAM: %u change: %d\n", location, ram, lastRam );
#endif
    }
    return ram;   
}

void loop()
{
	String outbuf;
  String LCDOutput = "";
  
#if defined _MEMLEAK_CHECK_DEBUG
  //Check ram state on entry
  checkRam( "LoopEntry" );
#endif 
  
  //Operate and Clear down flags
  if( fineTimerFlag )
  {
#if defined _ENABLE_BEARING 
    checkRam( "FineTimerEntry" );
    bearing = getBearing( sensorHostname );
    currentAzimuth = getAzimuth( bearing);
    debugD( "Bearing %03.2f, offset: %f, adjusted: %f\n", bearing, azimuthSyncOffset, currentAzimuth );
    checkRam( "FineTimerExit" );
#endif 
    fineTimerFlag = false;
  }

  if ( coarseTimerFlag )
  {
    // Main code here, to run repeatedly:
    //Handle state changes
#if defined _ENABLE_DOME
    checkRam( "DomeEntry" );
    //For dome  
    switch ( domeStatus )
    {
      case DOME_IDLE:    onDomeIdle();
                         break;
      case DOME_SLEWING: onDomeSlew(); 
                         break;   
      case DOME_ABORT:   onDomeAbort();     
                         break;
      default:      
        debugE( "Unexpected Dome status detected: %s\n", domeStateNames[(int) domeStatus ]);
        domeStatus = DOME_ABORT; //error condition
        break;
    }
    checkRam( "DomeExit");
#endif

#if defined _ENABLE_SHUTTER
    checkRam( "ShutterEntry" );
    //For shutter
    //Update our knowledge of shutter current status
    if ( getShutterStatus( shutterHostname, shutterStatus  ) == HTTP_CODE_OK )
    {
      debugD( "Dome: %s Shutter: %s\n", domeStateNames[(int)domeStatus], shutterStateNames[(int)shutterStatus] );
    }

    switch( shutterStatus )
    {
      //These are the idle states for the shutter
      case SHUTTER_ERROR:
      case SHUTTER_CLOSED:
      case SHUTTER_OPEN:   
                           onShutterIdle();
                           break;
      //The shutter is currently doing things so wait until complete or error.
      case SHUTTER_OPENING:
      case SHUTTER_CLOSING: 
                           break;
      default://Anything else. 
            debugE("Shutter status unexpected: %s", shutterStateNames[(int)shutterStatus] );
            shutterStatus = SHUTTER_ERROR;
           break;
    }
    checkRam( "ShutterExit" );
#endif

    //Clock tick onLCD 
    if ( lcdPresent ) 
    {
      int index = 0;
      int lastIndex = 0;
      String output = "";
      getTimeAsString( outbuf );
      index = outbuf.indexOf( " " );
      lastIndex = outbuf.indexOf( "." );
      if( index >= 0 && lastIndex >= index )
      {
        LCDOutput = outbuf.substring( index, lastIndex );
        myLCD.writeLCD( 1,1, LCDOutput );
      } 
    }

    coarseTimerFlag = false;
  }
 
  if ( client.connected() )
  {  
    //Service MQTT keep-alives
    client.loop();
    if (callbackFlag ) 
    {
      //publish results
      publishHealth();
      publishFnStatus();
      callbackFlag = false;
    }
  }
  else
  {
      reconnectNB();
      client.subscribe( inTopic, 1 );
  }

  //If there are any web client connections - handle them.
  server.handleClient();

  // Remote debug over WiFi
  debugHandle(); // Equal to SerialDebug  

  //Check for Alpaca Discovery packets
  handleManagement();

  //Final memory check
#if defined MEM_CHECK_DEBUG
  checkRam( "LoopExit" );
#endif
}
   
/* MQTT callback for subscription and topic.
 * Only respond to valid states ""
 * Publish under ~/skybadger/sensors/<sensor type>/<host>
 * Note that messages have an maximum length limit of 18 bytes - set in the MQTT header file. 
 */
void callback(char* topic, byte* payload, unsigned int length) 
{  
  //set callback flag
  callbackFlag = true;  
 }

void publishFnStatus( void )
 {
  String outTopic;
  String output;
  String timestamp;
  
  //publish to our device topic(s)
  DynamicJsonBuffer jsonBuffer(250);  
  JsonObject& root = jsonBuffer.createObject();

  getTimeAsString2( timestamp );
  root["time"] = timestamp;
  root["hostname"] = myHostname;
  root["azimuth"] = currentAzimuth;
  root["altitude"] = currentAltitude;
  root["syncOffset"] = azimuthSyncOffset;
  root["shutterStatus"] = shutterStateNames[(int) shutterStatus];
  root["domeStatus"] = domeStateNames[(int) domeStatus];
  root.printTo( output );
    
  outTopic = String( outFnTopic );        //PROGMEM
  outTopic.concat( String( DriverType) ); //PROGMEM
  outTopic.concat("/");
  outTopic.concat( myHostname );
  
  //publish with retention
  if( client.publish( outTopic.c_str(), output.c_str(), true ) )  
  {
    debugI( "MQTT FN topic published: %s ", output.c_str() ); 
  }
  else
  {
    debugW( "MQTT FN topic failed to publish: %s ", output.c_str() );   
  }
 }

/*
 @brief Publish health status to MQTT based on callback process. 
 */
 void publishHealth(  )
 {
  String outTopic;
  String output;
  String timestamp;
  
  //publish to our device topic(s)
  DynamicJsonBuffer jsonBuffer(250);  
  JsonObject& root = jsonBuffer.createObject();

  getTimeAsString2( timestamp );
  root["time"] = timestamp;

  // Once connected, publish an announcement...
  root["hostname"] = myHostname;
  if( connected != NOT_CONNECTED ) 
    root["message"] = F("Dome connected & operating");
  else
    root["message"] = F("Dome waiting for connection");
  root.printTo( output );

  //do once after reboot only. 
  if ( bootCount == 0 )
  {
    String buildVersion = String( __DATE__ ) + " " + FPSTR( BuildVersionName );
    root[ "version" ] = buildVersion.c_str();
    root[ "resetreason" ] = device.getResetReason().c_str();
    root[ "resetinfo" ] = device.getResetInfo().c_str();
    bootCount++;
  }
  
  outTopic = String( outHealthTopic ); //PROGMEM
  outTopic.concat( myHostname );
  
  if( client.publish( outTopic.c_str(), output.c_str(), true ) )  
  {
    debugI("MQTT Health topic: %s: output: %s", outTopic.c_str(), output.c_str() );
  }
  else
  {
    debugW("MQTT Health topic failed : %s: output: %s", outTopic.c_str(), output.c_str() );
  }
 }

void setupWifi( void )
{
  int zz = 0;

  WiFi.mode(WIFI_STA);
  WiFi.hostname( myHostname );  

  WiFi.begin( String(ssid1).c_str(), String(password1).c_str() );
  Serial.print("Searching for WiFi..");
  
  while (WiFi.status() != WL_CONNECTED) 
  {
     delay(500);
     Serial.print(".");
   if (zz++ >= 400) 
    {
      device.restart();
    }    
  }

  Serial.println( F("WiFi connected") );
  Serial.printf_P( PSTR("SSID: %s, Signal strength %i dBm \n\r"), WiFi.SSID().c_str(), WiFi.RSSI() );
  Serial.printf_P( PSTR("Hostname: %s\n\r"),       WiFi.hostname().c_str() );
  Serial.printf_P( PSTR("IP address: %s\n\r"),     WiFi.localIP().toString().c_str() );
  Serial.printf_P( PSTR("DNS address 0: %s\n\r"),  WiFi.dnsIP(0).toString().c_str() );
  Serial.printf_P( PSTR("DNS address 1: %s\n\r"),  WiFi.dnsIP(1).toString().c_str() );
  delay(500);

  //Setup sleep parameters
  wifi_set_sleep_type(NONE_SLEEP_T);

  Serial.println( F("WiFi connected" ) );
  delay(500);
}
