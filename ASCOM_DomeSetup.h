/*
File of ASCOM setup handler functions specific to REST dome interface and not standard to ASCOM
Assumes Use of ARDUINO ESP8266WebServer for entry handlers
*/
#if !defined _ASCOM_DomeSetup 
#define _ASCOM_DomeSetup
#include "JSONHelperFunctions.h"
#include <LinkedList.h>

//variable references
extern EspClass device;
extern int homePosition;
extern int parkPosition;

//Functions
String& setupFormBuilder(String& htmlForm, String& errMsg );
void handleSetup(void);
void handleRestart(void);
void handlerNotFound(void);
void handlerNotImplemented(void );
void handleHostnamePut( void ) ;
void handleDomeGoto( void );
void handleSyncOffsetPut(void);
void handleHomePositionPut(void);
void handleParkPositionPut(void);

void handlerNotFound()
{
  String message;
  int responseCode = 400;
  uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
  uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
  DynamicJsonBuffer jsonBuffer(250);
  JsonObject& root = jsonBuffer.createObject();
  jsonResponseBuilder( root, clientID, transID, ++serverTransID, "HandlerNotFound", invalidOperation , "No REST handler found for argument - check ASCOM Dome v2 specification" );    
  root["Value"] = 0;
  root.printTo(message);
  server.send(responseCode, F("application/json"), message);
}

void handlerStatus()
{
  String message;
  String timestamp;
  int responseCode = 400;
  DynamicJsonBuffer jsonBuffer(250);
  JsonObject& root = jsonBuffer.createObject();

  getTimeAsString( timestamp );
  
  //Status info
  root["time"]                  = timestamp;
  root["dome status"]           = (int) domeStatus;
  root["dome status string"]    = domeStateNames[domeStatus];
  root["shutter status"]        = (int) shutterStatus;
  root["shutter status string"] = shutterStateNames[shutterStatus];
  root["motor speed"]           = motorSpeed;
  root["motor direction"]       = motorDirection;
  root["target azimuth"]        = targetAzimuth;
  root["current azimuth"]       = currentAzimuth;
  root["current altitude"]      = currentAltitude;
  
  root.printTo(message);
  server.send(responseCode=200, F("application/json"), message);
}

void handlerRestart() //PUT or GET
{
  //Trying to do a redirect to the rebooted host so we pick up from where we left off. 
  server.sendHeader( WiFi.hostname().c_str(), String("/status"), true);
  server.send ( 302, F("text/html"), "<!Doctype html><html>Redirecting for restart</html>");
  debugW("Reboot requested");
  device.restart();
}

/*
 * String& setupFormBuilder( String& htmlForm )
 */
 void handleSetup(void)
 {
  String output = "";
  String err = "";
  output = setupFormBuilder( output, err );
  server.send(200, F("text/html"), output );
  return;
 }

//server.on("/Dome/*/Hostname", HTTP_PUT, handleHostnamePut );
void handleHostnamePut( void ) 
{
  String form;
  String errMsg;
  String newName;
  String argToSearchFor = "hostname";
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleHostnamePut" );
  
  //throw error message
  if( hasArgIC( argToSearchFor, server, false ) )
  {
    newName = server.arg( argToSearchFor);
    debugD( "New name: %s", newName.c_str() );    
  }
  if( newName != NULL && newName.length() < MAX_NAME_LENGTH )
  {
    //save new hostname and cause reboot - requires eeprom read at setup to be in place.  
    if ( myHostname != nullptr ) 
      free( myHostname );
    myHostname = (char*) calloc( MAX_NAME_LENGTH, sizeof(char));
    strcpy( myHostname, newName.c_str() );
    //Write new hostname to EEprom
    saveToEeprom();
    
    //Send response 
    //Trying to do a redirect to the rebooted host so we pick up from where we left off. 
    server.sendHeader( WiFi.hostname().c_str(), String("/setup"), true);
    server.send ( 302, F("text/html"), "<!Doctype html><html>Redirecting for restart</html>");
    device.reset();
  }
  else
  {
    errMsg = "handleHostnamePut: Error handling new hostname";
    debugI( "%s", errMsg.c_str() );
    form = setupFormBuilder( form, errMsg );
    server.send( 200, F("text/html"), form ); 
  }
}

//  server.on("/Dome/*/shutterhostname", HTTP_PUT, handleShutterNamePut );
void handleShutterNamePut( void ) 
{
  String form;
  String errMsg;
  String newName;
  String argToSearchFor = "shutterhostname";
  
  debugI( "Entered handleShutterNamePut" );
  
  //throw error message
  if( hasArgIC( argToSearchFor, server, false ) )
  {
    newName = server.arg( argToSearchFor );
    debugD( "New name: %s", newName.c_str() );
  }
  if( newName != NULL && newName.length() < MAX_NAME_LENGTH )
  {
    //save new shutter hostname
    if ( shutterHostname != nullptr )
      free( shutterHostname );
    shutterHostname = (char*)calloc( MAX_NAME_LENGTH, sizeof( char ) );
    strcpy( shutterHostname, newName.c_str() );
    //Write new hostname to EEprom
    saveToEeprom();    
  }
  else
  {
    errMsg = "handleShutterNamePut: Error handling new hostname - check length";
    debugI( "%s", errMsg.c_str() );
  }
  form = setupFormBuilder( form, errMsg );
  server.send( 200, F("text/html"), form ); 
}

