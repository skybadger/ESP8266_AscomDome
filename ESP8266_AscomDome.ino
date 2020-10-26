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
   Need to test with ALPACA
      
   Operating. 
   Check power demand and manage
   
   To do
   1, Add MQTT for Bz, temp and dome orientation - not doing.
   2, dome setup to support calc of auto-track pointing - maybe
   3, Add list handler - done
   4, Update smd list to handle variable settings and fixup responses for async responses. Report to ascom community
   Test handler functions 
   5, Add url to query for status of async operations. Consider whether user just needs to call for status again. 
   6, Fix EEPROM handling - done. 
   7, Add ALPACA mgmt API and UDP discovery call handling
   8, zero-justify seconds and minutes in sprintf time string.
   9, Fix LCD detection even when not present.. 

To test:
 Download curl for your operating system. Linux variants and Windows powershell should already have it.
 e.g. curl -v -X PUT -d 'homePosition=270' http://EspDom01/API/v1/Dome/0/setHome
 Use the ALPACA REST API docs at <> to validate the dome behaviour. At some point there will be a script

External Dependencies:
ArduinoJSON library 5.13 ( moving to 6 is a big change) 
pubsub library for MQTT 
ALPACA for ASCOM 6.5
Expressif ESP8266 board library for arduino - configured for v2.5
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
*/

/////////////////////////////////////////////////////////////////////////////////

//Internal variables
#include "SkybadgerStrings.h"
#include "ESP8266_AscomDome.h"   //App variables - pulls in the other include files - its all in there.
#include "Skybadger_common_funcs.h"
#include "ASCOMAPICommon_rest.h"
#include "ASCOMAPIDome_rest.h"
#include "JSONHelperFunctions.h"
#include "ASCOM_DomeCmds.h"
#include "ASCOM_Domehandler.h"
#include "ASCOM_DomeSetup.h"
#include "ASCOM_DomeEeprom.h"

void scanNet(void)
{
  int i;
  int n;
  WiFi.disconnect();
  n = WiFi.scanNetworks(false, true);
  if (n == 0) 
  {
    Serial.println("(no networks found)");
  } 
  else 
  {
    Serial.println(" networks found ");
    for (int i = 0; i < n; ++i)
    {
      Serial.printf( "ID: %s, Strength: %i \n", WiFi.SSID(i).c_str(), WiFi.RSSI(i) );
    }
  }
}

void setupWifi(void)
{
  //Setup Wifi
  int zz = 00;
  WiFi.mode(WIFI_STA);
  WiFi.hostname( myHostname );

  scanNet( );
  
  WiFi.begin( ssid2, password2 );  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);//This delay is essentially for the DHCP response. Shouldn't be required for static config.
    Serial.print(".");
    if ( zz++ > 200 )
    {
      Serial.println("Restarting to try to find a connection");
      device.restart();
    }
  }
  Serial.println("WiFi connected");
  Serial.printf("SSID: %s, Signal strength %i dBm \n\r", WiFi.SSID().c_str(), WiFi.RSSI() );
  Serial.printf("Hostname: %s\n\r",       WiFi.hostname().c_str() );
  Serial.printf("IP address: %s\n\r",     WiFi.localIP().toString().c_str() );
  Serial.printf("DNS address 0: %s\n\r",  WiFi.dnsIP(0).toString().c_str() );
  Serial.printf("DNS address 1: %s\n\r",  WiFi.dnsIP(1).toString().c_str() );

  //Setup sleep parameters
  //wifi_set_sleep_type(LIGHT_SLEEP_T);
  wifi_set_sleep_type(NONE_SLEEP_T);
  delay(5000);

  Serial.println("WiFi setup complete & connected");
}

