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
   EEprom partially in place - needs testing
   Add MQTT callbacks for health - done - need to measure and add publish for Dome voltage
   
   Testing.
   Curl resting of command handlers for ALPACA appears good. 
   Need to test with ALPACA
   
   Operating. 
   Check power demand and manage
   
   To do
   1, Add MQTT for Bz, temp and dome orientation - not doing.
   2, dome setup to support calc of auto-track pointing - maybe
   3, Add list handler - done
   4, Update smd list to handle variable settings and fixup responses for async responses. Report to ascom community
   Test handler functions 
   5, Add url to query for staus of async operations. Consider whether user just needs to call for status again. 

To test:
 Download curl for your operating system. Linux variants and Windows powershell should already have it.
 e.g. curl -v -X PUT -d 'homePosition=270' http://EspDom01/Dome/0/API/v1/FilterCount
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
  Humidity sensor - 1-wire interface
  Battery voltage Sensor- i2c
  Temp Sensor = i2c
  LCD Display - i2c
*/

/////////////////////////////////////////////////////////////////////////////////

#include "ESP8266_AscomDome.h"   //App variables - pulls in the other include files - its all in there.
#include "ASCOMAPIDome_rest.h"
#include "ASCOMAPICommon_rest.h"
#include "JSONHelperFunctions.h"
#include "ASCOM_Domehandler.h"
#include "ASCOM_DomeSetup.h"
#include "Skybadger_common_funcs.h"

void setupWifi(void)
{
  //Setup Wifi
  int zz = 0;
  WiFi.hostname(myHostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin( ssid1, password1 );
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);//This delay is essentially for the DHCP response. Shouldn't be required for static config.
    Serial.print(".");

    if ( zz++ >= 400) 
    {
      device.restart();
    }    
  }

  Serial.println("WiFi connected");
  Serial.printf("SSID: %s, Signal strength %i dBm \n\r", WiFi.SSID().c_str(), WiFi.RSSI() );
  Serial.printf("Hostname: %s\n\r",       WiFi.hostname().c_str() );
  Serial.printf("IP address: %s\n\r",     WiFi.localIP().toString().c_str() );
  Serial.printf("DNS address 0: %s\n\r",  WiFi.dnsIP(0).toString().c_str() );
  Serial.printf("DNS address 1: %s\n\r",  WiFi.dnsIP(1).toString().c_str() );
  delay(5000);

  //Setup sleep parameters
  //WiFi.mode(WIFI_NONE_SLEEP);
  wifi_set_sleep_type(NONE_SLEEP_T);
}