#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
//server.on("/Dome/*/sensorname", HTTP_PUT, handleSensorNamePut );
void handleSensorNamePut( void ) 
{
  String form;
  String errMsg;
  String newName;
  String argsToSearchFor[] = {"bearingsensorname"};
  
  debugI( "Entered handleHostnamePut" );
  
  //throw error message
  if( hasArgIC( argsToSearchFor[0], server, false ) )
  {
    newName = server.arg( argsToSearchFor[0]);
    debugD( "New name: %s", newName.c_str() );    
  }
  if( newName != NULL && newName.length() < MAX_NAME_LENGTH )
  {
    //save new hostname - requires eeprom read at setup to be in place.  
    if( sensorHostname != nullptr )
      free( sensorHostname );
    sensorHostname = (char*)calloc( MAX_NAME_LENGTH, sizeof( char ) );
    strcpy( sensorHostname, newName.c_str() );
    //Write new hostname to EEprom
    saveToEeprom();  
  }
  else
  {
    errMsg = "Error handling new hostname - check length";
    debugI( "%s", errMsg.c_str() );
  }
  //Send response 
  form = setupFormBuilder( form, errMsg );
  server.send( 200, F("text/html"), form ); 
  return;
}
#endif

void handleDomeGoto( void )
{
  String form;
  String errMsg;
  String argsToSearchFor[] = {"bearing"};
  String value;
  int newGoto = 0;
  
  debugI( "Entered handleDomeGotoPut"  );

  if( hasArgIC( argsToSearchFor[0], server, false ) )
  {
    value = server.arg(argsToSearchFor[0]);
    newGoto = (int) value.toInt();    
    if ( newGoto >= 0 && newGoto <=360 )
    {
      //update current position by moving to current azimuth
      //Add to list at top.
      addDomeCmd( 100, 1000, "", CMD_DOME_SLEW, newGoto );
    }
  }  
  else
  {
    errMsg = String("Goto position not in range 0-360");
    debugI("%s", errMsg.c_str() );
  }
  form = setupFormBuilder( form, errMsg );
  server.send( 200, F("text/html"), form );
}

/*
 * Set sync offset from setup web page - managed outside of ascom  
 * REST api provides no way of doing this at time of writing. 
 */
void handleSyncOffsetPut( void)
{
  const String nameStub = "syncOffset";
  String form;
  String name;
  String localName;
  String errMsg;
  float newGoto = 0;
  String argsToSearchFor[] = { "syncOffset" };
  
  debugI( "Entered handleSyncOffsetPut");
  float offsetVal = 0.0F;
  
  if( hasArgIC( argsToSearchFor[0], server, false ) )
  {
    localName = server.arg(argsToSearchFor[0]);
    newGoto = (int) localName.toFloat();    
 
    //range should be limited in HTML form constraints
    if ( newGoto >= 0.0F && newGoto <=360.0F )
    {
      offsetVal = newGoto - bearing;
      normaliseFloat( offsetVal, 360.0F );

      //Add to list at top.
      addDomeCmd( 100, 1000, "azimuthSyncOffset" , CMD_DOMEVAR_SET, offsetVal );
    }
    else
    {
      errMsg = String("Goto position not in range 0-360");
      debugI("%s",errMsg.c_str() );
    }
  }
  form = setupFormBuilder( form, errMsg );
  server.send( 200, F("text/html"), form );
}

void handleHomePositionPut(void)
{
  String argsToSearchFor[] = {"homePosition"};
  String errMsg;
  String form;
  int newHome;
  
  debugI( "Entered handleHomePositionPut" );
  if( hasArgIC( argsToSearchFor[0], server, false ) )
  {
    newHome  = server.arg(argsToSearchFor[0]).toInt();
    if ( newHome >= 0 && newHome <= 360)
    {
      homePosition = newHome;
    }
    else
    {
      errMsg = String("New home position not within range of 0 - 360");
      debugI("%s", errMsg.c_str() );
    }
  }  
  form = setupFormBuilder( form, errMsg );  
  server.send( 200, F("text/html"), form );
  return;
}

void handleParkPositionPut(void)
{
  String argsToSearchFor[] = {"parkPosition"};
  String name;
  String errMsg;
  String form;
  int newPark;
  
  debugI( "handleParkPositionPut" );
  if( hasArgIC( argsToSearchFor[0], server, false ) )
  {
    newPark  = server.arg(argsToSearchFor[0]).toInt();
    if ( newPark >= 0 && newPark <= 360)
    {
      parkPosition = newPark;
    }
    else
    {
      errMsg = String("New Park position not within range of 0 - 360");
      debugI("%s", errMsg.c_str() );
    }
  }  
  form = setupFormBuilder( form, errMsg );  
  server.send( 200, F("text/html"), form );
  return;
}

