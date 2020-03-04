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
   EEprom partially in place - needs ping -t testing
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
   6, Fix EEPROM handling - done. 

zero-justify seconds and minutes in sprintf time string.
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
#include "ASCOM_DomeEeprom.h"
#include "Skybadger_common_funcs.h"

void setupWifi(void)
{
  //Setup Wifi
  int zz = 00;
  WiFi.hostname( myHostname );
  WiFi.mode(WIFI_STA);
  WiFi.begin( ssid1, password1 );
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);//This delay is essentially for the DHCP response. Shouldn't be required for static config.
    Serial.print(".");
    if ( zz++ > 200 )
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
  Serial.begin( 115200 );
  //Serial.begin( 115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println(F("ESP starting."));
  //gdbstub_init();
  delay(2000); 

  //Start time
  //Start NTP client - even before wifi.
  configTime(TZ_SEC, DST_SEC, timeServer1, timeServer2, timeServer3 );
  Serial.println( "Time Services setup");
      
  //Read internal state, apply defaults if we can't find user-set values in Eeprom.
  EEPROM.begin(512);
  readFromEeprom();    
  
  //Setup defaults from EEPROM
  delay(5000);
  Serial.printf( "Entering Wifi setup for host %s\n", myHostname );
  setupWifi();
    
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
  Wire.setClock(50000 );//100KHz target rate

  //Open a connection to MQTT
  DEBUGS1("Starting to configure MQTT connection to :");DEBUGSL1( MQTTServerName );
  client.setServer( MQTTServerName, 1883 );
  Serial.printf(" MQTT settings id: %s user: %s pwd: %s\n", thisID, pubsubUserID, pubsubUserPwd );
  client.connect( thisID, pubsubUserID, pubsubUserPwd ); 
  //Create a timer-based callback that causes this device to read the local i2C bus devices for data to publish.
  client.setCallback( callback );
  client.subscribe( inTopic );
  publishHealth();
  client.loop();
  DEBUGSL1("Configured MQTT connection");
  
  //Setup i2C to drive motor controller
  DEBUGSL1("Starting to configure motor connection");
  myMotor.check();
  myMotor.init();
  DEBUGSL1("Configured motor connection");

  //Setup i2c to control LCD display
  DEBUGSL1("Starting to configure LCD connection");
  LCDPresent = myLCD.checkLCD();
  if ( LCDPresent )
    Serial.printf("unable to access LCD\n");  
  else
  { 
    myLCD.clearScreen();
    myLCD.setCursor( 1, 1, I2CLCD::CURSOR_UNDERLINE );
    myLCD.setBacklight( true ); 
    myLCD.writeLCD( 1, 1 , "ASCOMDome ready" );
  }
  DEBUGSL1("Configured LCD connection");
  
  //Use one of these to track the dome position
#ifdef _USE_ENCODER_FOR_DOME_ROTATION
  //setupEncoder();
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
  server.on("/api/v1/dome/0/commandblind",  HTTP_PUT, handleCommandBlind );        //These two have fdiffernt responses
  server.on("/api/v1/dome/0/commandbool",   HTTP_PUT, handleCommandBool );         //These two have fdiffernt responses
  server.on("/api/v1/dome/0/commandstring", HTTP_GET, handleCommandString );       //tested
  server.on("/api/v1/dome/0/connected",               handleConnected );           //tested  
  server.on("/api/v1/dome/0/description",   HTTP_GET, handleDescriptionGet );      //tested
  server.on("/api/v1/dome/0/driverinfo",    HTTP_GET, handleDriverInfoGet );       //freezes/times out
  server.on("/api/v1/dome/0/driverversion", HTTP_GET, handleDriverVersionGet );    //tested
  server.on("/api/v1/dome/0/name",          HTTP_GET, handleNameGet ); //tested    //tested - doesnt return hostname
  server.on("/api/v1/dome/0/actions",       HTTP_GET, handleSupportedActionsGet ); //tested
  
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
   
  //Get values
  domeStatus = getShutterStatus ( shutterHostname );
  bearing = getBearing( sensorHostname );
  currentAzimuth = getAzimuth( bearing);
  atHome = (currentAzimuth <= (homePosition + acceptableAzimuthError/2) && currentAzimuth >= (homePosition - acceptableAzimuthError/2) );
  atPark = (currentAzimuth <= (parkPosition + acceptableAzimuthError/2) && currentAzimuth >= (parkPosition - acceptableAzimuthError/2) );
    
  //Start timers last
  ets_timer_arm_new( &coarseTimer, 1000,     1/*repeat*/, 1);//millis
  ets_timer_arm_new( &fineTimer,   500,      1/*repeat*/, 1);//millis
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
   ;;//fn moved to Shutter controller  - delete later if not used. 
   timeoutTimerFlag = true;
}

void loop()
{
	String outbuf;
  String LCDOutput = "";
  
  if( WiFi.status() != WL_CONNECTED)
  {
      device.restart();
  }

  //Operate and Clear down flags
  if( fineTimerFlag )
  {
    bearing = getBearing( sensorHostname );
    //Serial.printf( "Loop: Bearing %3.2f\n", bearing );
    currentAzimuth = getAzimuth( bearing);
    fineTimerFlag = false;
  }
  
  if ( coarseTimerFlag )
  {
    //Update domeStatus    
    getShutterStatus( shutterHostname );
    Serial.printf( "Loop: Dome %i Shutter %i\n", domeStatus, shutterStatus );

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
    coarseTimerFlag = false;
  }
 
  if( client.connected() )
  {
    if (callbackFlag )
    {
      //publish any results ?
      callbackFlag = false;
      publishHealth();
    }
    client.loop(); 
  }    //Service MQTT keep-alives
  else
  {
    reconnectNB();
    client.subscribe( inTopic );
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
  /*
   * Pass in the string you want to check exists. 
   * modifies the string to the case-insensitive version found. 
   */
  bool hasArgIC( String& check, ESP8266WebServer& ref, bool caseSensitive )
  {
    String matchName;
    if ( caseSensitive )
    {
      int max = ref.args();
      for ( int i = 0; i < max; i++ )
      {
        if( ref.argName(i).equalsIgnoreCase( check ) )  
        {
          check = ref.arg(i);          
          return true;
        }
      }
      return false;
    }
    else
      return ( ref.hasArg( check ) );
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

  getTimeAsString( timestamp );
  root["time"] = timestamp;

  // Once connected, publish an announcement...
  root["hostname"] = myHostname;
  if( connected ) 
    root["message"] = "Dome connected & operating";
  else
    root["message"] = "Dome ready for connection";
  root.printTo( output );
  outTopic = outHealthTopic;
  outTopic.concat( myHostname );
  client.publish( outTopic.c_str(), output.c_str() );  
  //Serial.print( outTopic );Serial.print( " published: " ); Serial.println( output );
 }
