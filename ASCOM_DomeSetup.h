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
void handleRootReset(void);
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
  jsonResponseBuilder( root, clientID, transID, "HandlerNotFound", 0x500, "No REST handler found for argument - check ASCOM Dome v2 specification" );    
  JsonObject& err = root.createNestedObject("ErrorMessage");
  err["Value"] = "Dome REST handler not found or parameters incomplete";
  root.printTo(message);
  server.send(responseCode, "text/json", message);
}

void handlerNotImplemented()
{
  String message;
  int responseCode = 400;
  uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
  uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

  DynamicJsonBuffer jsonBuffer(250);
  JsonObject& root = jsonBuffer.createObject();
  jsonResponseBuilder( root, clientID, transID, "HandlerNotFound", 0x500, "No REST handler implemented for argument - check ASCOM Dome v2 specification" );    
  JsonObject& err = root.createNestedObject("ErrorMessage");
  err["Value"] = "Dome REST handler not found or parameters incomplete";
  root.printTo(message);
  server.send(responseCode, "text/json", message);
}

/*
 * String& setupFormBuilder( String& htmlForm )
 */
 void handleSetup(void)
 {
  String output = "";
  String err = "";
  output = setupFormBuilder( output, err );
  server.send(200, "text/html", output );
  return;
 }

//server.on("/Dome/*/Hostname", HTTP_PUT, handleHostnamePut );
void handleHostnamePut( void ) 
{
  String form;
  String errMsg;
  String newName;
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleHostnamePut" );
  
  //throw error message
  if( server.hasArg("hostName"))
  {
    newName = server.arg("hostName");
  }
  if( newName != NULL && newName.length() < nameLengthLimit)
  {
    //save new hostname and cause reboot - requires eeprom read at setup to be in place.  
    newName.toCharArray( Name, newName.length(), 0 );
    //Write new hostname to EEprom
    //......
    
    //Send response 
    server.send( 200, "text/html", "rebooting!" ); 
    //
    device.reset();
  }
  else
  {
    errMsg = "handleHostnamePut: Error handling new hostname";
    DEBUGSL1( errMsg );
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form ); 
  }
}

//  server.on("/Dome/*/DomeName", HTTP_PUT, handleNamePut );
void handleNamePut( void) 
{
  String errMsg;
  String form;

  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleNamePut" );
  
  //throw error message
  errMsg = "handleNamePut: Not handled yet";
  DEBUGSL1( errMsg );
  form = setupFormBuilder( form, errMsg );
  server.send( 200, "text/html", form );
}

//  server.on("/Dome/*/FilterCount", HTTP_PUT, handleFilterCountPut );
void handleDomeGoto( void )
{
  String form;
  String errMsg;
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleDomeGotoPut"  );
  
  //throw error message
  errMsg = "handleDomeGotoPut: Not yet implemented";
  DEBUGSL1( errMsg );
  form = setupFormBuilder( form, errMsg );
  server.send( 200, "text/html", form );
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
  int newGoto = 0;
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleSyncOffsetPut");

  name = nameStub;
  if( server.hasArg( name ) && server.hasArg( "ClientID") && server.hasArg( "transID" ) )
  {
    localName = server.arg(name);
    newGoto = (int) localName.toInt();    
    if ( newGoto >= 0 && newGoto <=360 )
    {
      azimuthSyncOffset = newGoto;
      //update current position by moving to current azimuth
      //Add to list at top.
      addDomeCmd( server.arg( "clientID" ).toInt(), server.arg("transID").toInt(), CMD_DOME_SLEW, currentAzimuth );
    }
  }  
  else
  {
    errMsg = String("Goto position not in range 0-360");
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form );
  }
  return;
}

void handleHomePositionPut(void)
{
  const String nameStub = "homePosition";
  String name;
  String errMsg;
  String form;
  int newHome;
  
  debugURI(errMsg);
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleHomePositionPut" );
  name = nameStub;
  if( server.hasArg(name) )
  {
    newHome  = server.arg(name).toInt();
    if ( newHome >= 0 && newHome <= 360)
    {
      homePosition = newHome;
    }
  }  
  else
  {
    errMsg = String("New home position not within range of 0 - 360");
    form = setupFormBuilder( form, errMsg );  
    server.send( 200, "text/html", form );
  }
  return;
}