void setup()
{
  // put your setup code here, to run once:
  String outbuf;

  //Minimise serial to one pin only. 
  Serial.begin( 115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println(F("ESP starting."));
  //gdbstub_init();
  delay(5000); 

  //Debugging over telnet setup
  // Initialize the server (telnet or web socket) of RemoteDebug
  //Debug.begin(HOST_NAME, startingDebugLevel );
  Debug.begin( WiFi.hostname().c_str(), Debug.ERROR ); 
  Debug.setSerialEnabled(true);//until set false 
  // Options
  // Debug.setResetCmdEnabled(true); // Enable the reset command
  // Debug.showProfiler(true); // To show profiler - time between messages of Debug
  //In practice still need to use serial commands until debugger is up and running.. 
  debugE("Remote debugger enabled and operating");

  //Start time
  configTime(TZ_SEC, DST_SEC, timeServer1, timeServer2, timeServer3 );
  Serial.println( "Time Services setup");
      
  //Read internal state, apply defaults if we can't find user-set values in Eeprom.
  EEPROM.begin(800);
  readFromEeprom();    
  delay(5000);
    
  //Setup WiFi
  Serial.printf( "Entering Wifi setup for host %s\n", myHostname );
  setupWifi();
    
  //Setup I2C
#if defined _ESP8266_01_
  //Pins mode and direction setup for i2c on ESP8266-01
  pinMode(0, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
    
  //I2C setup SDA pin 0, SCL pin 2
  //Normally Wire.begin(0, 2);
  Wire.begin( 2,0 /*0, 2*/);  
  Serial.println("Configured pins for ESP8266-01");
#else //__ESP8266_12_
  //Pins mode and direction setup for i2c on ESP8266-12
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  //I2C setup SDA GPIO 5, SCL GPIO4 4
  Wire.begin(4, 5);
  
  //setup pins 13, 14, 15 for encoder (only need two + home if available.)
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  Serial.println("Configured pins for ESP8266-12");
#endif
  Wire.setClock(100000 );//100KHz target rate is a bit hopeful for this device 
////////////////////////////////////////////////////////////////////////////////////////

  outbuf = scanI2CBus();
  debugD( "I2CScan: %s", outbuf.c_str() );

////////////////////////////////////////////////////////////////////////////////////////
  //Open a connection to MQTT
  DEBUGS1("Configuring MQTT connection to :");DEBUGSL1( MQTTServerName );
  client.setServer( MQTTServerName, 1883 );
  Serial.printf(" MQTT settings id: %s user: %s pwd: %s\n", thisID, pubsubUserID, pubsubUserPwd );
  client.connect( thisID, pubsubUserID, pubsubUserPwd ); 
  //Create a timer-based callback that causes this device to read the local i2C bus devices for data to publish.
  client.setCallback( callback );
  client.subscribe( inTopic );
  publishHealth();
  client.loop();
  DEBUGSL1("Configured MQTT connection");
   
  //Setup the sensors
  motorPresent = myMotor.check();
  if ( motorPresent )
  {
    myMotor.init();
    myMotor.setSpeedDirection( MOTOR_SPEED_OFF, MOTOR_DIRN_CW );  
    motorSpeed = MOTOR_SPEED_OFF;
    motorDirection = MOTOR_DIRN_CW;
    myMotor.getSpeedDirection();
    Serial.printf("motor initialised - speed: %u, direction: %u", myMotor.getSpeed(), myMotor.getDirection() );
  }
  else
    Serial.println("No motor found on i2c bus");

  //Setup i2c to control LCD display
  DEBUGSL1("Starting to configure LCD connection");
  lcdPresent = ( myLCD.checkLCD() == 0 )? true: false;
  if( lcdPresent )
  { 
    myLCD.clearScreen();
    myLCD.setCursor( 1, 1, I2CLCD::CURSOR_UNDERLINE );
    myLCD.setBacklight( true ); 
    myLCD.writeLCD( 4, 1 , "ASCOMDome ready" );
    DEBUGSL1("LCD found and configured");
  }
  else
  {
    Serial.printf("LCD not found to configure.\n");  
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
  server.on("/api/v1/dome/0/altitude",                   HTTP_GET, handleAltitudeGet ); //tested - 0
  server.on("/api/v1/dome/0/athome",                     HTTP_GET, handleAtHomeGet );   //tested - returns false ?
  server.on("/api/v1/dome/0/atpark",                     HTTP_GET, handleAtParkGet );   //tested - returns false ?
  server.on("/api/v1/dome/0/azimuth",                    HTTP_GET, handleAzimuthGet);
  server.on("/api/v1/dome/0/canfindhome",                HTTP_GET, handleCanFindHomeGet);
  server.on("/api/v1/dome/0/canpark",                    HTTP_GET, handleCanParkGet);
  server.on("/api/v1/dome/0/cansetaltitude",             HTTP_GET, handleCanSetAltitudeGet);
  server.on("/api/v1/dome/0/cansetazimuth",              HTTP_GET, handleCanSetAzimuthGet);
  server.on("/api/v1/dome/0/cansetpark",                 HTTP_GET, handleCanSetParkGet);
  server.on("/api/v1/dome/0/cansetshutter",             HTTP_GET, handleCanSetShutterGet);
  server.on("/api/v1/dome/0/canslave",                  HTTP_GET, handleCanSlaveGet);
  server.on("/api/v1/dome/0/cansyncazimuth",            HTTP_GET, handleCanSyncAzimuthGet);
  server.on("/api/v1/dome/0/shutterstatus",             HTTP_GET, handleShutterStatusGet);
  server.on("/api/v1/dome/0/slaved",                    HTTP_GET,handleSlavedGet);
  server.on("/api/v1/dome/0/slaved",                    HTTP_PUT,handleSlavedPut);
  server.on("/api/v1/dome/0/slewing",                   HTTP_GET,handleSlewingGet);
  server.on("/api/v1/dome/0/abortslew",                 HTTP_PUT,handleAbortSlewPut);
  server.on("/api/v1/dome/0/closeshutter",              HTTP_PUT,handleCloseShutterPut);
  server.on("/api/v1/dome/0/findhome",                  HTTP_PUT,handleFindHomePut);
  server.on("/api/v1/dome/0/openshutter",               HTTP_PUT,handleOpenShutterPut);
  server.on("/api/v1/dome/0/park",                      HTTP_PUT,handleParkPut);
  server.on("/api/v1/dome/0/setpark",                   HTTP_PUT,handleSetParkPut);
  server.on("/api/v1/dome/0/slewtoaltitude",            HTTP_PUT,handleSlewToAltitudePut);
  server.on("/api/v1/dome/0/slewtoazimuth",             HTTP_PUT,handleSlewToAzimuthPut);
  server.on("/api/v1/dome/0/synctoazimuth",             HTTP_PUT,handleSyncToAzimuthPut);
 
//Common ASCOM function handlers
  server.on("/api/v1/dome/0/action",        HTTP_PUT, handleAction );              //Tested: Not implemented
  server.on("/api/v1/dome/0/commandblind",  HTTP_PUT, handleCommandBlind );        //These two have different responses
  server.on("/api/v1/dome/0/commandbool",   HTTP_PUT, handleCommandBool );         //These two have different responses
  server.on("/api/v1/dome/0/commandstring", HTTP_GET, handleCommandString );       //tested
  server.on("/api/v1/dome/0/connected",               handleConnected );           //tested  
  server.on("/api/v1/dome/0/description",   HTTP_GET, handleDescriptionGet );      //tested
  server.on("/api/v1/dome/0/driverinfo",    HTTP_GET, handleDriverInfoGet );       //freezes/times out
  server.on("/api/v1/dome/0/driverversion", HTTP_GET, handleDriverVersionGet );    //tested
  server.on("/api/v1/dome/0/interfaceversion", HTTP_GET, handleInterfaceVersionGet );    //tested
  server.on("/api/v1/dome/0/name",          HTTP_GET, handleNameGet ); //tested    //tested - doesnt return hostname
  server.on("/api/v1/dome/0/actions",       HTTP_GET, handleSupportedActionsGet ); //tested
  
  //Custom and setup handlers used by the custom setup form 
  //ASCOM ALPACA doesn't support any way to tell the REST device some of its basic setup constants so you have to 
  //do something like this. Or a serial interface .. Its a legacy of expecting a windows exe driver thing.
  server.on("/setup",            HTTP_GET, handleSetup);
  server.on("/api/v1/dome/0/setup",HTTP_GET, handleSetup);
  
  //HTML forms don't support PUT -  they typically transform them to use GET instead.
  server.on("/Hostname",    HTTP_GET, handleHostnamePut );
  server.on("/ShutterName", HTTP_GET, handleShutterNamePut );
  server.on("/SensorName",  HTTP_GET, handleSensorNamePut );
  server.on("/Park",        HTTP_GET, handleParkPositionPut );
  server.on("/Home",        HTTP_GET, handleHomePositionPut );
  server.on("/Goto",        HTTP_GET, handleDomeGoto );
  server.on("/Sync",        HTTP_GET, handleSyncOffsetPut );
  server.on("/restart", handlerRestart );
  server.on("/status", handlerStatus );
  server.on("/", handlerStatus);
  server.onNotFound(handlerNotFound);
  Serial.println( "Web handlers registered" );
  
  //setup interrupt-based 'soft' alarm handler for dome state update and async commands
  ets_timer_setfn( &fineTimer,    onFineTimer,    NULL ); 
  ets_timer_setfn( &coarseTimer,  onCoarseTimer,  NULL ); 
  ets_timer_setfn( &timeoutTimer, onTimeoutTimer, NULL ); 
  
  domeCmdList    = new LinkedList <cmdItem_t*>();
  shutterCmdList = new LinkedList <cmdItem_t*>();
  cmdStatusList  = new LinkedList <cmdItem_t*>(); // use to track async completion state. 
  Serial.println( "LinkedList setup complete");
  
  //Start web server
  updater.setup( &server );
  server.begin();
  Serial.println( "webserver setup complete");
   
  //Get startup values
  domeStatus = DOME_IDLE;
  domeTargetStatus = domeStatus;
  
  shutterStatus = getShutterStatus ( shutterHostname );
  shutterTargetStatus = shutterStatus;
  
  bearing = getBearing( sensorHostname );
  currentAzimuth = getAzimuth( bearing);
  
  //Are we close to locations of interest ?
  atHome = ( abs( currentAzimuth - homePosition ) < acceptableAzimuthError );
  atPark = ( abs( currentAzimuth - parkPosition ) < acceptableAzimuthError );
    
  //Start timers last
  ets_timer_arm_new( &coarseTimer, 2500,     1/*repeat*/, 1);//millis
  ets_timer_arm_new( &fineTimer,   1000,      1/*repeat*/, 1);//millis
  //ets_timer_arm_new( &timeoutTimer, 2500, 0/*one-shot*/, 1);

  //Show welcome message
  Debug.setSerialEnabled(true);
  debugW( "setup complete");
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
   ;;//fn moved to Shutter controller  - delete later if not used. 
   timeoutFlag = true;
}

void loop()
{
	String outbuf;
  String LCDOutput = "";
  
  //Operate and Clear down flags
  if( fineTimerFlag )
  {
    bearing = getBearing( sensorHostname );
    currentAzimuth = getAzimuth( bearing);
    debugD( "Loop: Bearing %03.2f, offset: %f, adjusted: %f\n", bearing, azimuthSyncOffset, currentAzimuth );
    fineTimerFlag = false;
  }
  
  if ( coarseTimerFlag )
  {
    //Update domeStatus    
    //getShutterStatus( shutterHostname );
    //debugI( "Loop: Dome %i Shutter %i\n", domeStatus, shutterStatus );

    // Main code here, to run repeatedly:
    //Handle state changes
    //in dome  
   
    switch ( domeStatus )
    {
      case DOME_IDLE:    onDomeIdle();
                         break;
      case DOME_SLEWING: onDomeSlew(); 
                         break;   
      case DOME_ABORT:   onDomeAbort();     
                         break;
      default:
        break;
    }
  
    //in shutter
    switch( shutterStatus )
    {
      case SHUTTER_CLOSED:
      case SHUTTER_OPEN:   onShutterIdle();
                           break;
      case SHUTTER_OPENING:
      case SHUTTER_CLOSING:onShutterSlew(); 
                           break;
      default:
           break;
    }
 
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
      callbackFlag = false;
    }
  }
  else
  {
    //reconnect();
    reconnectNB();
    client.subscribe(inTopic);
  }

  //If there are any web client connections - handle them.
  server.handleClient();

  // Remote debug over WiFi
  Debug.handle();
  // Or
  //debugHandle(); // Equal to SerialDebug  
}
   
  void coarseTimerHandler(void)
  {
    coarseTimerFlag = true;
  }
  
  void fineTimerHandler(void)
  {
    fineTimerFlag = true;
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

/*
 */
 void publishHealth( void )
 {
  String outTopic;
  String output;
  String timestamp;
  
  //publish to our device topic(s)
  DynamicJsonBuffer jsonBuffer(256);  
  JsonObject& root = jsonBuffer.createObject();

  getTimeAsString2( timestamp );
  root["time"] = timestamp;

  // Once connected, publish an announcement...
  root["hostname"] = myHostname;
  if( connected != NOT_CONNECTED ) 
    root["message"] = "Dome connected & operating";
  else
    root["message"] = "Dome ready for connection";
  root.printTo( output );
  
  outTopic = outHealthTopic;
  outTopic.concat( myHostname );
  
  client.publish( outTopic.c_str(), output.c_str() );  
  debugI( "Topic published: %s ", output.c_str() ); 
  }
