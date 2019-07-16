/*
File to be included into relevant device REST setup 
Assumes Use of ARDUINO ESP8266WebServer for entry handlers
*/
#if !defined _ASCOM_DomeSetup 
#define _ASCOM_DomeSetup
#include "JSONHelpFunctions.h"

void handlerNotFound()
{
  String message;
  int responseCode = 400;
  uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
  uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

  JsonObject& root = jsonBuffer.createObject();
  jsonResponseBuilder( root, clientID, transID, "HandlerNotFound", 0x500, "No REST handler found for argument - check ASCOM Dome v2 specification" );    
  JsonObject& err = root.createNestedObject("ErrorMessage");
  err["Value"] = "Dome REST handler not found or parameters incomplete";
  root.printTo(message);
  server.send(400, "text/json", message);
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
  int i=0;
  String form;
  String errMsg;
  int namesFoundCount = 0;
  String newName;
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleHostnamePut" );
  
  //throw error message
  if( server.hasArg("hostName"))
  {
    newName = server.arg("hostName");
  }
  if( newName != null && newName.length() < 20)
  {
    //save new hostname and cause reboot - requires eeprom read at setup to be in place.  
    hostname = newName;
    //Write to EEprom
    server.send( 200, "text/html", "rebooting!" ); 
  }
  else
  {
    errMsg = "handleHostnamePut: Error handling new hostname";
    DEBUGSL1( errMsg );
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html",  ); 
  }
}

//  server.on("/Dome/*/DomeName", HTTP_PUT, handleNamePut );
void handleNamePut( void) 
{
  int i=0;
  String errMsg;
  String form;
  int namesFoundCount = 0;
  
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
  int i=0;
  String form;
  String errMsg;
  int namesFoundCount = 0;
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleDomeGotoPut"  );
  
  //throw error message
  errMsg = "handleDomeGotoPut: Not enough named filternames found in request body";
  DEBUGSL1( errMsg );
  form = setupFormBuilder( form, errMsg );
  server.send( 200, "text/html", form );
}

/*
 * Set filternames from setup web page - managed outside of ascom  
 * REST api provides no way of doing this at time of writing. 
 */
void handleFilterNamesPut(void)
{
  int i=0;
  const String nameStub = "filterName";
  String name;
  String form;
  String localName;
  String errMsg;
  int namesFoundCount = 0;
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleOffsetsPut");
  do
  {
    name = nameStub + i;
    if( server.hasArg(name) )
    {
      localName = server.arg(name);
      //I don't care if you call two filters the same... 
      if ( localName.length() < FilterNameLengthLimit && localName.length() > 0 )
      {
        filterNames[i] = localName;
        namesFoundCount++;
      }
    }  
  }while ( i < filtersPerWheel );
  
  if ( namesFoundCount != filtersPerWheel )
  {
    //throw error message
    errMsg = "UpdateFilterNames:Not enough named filternames found in request body";
    DEBUGSL1( "Not enough named filternames found");
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form );
  }
  else
  {
    errMsg = String("");
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form );
  }
  return;
}

void handleFocusOffsetsPut(void)
{
  int i=0;
  const String nameStub = "filterOffset";
  String name;
  String errMsg;
  String form;
  int namesFoundCount = 0;
  int localOffset = 0;
  
  debugURI(errMsg);
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleOffsetsPut" );
  do
  {
    name = nameStub + i;
    if( server.hasArg(name) )
    {
      localOffset  = server.arg(name).toInt();
      if ( localOffset < stepsPerRevolution && localOffset >= 0 )
      {
        focusOffsets[i] = localOffset;
        namesFoundCount++;
      }
    }  
  }while ( i < filtersPerWheel );
  
  if ( namesFoundCount != filtersPerWheel )
  {
    //throw error message
    String errMsg = "Update filter names:Not enough filter offsets found in request body";
    DEBUGSL1( "Update filter names:Not enough named filter offsets found");
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form );
  }
  else
  {
    errMsg = String("");
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
  int i=0;
  
  htmlForm = "<!DOCTYPE HTML><html><head></head><meta></meta><body><div id=\"ErrorMsg\" >\n";
  if( errMsg != NULL && errMsg.length() > 0 ) 
  {
    htmlForm +="<b>";
    htmlForm.concat( errMsg );
    htmlForm += "</b>";
  }
  htmlForm += "</div>\n"
  
  //Hostname
  htmlForm += "<div id=\"hostname\" >"
  htmlForm += "<h1> Enter new hostname for filter wheel</h1>\n"
  htmlForm += "<form action=\"http://";
  htmlForm.concat( hostname );
  htmlForm += "/Dome/0/Hostname\" method=\"PUT\" id=\"hostname\" >\n";
  htmlForm += "Changing the hostname will cause the filter to reboot and may change the address!\n<br>";
  htmlForm += "<input type=\"text\" name=\"hostname\" value=\"";
  htmlForm.concat( hostname );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Wheelname
  htmlForm += "<div id=\"Domenameset\" >"
  htmlForm += "<h1> Enter new descriptive name for Dome</h1>\n";
  htmlForm += "<form action=\"/Dome/0/Domename\" method=\"PUT\" id=\"domename\" >\n";
  htmlForm += "<input type=\"text\" name=\"wheelname\" value=\"";
  htmlForm.concat( Name );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Number of filters
  htmlForm += "<div id=\"gotoset\" >"
  htmlForm += "<h1> Enter bearing to move dome to </h1>\n"
  htmlForm += "<form action=\"http://";
  htmlForm.concat(hostname);
  htmlForm += "/Dome/0/goto\" method=\"PUT\" id=\"filtersPerWheel\" >\n";
  htmlForm += "<input type=\"text\" name=\"filtersPerWheel\" value=\"";
  htmlForm.concat( filtersPerWheel );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  //Filter names by position
  htmlForm += "<div id=\"syncset\" >"
  htmlForm += "<h1> Enter actual position for dome</h1>\n"
  htmlForm += "<form action=\"http://";
  htmlForm.concat(hostname);
  htmlForm += "/Dome/0/sync\" method=\"PUT\" id=\"sync\" >\n";
    htmlForm += "<input type=\"submit\" value=\"submit\">\n</form></div>\n";
  
  htmlForm += "</body>\n</html>\n";
  return htmlForm;
}


/*
 * Reset the device - Non-ASCOM call
 */
  void handleRootReset()
  {
    String message;
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