void setup()
{
  // put your setup code here, to run once:
  String outbuf;
  Serial.begin( 115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println(F("ESP starting."));
  gdbstub_init();

  //Start NTP client
  configTime(TZ_SEC, DST_SEC, timeServer1, timeServer2, timerServer3 );

  //Setup defaults first - via EEprom.
  //Read internal state, apply defaults if we can't find user-set values in Eeprom.
  EEPROM.begin(512);
  setupDefaults();
  setupFromEeprom();
    
  //Setup I2C
#if defined _ESP8266_01_
  //Pins mode and direction setup for i2c on ESP8266-01
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, INPUT_PULLUP);
    
  //I2C setup SDA pin 0, SCL pin 2
  Wire.begin(0, 2);
  Serial.println("Configured pins for ESP8266-01");
#else //__ESP8266_12_
  //Pins mode and direction setup for i2c on ESP8266-12
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(3, INPUT_PULLUP);
  //setup pins 13, 14, 15 for encoder 
  //setup pin 11 to be analogue in for battery voltage sensing.   
  //pinMode( 11, INPUT );
  //I2C setup SDA pin 0, SCL pin 2
  Wire.begin(4, 5);
  Serial.println("Configured pins for ESP8266-12");
#endif
  Wire.setClock(100000 );//100KHz target rate

  setupWifi();
  
  //Open a connection to MQTT
  client.setServer( MQTTHostname, 1883 );
  client.connect( thisID, pubsubUserID, pubsubUserPwd ); 
  //Create a timer-based callback that causes this device to read the local i2C bus devices for data to publish.
  client.setCallback( callback );
  client.subscribe( inTopic );
  client.loop();
  Serial.println("Configured MQTT connection");
  
  //Setup i2C to drive motor controller
  myMotor.check();
  myMotor.init();

  //Setup i2c to control LCD display
  if ( LCDPresent = myLCD.checkLCD() )
    Serial.printf("unable to access LCD\n");  
  else
  { 
    myLCD.clearScreen();
    myLCD.setCursor( 1, 1, I2CLCD::CURSOR_UNDERLINE );
    myLCD.setBacklight( true ); 
    myLCD.writeLCD( 1, 1 , "ASCOMDome ready" );
  }
  
  //Use one of these to track the dome position
#ifdef _USE_ENCODER_FOR_DOME_ROTATION
  setupEncoder();
#else
  //Magnetometer setup 
  if ( setupCompass( sensorHostname ) )
    bearing = getBearing();
  if ( bearing == 0.0F)
    Serial.println( "Dome rotation sensor suspect reading (0.0F) " );
  currentAzimuth = getAzimuth( bearing );
#endif

  Serial.println( "Dome rotation sensor connected" );
  
  //Setup domeHW state 
  //remote device on the rotating dome part
  shutterStatus = getShutterStatus( shutterHostname );
  
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
  server.on("/api/v1/dome/0/Action",        HTTP_PUT, handleAction );              //Tested: Not implemented
  server.on("/api/v1/dome/0/CommandBlind",  HTTP_PUT, handleCommandBlind );        //These two have fdiffernt responses
  server.on("/api/v1/dome/0/CommandBool",   HTTP_PUT, handleCommandBool );         //These two have fdiffernt responses
  server.on("/api/v1/dome/0/CommandString", HTTP_GET, handleCommandString );       //tested
  server.on("/api/v1/dome/0/Connected",               handleConnected );           //tested  
  server.on("/api/v1/dome/0/Description",   HTTP_GET, handleDescriptionGet );      //tested
  server.on("/api/v1/dome/0/DriverInfo",    HTTP_GET, handleDriverInfoGet );       //freezes/times out
  server.on("/api/v1/dome/0/DriverVersion", HTTP_GET, handleDriverVersionGet );    //tested
  server.on("/api/v1/dome/0/Name",          HTTP_GET, handleNameGet ); //tested    //tested - doesnt return hostname
  server.on("/api/v1/dome/0/Actions",       HTTP_GET, handleSupportedActionsGet ); //tested
  
  //Custom and setup handlers used by the custom setup form 
  //ASCOM ALPACA doesn't support any way to tell the REST device some of its basic setup constants so you have to 
  //do something like this. Or a serial interface .. Its a legacy of expecting a windows exe driver thing.
  server.on("/",            HTTP_GET, handleSetup);
  //HTML forms don't support PUT - use GET instead.
  server.on("/Sync",        HTTP_GET, handleSyncOffsetPut );
  //  server.on("/Dome/0/SetHome", HTTP_GET, handleFilterNamesPut );
  server.on("/Hostname",    HTTP_GET, handleHostnamePut );
  server.on("/DomeName",    HTTP_GET, handleNamePut );
  //server.on("/SetPark",    HTTP_GET, handleParkPut );
  //server.on("/SetHome",    HTTP_GET, handleHomePut );

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
   
  //Start timers last
  ets_timer_arm_new( &coarseTimer, 1000,     1/*repeat*/, 1);//millis
  ets_timer_arm_new( &fineTimer,   250,      1/*repeat*/, 1);//millis
  //ets_timer_arm_new( &timeoutTimer, 2500, 0/*one-shot*/, 1);

  //Show welcome message
  Serial.println( "setup complete");
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
  //Used for MQTT reconnects timing currently
}

