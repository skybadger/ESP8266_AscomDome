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
   12, Provide option to not use connected client id for use in brownouts - done
   13, Adjust slow rate to almost max rate.
   14, add call to set slow rates and max rates via API.
   15, Add action to backup crawling over lockups and attempt at full speed - done

  To test:
  Download curl for your operating system. Linux variants and Windows powershell should already have it.
  e.g. curl -v -X PUT -d "homePosition=270&ClientID=99&ClientTransactionID=99" http://EspDom01/API/v1/Dome/1/setHome
  Use the ALPACA REST API docs at <> to validate the dome behaviour. At some point there will be a script

  External Dependencies:
  Linked List https://github.com/ivanseidel/LinkedList
  ArduinoJSON library 7 - moved from 5.13 due to needing to use JsonVariuant.set()
  pubsub library for MQTT
  ALPACA for ASCOM 6.5+
  Expressif ESP8266 board library for arduino - configured for v2.7
  EEPROMAnything library - added string handler since it doesn't do char* strings well - moving to FRAM library when ready .
  GDB - not really used
  RemoteDebug - used more and more.
  +
  ESP8266-12 Huzzah
  /INT - GPIO3
  [I2C SDA - GPIO4  SCL - GPIO5]
  [SPI SCK = GPIO #14 (default)  SPI MOSI = GPIO #13 (default)  SPI MISO = GPIO #12 (default)]
  ENC_A GPIO 12
  ENC_B GPIO 13
  ENC_H - nused

  Motor controller - i2c
  LCD Display - i2c

  Change Log
  21/05/2021 Added last will and testament to MQTT and use of dirty sessions rather than fresh ones.
  31/05/2021 Updated the setup handler page presentation to chunk the content  - needs testing. Splits down page into a series of content transfers rather than
  tries to send a whole page.
  01/06/2021 Added last reset reason and binary version info (date) to first publishHealth response
  01/06/2021 Updated handleSlavedGet to return value of slaved setting. PUT command always returns NotImplementedException.
  01/06/2021 handleAbortSlewPut updated to remove InvalidOperation exception when Abort is called and system is not slewing
  01/06/2021 Updated slewToAltitudePut to respect canSetAltitude for Conform compliance.
   /       Also added exception handling for out of range values.
  01/06/2021 Updated handleSlewToAzimuthPut to enforce value limits by invalidValue Exception
           Causes actual azimuth to be updated asynchronously - this may yet be too slow for Conform.
  01/06/2021 Amended OnSlew to manage the zero hunting issue where the fast speed is selected for a small reversal slew.
  29/09/2021 Amended onSlew to manage detection of atHome and atPark and speed determination when near to zero/360
  02/10/2021 Fixed a bug where a call to shutter may fail and return error rather than keeping last state. Now tracks response state. Error state upsets Voyager.
  26/10/2021 Changed free(pCmd) to freeCmd( pCmd) since it appears to be leaking memory by not releasing Strings properly. chnaged Strings to char* & free xplicitly.
  26/10/2021 //TODO do similar while loop to detect encoder readiness. - done
  16/11/2021 Added lots of debugV statements to get to bottom of memory leak. Nothing seen really in terms of trends of memory usage
  10/09/2024 Added domeLock handling for when dome gets stuck on certain points. 'Detect, abort slew, back off and run at the sticky point' implemented.
  21/09/2025 Fixed Alpaca management code - versions are now correct data format, URLs now have correct handlers, UDP service now runs and responds.
  Used Confirm Universal to validate. Still returns some non-compliant IDs when conform provides them , however meets normal conditions fine.
  Will NOT try to fix since that means fixing web server handling service.
  30/05/2026 updated ASCOMAPICommonRest functions to prevent bad parameteres and badly-cased arguments through, to get rid of a lot of CONFORM noise.
  Added use of isValidRequest to check parameter casing and values in ranges for types.
  01/08/2026 Migrated handlers to use JsonVariant and JsonDoc as per ArduinoJson v7 to enforce checks for non-compliant ids to get rid aof a lot of the compliance check warnings.
  Added handlers for more async as per interface v3 but not fully integrated.
  Added status handler as per interface v3.

  Bugs
  Theres a bug in here somewhere which causes a reboot and a client clost connection occasionally enough to be a problem, causing Voyager to lose 'connected' state
  remedy is to re-issue call to connect using curl by hand using Voyager's last clientID. Or fix at source.
  checkout : https://arduino-esp8266.readthedocs.io/en/latest/PROGMEM.html#declare-a-flash-string-within-code-block for moving strings into PROGMEM to reduce the memory footprint.
  F("myString") stores the string into PROGMEM and makes accessible to string functions that can manage access to that memory
  Serial.println( F("myString") ) is ok
  Serial.printf_P( PSTR("myString") ) is needed as too the progmem functions for more extensive string loading into heap memory from flash.
  22/11/2021 Memleak bug apparently fixed by moving hClient.end() to end of statements in restQuery - it was occassionally being missed caused by a certain combination of errors.
  leads to memory in rest queries not being released and memory decreasing by about 1300 bytes per miss.
  10/09/2024 Reboot bug is due to not being able to reach one remote client or taking a long time while another remote call i smade and the ensuing buffer allocations on teh heap cause it to run out of memory.
  Solution to this is:
  - reduce timeouts on remote calls to < 300 ms
  - increase the heap size. So an ESP8266-12 should be fine with a 4MB memory. Just needs to be configured to be able to use it.
  In spite of all of this, over time, sometimes when a response is not rapid enough the device will run out of local heap mememory since there will be calls waiting that have data bufffers waiting to be serviced.
  Need to work out how to increase local heap allocation.
  Actually, after some conversations with chatGPt, it might be that there is a clash in use of the WiFi port and that causes the reboot. Try without MQTT in nbreconnect mode and see.
*/

/////////////////////////////////////////////////////////////////////////////////

//Internal variables
#include "ESP8266_AscomDome.h" //App variables - pulls in the other include files - its all in there.
#include <Skybadger_common_funcs.h>
#include "ASCOMAPICommon_rest.h"
#include "ASCOMAPIDome_rest.h"
#include "JSONHelperFunctions.h"
#include "AlpacaManagement.h"
#include "ASCOM_DomeCmds.h"
#include "ASCOM_Domehandler.h"
#include "ASCOM_DomeSetup.h"
#include "ASCOM_DomeEeprom.h"

void setup(void);
void setupWifi(void);
void publishFnStatus(void);
void publishHealth(void);

#if DOME_HEAP_TRACE
int domeHeapQueueSize() { return domeCmdList ? domeCmdList->size() : 0; }
int shutterHeapQueueSize() { return shutterCmdList ? shutterCmdList->size() : 0; }
#endif

void setup()
{
  // put your setup code here, to run once:
  String outbuf;
  String path;
  int response = 403;

  //Minimise serial to one pin only.
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println(F("ESP starting."));
  //gdbstub_init();

  //Give time to open a serial monitor for debugging
  delay(5000);

  //Start time
  configTime(TZ_SEC, DST_MN, timeServer1, timeServer2, timeServer3);
  Serial.println(F("Time Services setup"));

  //Read internal state, apply defaults if we can't find user-set values in Eeprom.
  EEPROM.begin(eepromSize);
  setupDefaults();
  readFromEeprom();

  //Setup WiFi
  Serial.printf_P(PSTR("Entering Wifi setup for host %s\n"), myHostname);
  setupWifi();

#if !defined _DISABLE_REMOTE_DEBUG
  //Debugging over telnet setup
  // Initialize the server (telnet or web socket) of RemoteDebug
  Debug.begin(WiFi.hostname().c_str(), Debug.ERROR);
  Debug.setSerialEnabled(true); //until set false
  // Options
  Debug.setResetCmdEnabled(true); // Enable the reset command
  // Debug.showProfiler(true); // To show profiler - time between messages of Debug
  //In practice still need to use serial commands until debugger is up and running..
  DEBUGSL1(F("Remote debugger enabled and operating"));
#endif

  //for use in debugging reset - may need to move
  Serial.printf_P(PSTR("Device reset reason: %s\n"), device.getResetReason().c_str());
  Serial.printf_P(PSTR("device reset info: %s\n"), device.getResetInfo().c_str());

  //Setup I2C
#if defined _ESP8266_01_
  //Pins mode and direction setup for i2c on ESP8266-01
  pinMode(0, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  //pinMode(3, INPUT_PULLUP);

  //I2C setup SDA pin 0, SCL pin 2
  //Normally Wire.begin(0, 2);
  Wire.begin(2, 0 /*0, 2*/);
  Serial.println(F("Configured pins for ESP8266-01\n"));
#elif defined _ESP8266_12_
  //Pins mode and direction setup for i2c on ESP8266-12
  pinMode(14, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  //I2C setup SDA GPIO 5, SCL GPIO 4 - emerges at pins D1 and D2 on nodeMCU ESP8266-12
  //I2C setup SDA GPIO 14, SCL GPIO 2 - emerges at pins D5 and D4 on nodeMCU ESP8266-12
  Wire.begin(14, 2);

  //setup pins 13, 15 for encoder (only need two + home if available.)
  //emerges at pins D7 and D8 on nodeMCU ESP8266-12
  pinMode(13, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);
  Serial.println(F("Configured pins for ESP8266-12\n"));
#elif defined _ESP32_XX_
  //TODO
  Serial.println(F("Configured pins for ESP32-XX\n"));
#endif
  Wire.setClock(100000); //100KHz target rate is a bit hopeful for this device

  ////////////////////////////////////////////////////////////////////////////////////////
  outbuf = scanI2CBus();
  debugD("I2CScan: %s", outbuf.c_str());
  Serial.printf_P(PSTR("I2CScan: %s\n"), outbuf.c_str());
  ////////////////////////////////////////////////////////////////////////////////////////

  //Open a connection to MQTT
#if !defined _DISABLE_MQTT
  //MQTTServerName is in EEProm, as too ThisID.
  Serial.printf_P(PSTR(("Configuring MQTT connection to :%s\n")), MQTTServerName);
  client.setServer(MQTTServerName, 1883);
  Serial.printf_P(PSTR(" MQTT settings id: %s user: %s pwd: %s\n"), thisID, pubsubUserID, pubsubUserPwd);

  //want to connect to a 'dirty' session not a clean one, that way we receive any outstanding messages waiting
  //Also setup our last will message
  String lastWillTopic = String(outHealthTopic);
  lastWillTopic.concat(myHostname);
  client.connect(thisID, pubsubUserID, pubsubUserPwd, lastWillTopic.c_str(), 1, true, "Offline", false);

  //Create a callback that causes this device to publish its health and sensor data over MQTT to the centre
  client.setCallback(callback);
  client.subscribe(inTopic);
  Serial.printf_P(PSTR("Configured MQTT subscription connection: %s\n"), inTopic);
#endif

  //Setup the sensors
  motorPresent = myMotor.check();
  if (motorPresent)
  {
    myMotor.init();
    myMotor.setSpeedDirection(MOTOR_SPEED_OFF, MOTOR_DIRN_CW);
    motorSpeed = MOTOR_SPEED_OFF;
    motorDirection = MOTOR_DIRN_CW;
    myMotor.getSpeedDirection();
    Serial.printf_P(PSTR("motor initialised - speed: %u, direction: %u\n"), myMotor.getSpeed(), myMotor.getDirection());
    debugD("motor initialised - speed: %u, direction: %u\n", myMotor.getSpeed(), myMotor.getDirection());
  }
  else
  {
    Serial.println(F("No motor found on i2c bus\n"));
    debugE("No motor found on i2c bus\n");
  }

  //The FRAM library begin() does not probe the device. Check for an ACK first;
  //this detects a device at the configured address without changing stored data.
  Wire.beginTransmission(FRAMControllerAddr);
  const uint8_t framStatus = Wire.endTransmission();
  framPresent = (framStatus == 0);
  if (framPresent)
  {
    myFRAM.begin();
    Serial.printf_P(PSTR("FRAM found on i2c bus at 0x%02X\n"), FRAMControllerAddr);
    debugD("FRAM found on i2c bus at 0x%02X\n", FRAMControllerAddr);
  }
  else
  {
    Serial.printf_P(PSTR("No FRAM found on i2c bus at 0x%02X - status: %u\n"), FRAMControllerAddr, framStatus);
    debugE("No FRAM found on i2c bus at 0x%02X - status: %u\n", FRAMControllerAddr, framStatus);
  }

  //Setup i2c to control LCD display
  debugI("Starting to configure LCD connection");
  lcdPresent = (myLCD.checkLCD() == 0) ? true : false;
  lcdPresent = false; //above check doesnt work - LCD currently not fitted.
  if (lcdPresent)
  {
    myLCD.clearScreen();
    myLCD.setCursor(1, 1, I2CLCD::CURSOR_UNDERLINE);
    myLCD.setBacklight(true);
    myLCD.writeLCD(4, 1, "ASCOMDome ready");
    debugI("LCD found and configured");
  }
  else
  {
    Serial.printf_P(PSTR("No LCD found on i2c bus.\n"));
    debugI("No LCD display found on i2c bus\n");
  }

  //Use one of these to track the dome position
#ifdef USE_LOCAL_ENCODER_FOR_DOME_ROTATION
  setupEncoder();
#elif defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  //Nothing to do .
#elif defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION
  //Nothing to do.
#endif

  //Register Web server handler functions
  //ASCOM dome-specific functions
  Serial.printf_P(PSTR("Registering web handlers.\n"));
  server.on(F("/api/v1/dome/1/altitude"), HTTP_GET, handleAltitudeGet); //tested - 0
  server.on(F("/api/v1/dome/1/athome"), HTTP_GET, handleAtHomeGet);     //tested - returns false ?
  server.on(F("/api/v1/dome/1/atpark"), HTTP_GET, handleAtParkGet);     //tested - returns false ?
  server.on(F("/api/v1/dome/1/azimuth"), HTTP_GET, handleAzimuthGet);
  server.on(F("/api/v1/dome/1/canfindhome"), HTTP_GET, handleCanFindHomeGet);
  server.on(F("/api/v1/dome/1/canpark"), HTTP_GET, handleCanParkGet);
  server.on(F("/api/v1/dome/1/cansetaltitude"), HTTP_GET, handleCanSetAltitudeGet);
  server.on(F("/api/v1/dome/1/cansetazimuth"), HTTP_GET, handleCanSetAzimuthGet);
  server.on(F("/api/v1/dome/1/cansetpark"), HTTP_GET, handleCanSetParkGet);
  server.on(F("/api/v1/dome/1/cansetshutter"), HTTP_GET, handleCanSetShutterGet);
  server.on(F("/api/v1/dome/1/canslave"), HTTP_GET, handleCanSlaveGet);
  server.on(F("/api/v1/dome/1/cansyncazimuth"), HTTP_GET, handleCanSyncAzimuthGet);
  server.on(F("/api/v1/dome/1/shutterstatus"), HTTP_GET, handleShutterStatusGet);
  server.on(F("/api/v1/dome/1/slaved"), HTTP_GET, handleSlavedGet);
  server.on(F("/api/v1/dome/1/slaved"), HTTP_PUT, handleSlavedPut);
  server.on(F("/api/v1/dome/1/slewing"), HTTP_GET, handleSlewingGet);
  server.on(F("/api/v1/dome/1/abortslew"), HTTP_PUT, handleAbortSlewPut);
  server.on(F("/api/v1/dome/1/closeshutter"), HTTP_PUT, handleCloseShutterPut);
  server.on(F("/api/v1/dome/1/findhome"), HTTP_PUT, handleFindHomePut);
  server.on(F("/api/v1/dome/1/openshutter"), HTTP_PUT, handleOpenShutterPut);
  server.on(F("/api/v1/dome/1/park"), HTTP_PUT, handleParkPut);
  server.on(F("/api/v1/dome/1/setpark"), HTTP_PUT, handleSetParkPut);
  server.on(F("/api/v1/dome/1/slewtoaltitude"), HTTP_PUT, handleSlewToAltitudePut);
  server.on(F("/api/v1/dome/1/slewtoazimuth"), HTTP_PUT, handleSlewToAzimuthPut);
  server.on(F("/api/v1/dome/1/synctoazimuth"), HTTP_PUT, handleSyncToAzimuthPut);
  server.on(F("/api/v1/dome/1/connect"), HTTP_PUT, handleConnectPut);
  server.on(F("/api/v1/dome/1/disconnect"), HTTP_PUT, handleDisconnectPut);
  server.on(F("/api/v1/dome/1/connecting"), HTTP_GET, handleConnectingGet);
  server.on(F("/api/v1/dome/1/devicestate"), HTTP_GET, handleDeviceStateGet);

  //Common ASCOM function handlers
  server.on(F("/api/v1/dome/1/action"), HTTP_PUT, handleAction);                        //Tested: Not implemented
  server.on(F("/api/v1/dome/1/commandblind"), HTTP_PUT, handleCommandBlind);            //These two have different responses
  server.on(F("/api/v1/dome/1/commandbool"), HTTP_PUT, handleCommandBool);              //These two have different responses
  server.on(F("/api/v1/dome/1/commandstring"), HTTP_PUT, handleCommandString);          //Alpaca command methods use PUT
  server.on(F("/api/v1/dome/1/connected"), HTTP_PUT, handleConnected);                  //tested
  server.on(F("/api/v1/dome/1/connected"), HTTP_GET, handleConnected);                  //tested
  server.on(F("/api/v1/dome/1/description"), HTTP_GET, handleDescriptionGet);           //tested
  server.on(F("/api/v1/dome/1/driverinfo"), HTTP_GET, handleDriverInfoGet);             //freezes/times out
  server.on(F("/api/v1/dome/1/driverversion"), HTTP_GET, handleDriverVersionGet);       //tested
  server.on(F("/api/v1/dome/1/interfaceversion"), HTTP_GET, handleInterfaceVersionGet); //tested
#if DOME_MODERN_SETUP
  server.on(F("/api/v1/dome/1/name"), HTTP_GET, handleModernName);
#else
  server.on(F("/api/v1/dome/1/name"), HTTP_GET, handleNameGet);
#endif
  server.on(F("/api/v1/dome/1/supportedactions"), HTTP_GET, handleSupportedActionsGet); //tested

  /* ALPACA Management and setup interfaces
     The main browser setup URL would be http://192.168.1.89:7843/setup
    The JSON list of supported interface versions would be available through a GET to http://192.168.1.89:7843/management/apiversions
    The JSON list of configured ASCOM devices would be available through a GET to http://192.168.1.89:7843/management/v1/configureddevices
    Updated 21/09/2025 - satisfies conform.
  */
  //Management API
  server.on(F("/management/apiversions"), HTTP_GET, handleMgmtVersions);
  server.on(F("/management/v1/description"), HTTP_GET, handleMgmtDescription);
#if DOME_MODERN_SETUP
  server.on(F("/management/v1/configureddevices"), HTTP_GET, handleModernConfiguredDevices);
  registerModernSetupRoutes();
#else
  server.on(F("/management/v1/configureddevices"), HTTP_GET, handleMgmtConfiguredDevices);

  //Custom and setup handlers used by the custom setup form - currently there is no collection of devices.
  server.on(F("/setup"), HTTP_GET, handleSetup);                 //Primary browser web page for the overall collection of devices
  server.on(F("/setup/v1/dome/1/setup"), HTTP_GET, handleSetup); //Browser web page for this driver instance

  //HTML forms don't support PUT -  they typically transform them to use GET instead.
  //form commands
  server.on(F("/Hostname"), handleHostnamePut);
  server.on(F("/ShutterName"), handleShutterNamePut);
  server.on(F("/SensorName"), handleSensorNamePut);
  server.on(F("/ParkSet"), handleParkPositionPut);
  server.on(F("/ParkAction"), handleParkActionPut);
  //  server.on(F("/ShutterAction"),handleShutterActionPut );
  server.on(F("/Home"), handleHomePositionPut);
  server.on(F("/Goto"), handleDomeGoto);
  server.on(F("/Sync"), handleSyncOffsetPut);
  server.on(F("/restart"), handlerRestart);
  server.on(F("/"), handlerStatus);
#endif // DOME_MODERN_SETUP
  server.on(F("/status"), HTTP_GET, handlerStatus);

  server.onNotFound(handlerNotFound);
  Serial.println(F("Web handlers registered"));

  //setup interrupt-based 'soft' alarm handler for dome state update and async commands
  ets_timer_setfn(&fineTimer, onFineTimer, NULL);
  ets_timer_setfn(&coarseTimer, onCoarseTimer, NULL);     //Used for onIdle processing
  ets_timer_setfn(&timeoutTimer, onTimeoutTimer, NULL);   //Used for callback timer.
  ets_timer_setfn(&watchdogTimer, onWatchdogTimer, NULL); //Used for slew failure watchdog

  domeCmdList = new LinkedList<cmdItem_t *>();
  shutterCmdList = new LinkedList<cmdItem_t *>();
  cmdStatusList = new LinkedList<cmdItem_t *>(); // use to track async completion state.
  Serial.println(F("LinkedList setup complete"));

  //Start web server
  updater.setup(&server);
  server.begin();
  Serial.println(F("webserver setup complete"));

  //Get startup values
  domeStatus = DOME_IDLE;
  int requestStatus = 0;
  int attemptCount = 0;
  connectionStatus = CONNECTION_DISCONNECTED;

  Serial.printf_P(PSTR("Waiting for shutter\n"));
  do
  {
    //there's a chance of a watchdog timer timeout here.
    requestStatus = getShutterStatus(shutterHostname, shutterStatus);
    Serial.printf(".");
    delay(500);
    yield();
  } while (requestStatus != HTTP_CODE_OK && ++attemptCount < 10);
  if (attemptCount < 10 && requestStatus == HTTP_CODE_OK)
    Serial.printf_P(PSTR("Shutter found OK\n"));
  else
    Serial.printf_P(PSTR("Shutter NOT found\n"));

#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  Serial.printf_P(PSTR("Searching for remote compass/encoder\n"));
  attemptCount = 0;

  do
  {
#if defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
    response = restQuery(sensorHostname, "/encoder/bearing", "", outbuf, HTTP_GET);
#elif defined defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION
    response = restQuery(sensorHostname, "/bearing", outbuf, HTTP_GET);

#endif
    debugI("Waiting for remote encoder/compass\n");
    delay(500);
    yield();
  } while (response != HTTP_CODE_OK && ++attemptCount < 10);
  if (attemptCount < 10 && response == HTTP_CODE_OK)
    Serial.printf_P(PSTR("Found remote compass/encoder\n"));
  else
    Serial.printf_P(PSTR("Remote compass/encoder NOT FOUND \n"));

#endif

  //TODO consider some mech of reading actual position and storing last position between power cycles.
  bearing = getBearing(sensorHostname);
  currentAzimuth = getAzimuth(bearing);
  targetAzimuth = currentAzimuth;
  Serial.printf_P(PSTR("Updated position from encoder\n"));

  //Start timers last
  ets_timer_arm_new(&coarseTimer, ACTIVE_STATUS_PERIOD_MS, 1 /*repeat*/, 1); //Adaptive status and command processing
  ets_timer_arm_new(&fineTimer, 1000, 1 /*repeat*/, 1);                      //millis   1 second - bearing reading.
  ets_timer_arm_new(&watchdogTimer, 5000, 1 /*repeat*/, 1);                  //millis 5 seconds - watchdog check of dome stuck.
#if !defined _DISABLE_MQTT_
  ets_timer_arm_new(&timeoutTimer, 2500, 0 /*one-shot*/, 1); //MQTT background reconnection timer.
#endif

  Serial.println(FPSTR(BuildVersionName));

  //Starts the discovery responder server
  Udp.begin(udpPort);

  //Show welcome message
  debugI("setup complete");
}

void onFineTimer(void *pArg)
{
  //Read command list and apply.
  fineTimerFlag = true;
}

void onCoarseTimer(void *pArg)
{
  //Read command list and apply.
  coarseTimerFlag = true;
}

//Used to complete timeout actions in MQTT background connection checks.
void onTimeoutTimer(void *pArg)
{
  timeoutFlag = true;
}

void onWatchdogTimer(void *pArg)
{
  watchdogTimerFlag = true;
}

void manageConnectionState(void)
{
  switch (connectionStatus)
  {
  case CONNECTION_CONNECTING:
    if (millis() - connectionStateChangedAt < 100)
      break;
    connected = pendingConnectionClientID;
    connectionStatus = CONNECTION_CONNECTED;
    break;

  case CONNECTION_DISCONNECTING:
    if (millis() - connectionStateChangedAt < 100)
      break;
    if (parkDomeOnDisconnect && domeCmdList != nullptr)
      addDomeCmd(pendingConnectionClientID, 0, "", CMD_DOME_PARK, parkPosition);
    if (closeShutterOnDisconnect && shutterCmdList != nullptr)
      addShutterCmd(pendingConnectionClientID, 0, "", CMD_SHUTTER_CLOSE, 0);
    connected = NOT_CONNECTED;
    pendingConnectionClientID = 0;
    connectionStatus = CONNECTION_DISCONNECTED;
    break;

  case CONNECTION_CONNECTED:
    if (connected == NOT_CONNECTED)
      connectionStatus = CONNECTION_DISCONNECTED;
    break;

  case CONNECTION_DISCONNECTED:
    // Keep the V3 state synchronized when the legacy Connected PUT is used.
    if (connected != NOT_CONNECTED)
      connectionStatus = CONNECTION_CONNECTED;
    break;
  }
}

void updateStatusPollingPeriod(void)
{
  DOME_HEAP_SCOPE("updateStatusPollingPeriod");
  const bool domeActive = domeStatus == DOME_SLEWING || domeStatus == DOME_ABORT;
  const bool shutterActive = shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_CLOSING || shutterStatus == SHUTTER_ABORTING;
  const bool commandsPending = (domeCmdList != nullptr && domeCmdList->size() > 0) || (shutterCmdList != nullptr && shutterCmdList->size() > 0);
  const uint32_t requestedPeriod = (domeActive || shutterActive || commandsPending) ? ACTIVE_STATUS_PERIOD_MS : IDLE_STATUS_PERIOD_MS;

  if (requestedPeriod != statusPollingPeriodMs)
  {
    statusPollingPeriodMs = requestedPeriod;
    ets_timer_disarm(&coarseTimer);
    ets_timer_arm_new(&coarseTimer, statusPollingPeriodMs, 1 /*repeat*/, 1);
    debugI("Status polling period changed to %u ms\n", statusPollingPeriodMs);
  }
}

void loop()
{
  const bool traceCoarseLoop = coarseTimerFlag;
  (void)traceCoarseLoop;
  DOME_HEAP_SCOPE_IF("loop.coarseTick", traceCoarseLoop);
  String outbuf;
  String LCDOutput = "";

  //Operate and Clear down flags
  if (fineTimerFlag)
  {
#if defined _ENABLE_BEARING
    DOME_HEAP_SCOPE("loop.bearing");

    bearing = getBearing(sensorHostname);
    currentAzimuth = getAzimuth(bearing);
    debugD("Bearing %03.2f, offset: %f, adjusted: %f\n", bearing, azimuthSyncOffset, currentAzimuth);

#endif
    fineTimerFlag = false;
  }

  if (coarseTimerFlag)
  {
    //Handle state changes
#if defined _ENABLE_DOME
    {
      DOME_HEAP_SCOPE("loop.domeDispatch");

      //For dome
      switch (domeStatus)
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
        case DOME_ABORTED:
        case DOME_HALTED:
        break;
        default:
        debugE("Unexpected Dome status detected: %s\n", domeStateNames[(int)domeStatus]);
        domeStatus = DOME_ABORT; //error condition
        break;
      }

    }
#endif //dome

#if defined _ENABLE_SHUTTER
    {
      DOME_HEAP_SCOPE("loop.shutterDispatch");

      //For shutter
      //Update our knowledge of shutter current status
      if (getShutterStatus(shutterHostname, shutterStatus) == HTTP_CODE_OK)
      {
        debugD("Dome: %s Shutter: %s\n", domeStateNames[(int)domeStatus], shutterStateNames[(int)shutterStatus]);
      }

      switch (shutterStatus)
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
        case SHUTTER_ABORTING:
        case SHUTTER_ABORTED:
        case SHUTTER_HALTED:
        break;
        default: //Anything else.
        debugE("Shutter status unexpected: %s", shutterStateNames[(int)shutterStatus]);
        shutterStatus = SHUTTER_ERROR;
        break;
      }
    }
#endif //shutter

    //Clock tick onLCD
    if (lcdPresent)
    {
      DOME_HEAP_SCOPE("loop.clockLCD");
      int index = 0;
      int lastIndex = 0;
      String output = "";
      getTimeAsString(outbuf);
      index = outbuf.indexOf(" ");
      lastIndex = outbuf.indexOf(".");
      if (index >= 0 && lastIndex >= index)
      {
        LCDOutput = outbuf.substring(index, lastIndex);
        myLCD.writeLCD(1, 1, LCDOutput);
      }
    }

    //If there's nothing going on - slow down polling
    updateStatusPollingPeriod();
    coarseTimerFlag = false;
  }

#if !defined _DISABLE_MQTT
  {
    DOME_HEAP_SCOPE_IF("loop.mqtt", traceCoarseLoop);
    if (!client.connected())
    {
      reconnectNB();
      //reconnect();
    }
    //Service MQTT keep-alives
    client.loop();
    if (callbackFlag) //found as a consequence of being connected
    {
      //publish results
      publishHealth();
      publishFnStatus();
      callbackFlag = false;
    }
  }
#endif

  //If there are any web client connections - handle them.
  {
    DOME_HEAP_SCOPE_IF("server.handleClient", traceCoarseLoop);
    server.handleClient();
  }
  {
    DOME_HEAP_SCOPE_IF("manageConnectionState", traceCoarseLoop);
    manageConnectionState();
  }

  //Check for Alpaca Discovery packets
  {
    DOME_HEAP_SCOPE_IF("handleManagement", traceCoarseLoop);
    handleManagement();
  }

#if !defined _DISABLE_REMOTE_DEBUG
  //Handle remote telnet debug session
  {
    DOME_HEAP_SCOPE_IF("Debug.handle", traceCoarseLoop);
    Debug.handle();
  }
#endif

  delay(20); //If nothing else happens, just slow the loop a touch.
}

/* MQTT callback for subscription and topic.
   Only respond to valid states ""
   Publish under ~/skybadger/sensors/<sensor type>/<host>
   Note that messages have an maximum length limit of 18 bytes - set in the MQTT header file.
*/
void callback(char *topic, byte *payload, unsigned int length)
{
  //set callback flag
  callbackFlag = true;
}

void publishFnStatus(void)
{
  String outTopic;
  String output;
  String timestamp;

  //publish to our device topic(s)
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  getTimeAsString2(timestamp);
  root["time"] = timestamp;
  root["hostname"] = myHostname;
  root["azimuth"] = currentAzimuth;
  root["altitude"] = currentAltitude;
  root["syncOffset"] = azimuthSyncOffset;
  root["shutterStatus"] = shutterStateNames[(int)shutterStatus];
  root["domeStatus"] = domeStateNames[(int)domeStatus];
  serializeJson(root, output);

  outTopic = String(outFnTopic);       //PROGMEM
  outTopic.concat(String(FPSTR(DriverType))); //PROGMEM
  outTopic.concat("/");
  outTopic.concat(myHostname);

  //publish with retention
  if (client.publish(outTopic.c_str(), output.c_str(), true))
  {
    debugI("MQTT FN topic published: %s \n", output.c_str());
  }
  else
  {
    debugW("MQTT FN topic failed to publish: %s \n", output.c_str());
  }
}

/*
  @brief Publish health status to MQTT based on callback process.
*/
void publishHealth()
{
  String outTopic;
  String output;
  String timestamp;

  //publish to our device topic(s)
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  getTimeAsString2(timestamp);
  root["time"] = timestamp;

  // Once connected, publish an announcement...
  root["hostname"] = myHostname;
  if (connected != NOT_CONNECTED)
    root["message"] = F("Dome connected & operating");
  else
    root["message"] = F("Dome waiting for connection");
  serializeJson(root, output);

  //do once after reboot only.
  if (bootCount == 0)
  {
    String buildVersion = String(__DATE__) + " " + FPSTR(BuildVersionName);
    root["version"] = buildVersion.c_str();
    root["resetreason"] = device.getResetReason().c_str();
    root["resetinfo"] = device.getResetInfo().c_str();
    bootCount++;
  }

  outTopic = String(outHealthTopic); //PROGMEM
  outTopic.concat(myHostname);

  if (client.publish(outTopic.c_str(), output.c_str(), true))
  {
    debugI("MQTT Health topic: %s: output: %s", outTopic.c_str(), output.c_str());
  }
  else
  {
    debugW("MQTT Health topic failed : %s: output: %s", outTopic.c_str(), output.c_str());
  }
}

void setupWifi(void)
{
  int zz = 0;

  WiFi.mode(WIFI_STA);
  WiFi.hostname(myHostname);

  WiFi.begin(String(ssid2).c_str(), String(password2).c_str());
  Serial.print("Searching for WiFi..\n");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (zz++ >= 200)
    {
      device.restart();
    }
  }

  Serial.println(F("WiFi connected"));
  Serial.printf_P(PSTR("SSID: %s, Signal strength %i dBm \n\r"), WiFi.SSID().c_str(), WiFi.RSSI());
  Serial.printf_P(PSTR("Hostname: %s\n\r"), WiFi.hostname().c_str());
  Serial.printf_P(PSTR("IP address: %s\n\r"), WiFi.localIP().toString().c_str());
  Serial.printf_P(PSTR("DNS address 0: %s\n\r"), WiFi.dnsIP(0).toString().c_str());
  Serial.printf_P(PSTR("DNS address 1: %s\n\r"), WiFi.dnsIP(1).toString().c_str());
  delay(500);

  //Setup sleep parameters
  wifi_set_sleep_type(NONE_SLEEP_T);

  String host[] = {sensorHostname, shutterHostname};
  for (int i = 0; i < (sizeof(host) / sizeof(host[0])); i++)
  {
    Serial.printf_P(PSTR("Wifi setup rest resolution test - %s\n"), host[i].c_str());

    IPAddress resolvedIP;

    if (WiFi.hostByName(host[i].c_str(), resolvedIP))
    {
      Serial.printf_P(PSTR("DNS: %s -> %s\n"), host[i].c_str(), resolvedIP.toString().c_str());
    }
    else
    {
      Serial.printf_P(PSTR("DNS lookup failed for %s\n"), host[i].c_str());
    }
  }

  Serial.println(F("WiFi connected"));
  delay(500);
}