void handleParkPositionPut(void)
{
  int i=0;
  const String nameStub = "parkPosition";
  String name;
  String errMsg;
  String form;
  int newPark;
  
  debugURI(errMsg);
  DEBUGSL1 (errMsg);
  DEBUGSL1( "handleParkPositionPut" );
  name = nameStub + i;
  if( server.hasArg(name) )
  {
    newPark  = server.arg(name).toInt();
    if ( newPark >= 0 && newPark <= 360)
    {
      parkPosition = newPark;
    }
  }  
  else
  {
    errMsg = String("New Park position not within range of 0 - 360");
    form = setupFormBuilder( form, errMsg );  
    server.send( 200, "text/html", form );
  }
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
  htmlForm += "<h2> Enter new hostname for dome driver host</h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( myHostname );
  htmlForm += "/Hostname\" method=\"PUT\" id=\"hostname\" >\n";
  htmlForm += "Changing the hostname will cause the dome hardware to reboot and may change the address!\n<br>";
  htmlForm += "<input type=\"text\" name=\"hostname\" value=\"";
  htmlForm.concat( myHostname );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Device name - do we really want to enable this to be changed ?
  htmlForm += "<div id=\"Domenameset\" >";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( Name );
  htmlForm += "/Name\" method=\"PUT\" id=\"domename\" >\n";
  htmlForm += "<h2> Enter new descriptive name for Dome driver </h2>\n";
  htmlForm += "<form action=\"Domename\" method=\"PUT\" id=\"domename\" >\n";
  htmlForm += "<input type=\"text\" name=\"wheelname\" value=\"";
  htmlForm.concat( Name );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Dome goto position
  htmlForm += "<div id=\"gotoset\" >";
  htmlForm += "<h2> Enter bearing to move dome to </h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( myHostname );
  htmlForm += "/Goto\" method=\"PUT\" id=\"goto\" >\n";
  htmlForm += "<input type=\"text\" name=\"Goto bearing &deg\" value=\"0\"\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Dome sync offset
  htmlForm += "<div id=\"syncset\" >";
  htmlForm += "<h2> Enter actual position for dome to sync to </h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat(myHostname);
  htmlForm += "/Sync\" method=\"PUT\" id=\"sync\" >\n";
  htmlForm += "<input type=\"text\" name=\"Actual bearing &deg\" value=\"0\"\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Dome home position
  htmlForm += "<div id=\"homeset\" >";
  htmlForm += "<h2> Enter new home position for dome </h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat(myHostname);
  htmlForm += "/Home\" method=\"PUT\" id=\"home\" >\n";
  htmlForm += "<input type=\"text\" name=\"New Home position &deg\" value=\"";
  htmlForm += homePosition;
  htmlForm += "\">\n";  
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Dome park position
  htmlForm += "<div id=\"parkset\" >";
  htmlForm += "<h2> Enter new home position for dome </h2>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat(myHostname);
  htmlForm += "/ParkSet\" method=\"PUT\" id=\"park\" >\n";
  htmlForm += "<input type=\"text\" name=\"New Park value &deg\" value=\"";
  htmlForm += parkPosition;
  htmlForm += "\">\n";  
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Consider also - for later, for systems that don't calc this for us, we 
  //may have to do it ourselves

  //Auto-tracking enable
  //W of pier
  //N of pier
  //Elevatin above centre
  //Dome radius
  //Distance from RA axis.
  
  htmlForm += "</body>\n</html>\n";
  return htmlForm;
}

/*
 * Reset the device - Non-ASCOM call
 */
void handleRootReset()
{
  String message;
  DynamicJsonBuffer jsonBuffer(250);
  JsonObject& root = jsonBuffer.createObject();
  root["messageType"] = "Alert";
  root["message"]= "Dome controller Resetting";
  Serial.println( "Server resetting" );
  root.printTo(message);
  server.send(200, "application/json", message);
  device.restart();
  return;
}

#endif