void loop()
{
	String outbuf;
  
  //Grab a copy in case it changes while we are running.
  bool coarseTimer = coarseTimerFlag;
  bool fineTimer = fineTimerFlag;
    
  // Main code here, to run repeatedly:
  if( WiFi.status() != WL_CONNECTED)
  {
      device.restart();
  }
 
  //Handle state changes
  //in dome  
  switch ( domeStatus )
  {
    case DOME_IDLE:
      onDomeIdle();
      break;
    case DOME_SLEWING:
      onDomeSlew(); 
      break;   
    case DOME_ABORT:
      onDomeAbort();     
      break;
    default:
      break;
  }

  //in shutter
  switch( shutterStatus )
  {
    case SHUTTER_CLOSED:
    case SHUTTER_OPEN:
         onShutterIdle();
         break;
    case SHUTTER_OPENING:
    case SHUTTER_CLOSING:    
         onShutterSlew(); 
         break;
    default:
         break;
  }

  //Clear down flags
  if( fineTimer )
  {
    bearing = getBearing();
    Serial.printf( "Loop: Bearing %f\n", bearing );
    currentAzimuth = getAzimuth( bearing);
    Serial.printf( "Loop: Dome %i Shutter %i\n", domeStatus, shutterStatus );
    fineTimerFlag = false;
  }
  
  if ( coarseTimer )
  {
    getTimeAsString( outbuf );
    //Clock tick
    myLCD.writeLCD( 1,1, outbuf );
    coarseTimerFlag = false;
  }
 
  if ( client.connected() )
  {
     if ( callbackFlag == true )
     {
       //publish results
         publishStuff();
         callbackFlag = false;
     }
     client.loop();
  }
  else
  {
     reconnectNB();
  }
  
  //If there are any web client connections - handle them.
  server.handleClient();
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
      pCmd->value = value;
      pCmd->clientId = clientId;
      pCmd->transId = transId; 
      domeCmdList->add( pCmd );
      return pCmd;
  }

  cmdItem_t* addShutterCmd( uint32_t clientId, uint32_t transId, enum shutterCmd newCmd, int value )
  {
      //Create new command
      cmdItem_t* pCmd = (cmdItem_t*) calloc( sizeof( cmdItem_t ), 1); 
      //Add to list 
      pCmd->cmd = (int) newCmd;
      pCmd->value = value;
      pCmd->clientId = clientId;
      pCmd->transId = transId; 
      shutterCmdList->add( pCmd );
      return pCmd;
  }

  //Wrapper for web server args handling for URL query arguments that differ by case. The ALPACA API allows this!
  bool hasArgIC( String& check, ESP8266WebServer& ref, bool caseSensitive )
  {
    String matchName;
    if ( caseSensitive )
    {
      int max = server.args();
      for ( int i = 0; i < max; i++ )
      {
        if( server.argName(i).equalsIgnoreCase( check ) )  
        {
          check = server.arg(i);          
          return true;
        }
      }
      return false;
    }
    else
      return ( server.hasArg( check ) );
  }

/* MQTT callback for subscription and topic.
 * Only respond to valid states ""
 * Publish under ~/skybadger/sensors/<sensor type>/<host>
 * Note that messages have an maximum length limit of 18 bytes - set in the MQTT header file. 
 */
void callback(char* topic, byte* payload, unsigned int length) 
{  
  String sTopic = String( topic );
  String sPayload = String( (char*) payload );
  JsonObject = //'
  //Need to subscribe to some postings to handle intelligently.
  //Listen for safety postings. 
  if ( sTopic.indexOf( "/function/safety/" ) )
  {
      
  }
  else if (sTopic.indexOf( "/function/observing_conditions/" ) )
  {
  }
      
   
  
  //listen for Observing conditions postings. 
  
  
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
  
  getTimeAsString2( timestamp);

  // Once connected, publish an announcement...
  root["hostname"] = myHostname;
  root["time"] = timestamp;
  if( connected ) 
    root["message"] = "Dome connected & operating";
  else
    root["message"] = "Dome ready for connection";

  outTopic = outHealthTopic;
  outTopic.concat( myHostname );
  client.publish( outTopic.c_str(), output.c_str() );  
  Serial.print( outTopic.c_str() );Serial.print( " published: " ); Serial.println( output.c_str() );
 }