/*
 * Handler for setup dialog - issue call to <hostname>/setup and receive a webpage
 * Fill in form and submit and handler for each form button will store the variables and return the same page.
 optimise to something like this:
 https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/
 Bear in mind HTML standard doesn't support use of PUT in forms and changes it to GET so arguments get sent in plain sight as 
 part of the URL.
 */
String& setupFormBuilder( String& htmlForm, String& errMsg )
{
  htmlForm = "<!DOCTYPE HTML><html><head></head><meta></meta><body><div id=\"ErrorMsg\" >\n";
  if( errMsg != NULL && errMsg.length() > 0 ) 
  {
    htmlForm +="<b background='red' >";
    htmlForm.concat( errMsg );
    htmlForm += "</b>";
  }
  htmlForm += "</div>\n";
  
  //Hostname
  htmlForm += "<div id=\"hostnameset\" >";
  htmlForm += "<h2> Enter new hostname for ASCOM Dome driver host</h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( myHostname );
  htmlForm += "/Hostname\" method=\"PUT\" id=\"hostname\" >\n";
  htmlForm += "Changing the hostname will cause the dome hardware to reboot and may change the address!\n<br>";
  htmlForm += "<input type=\"text\" name=\"hostname\" value=\"";
  htmlForm.concat( myHostname );
  htmlForm += "\" maxlength=\"";
  htmlForm += MAX_NAME_LENGTH;
  htmlForm += "\" >\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Shutter controller hostname
  htmlForm += "<div id=\"Shutternameset\" >";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( myHostname );
  htmlForm += "/ShutterName\" method=\"PUT\" id=\"shutterhostname\" >\n";
  htmlForm += "<h2> Enter new URL path \<hostname\>/\<path\> for Shutter controller </h2>\n";
  htmlForm += "<input type=\"text\" name=\"shutterhostname\" value=\"";
  htmlForm.concat( shutterHostname );
  htmlForm += "\" maxlength=\"";
  htmlForm += MAX_NAME_LENGTH;
  htmlForm += "\" >\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
#if defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION  || defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION  
  //Device name - do we really want to enable this to be changed ? Yes, and the url to be updatable too. 
  htmlForm += "<div id=\"Bearingsensorname\" >";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( myHostname );
  htmlForm += "/SensorName\" method=\"PUT\" id=\"bearingsensorname\" >\n";
  htmlForm += "<h2> Enter new URL \<hostname\>/\<path\> for Dome orientation sensor </h2>\n";
  htmlForm += "<p> Rest response returned must return a 'bearing' entry in json form</p>\n";
  htmlForm += "<input type=\"text\" name=\"bearingsensorname\" value=\"";
  htmlForm.concat( sensorHostname );
  htmlForm += "\" maxlength=\"";
  htmlForm += MAX_NAME_LENGTH;
  htmlForm += "\" >\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
#endif

  //Dome goto position
  htmlForm += "<div id=\"gotoset\" >";
  htmlForm += "<h2> Enter bearing to move dome to </h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( myHostname );
  htmlForm += "/Goto\" method=\"PUT\" id=\"goto\" >\n";
  htmlForm += "<input type=\"number\" name=\"bearing\" max=\"360.0\" min=\"0.0\" value=\"";
  htmlForm += parkPosition;
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Dome sync offset
  htmlForm += "<div id=\"syncset\" >";
  htmlForm += "<h2> Enter actual position for dome to sync to </h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat(myHostname);
  htmlForm += "/Sync\" method=\"PUT\" id=\"sync\" >\n";
  htmlForm += "<input type=\"number\" name=\"syncOffset\" max=\"360.0\" min=\"0.0\" value=\"";
  htmlForm += bearing;
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Dome home position
  htmlForm += "<div id=\"homeset\" >";
  htmlForm += "<h2> Enter new home position for dome </h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat(myHostname);
  htmlForm += "/Home\" method=\"PUT\" id=\"home\" >\n";
  htmlForm += "<input type=\"number\" name=\"homePosition\" max=\"360.0\" min=\"0.0\" value=\"";
  htmlForm += homePosition;
  htmlForm += "\">\n";  
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Dome park position
  htmlForm += "<div id=\"parkset\" >";
  htmlForm += "<h2> Enter new Park position for dome </h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat(myHostname);
  htmlForm += "/Park\" method=\"PUT\" id=\"park\" >\n";
  htmlForm += "<input type=\"number\" name=\"parkPosition\" max=\"360.0\" min=\"0.0\" value=\"";
  htmlForm += parkPosition;
  htmlForm += "\">\n";  
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Consider also - for later, for systems that don't calc this for us, we 
  //may have to do it ourselves

  //Auto-tracking enable
  //W of pier
  //N of pier
  //Elevation above centre
  //Dome radius
  //Distance from RA axis.
  
  htmlForm += "</body>\n</html>\n";
  return htmlForm;
}

#endif
