/*
File to be included into relevant device REST setup 
*/
//Assumes Use of ARDUINO ESP8266WebServer for entry handlers
#if !defined _ESP8266_DomeHandler_h_ 
#define _ESP8266_DomeHandler_h_
#include "AlpacaErrorConsts.h"
#include "ESP8266_AscomDome.h"
#include "ASCOMAPIDOME_rest.h"     //Definitions for functions
#include "JSONHelperFunctions.h"

#include <LinkedList.h>

/* 
extern int azimuth;
extern int altitude;
extern int parkPosition;
extern int homePosition;
extern bool canSyncAzimuth; 
extern bool canSetAltitude, canSetAzimuth;
extern bool atHome, atPark, canFindHome, canPark, canSetPark;
extern bool canSetShutter;
extern bool canSlave;
extern LinkedList <cmdItem_t*> domeCmdList;
extern LinkedList <cmdItem_t*> shutterCmdList;

extern cmdItem_t* addDomeCmd( int, int, enum domeCmd, int ); 
extern cmdItem_t* addShutterCmd( int, int, enum shutterCmd, int ); 
*/
bool hasArgIC( String& check, ESP8266WebServer& ref, bool caseSensitive );

void handleAltitudeGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "AltitudeGet", Success, "" );    


    /*Check there is no pending altitude cmd that this needs to wait for. 
    cmdItemPtr *pCmd;
    for( int i=0; i< domeCmdList.size(); pCmd = domeCmdList[0] )
     if (pCmd->cmd == CMD_DOMEVAR_SET && pCmd->value != -1 && !pCmd->cmdName.equalsIgnoreCase( "Altitude")")
     {
          root["Value"] = altitude;
     }
    else
    {
      
    }
    This is the sort of way we would like to behave
    */
    
    root["Value"] = altitude;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    debugI( "AltitudeGet: %s", message.c_str() );    
    server.send(200, F("application/json"), message);
    return ;
}

void handleAtHomeGet( void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "AtHome", 0, "" );    
    root["Value"] = ( abs( currentAzimuth - homePosition ) <= acceptableAzimuthError )? atHome = true : atHome = false;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    debugI( "AtHomeGet : %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;
}

void handleAtParkGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID,"AtPark", 0, "" );    
    root["Value"] = ( abs( currentAzimuth - parkPosition) <= acceptableAzimuthError ) ? atPark = true : atPark = false;
    root.printTo(message);
    debugI("AtParkGet: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;
}

void handleAzimuthGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "Azimuth", 0, "" );    
    root["Value"] = normaliseFloat( currentAzimuth, 360.0);
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    debugI("AzimuthGet: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;
}

void handleCanFindHomeGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CanFindHome", 0, "" );    
    root["Value"] = canFindHome;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    debugI("CanFindHomeGet: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;
}

void handleCanParkGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    DynamicJsonBuffer jsonBuff(256);
    JsonObject& root = jsonBuff.createObject();

    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CanPark", 0, "" );    
    root["Value"] = canPark;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    debugI("CanParkGet: %s", message.c_str() );    
    server.send(200, F("application/json"), message);
    return ;
}

void handleCanSetAltitudeGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CanSetAltitude", 0, "" );    
    root["Value"] = canSetAltitude;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    debugI("CanSetAltitudeGet: %s", message.c_str() );    
    server.send(200, F("application/json"), message);
    return ;  
}

void handleCanSetAzimuthGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CanSetAzimuth", Success, "" );    
    root["Value"] = canSetAzimuth;
    root.printTo(message);
    debugI( "CanSetAzimuthGet: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;  
}

void handleCanSetParkGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CanSetPark", 0, "" );    
    root["Value"] = canSetPark;
    root.printTo(message);
    debugI( "CanSetParkGet: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;  
}

void handleCanSetShutterGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CanSetShutter", Success, "" );    
    root["Value"] = canSetShutter;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    debugI( "CanSetShutterGet: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;  
}

void handleCanSlaveGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CanSlave", Success, "" );
    root["Value"] = canSlave;
    root.printTo(message);
    debugI( "CanSlaveGet: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;  
}

void handleCanSyncAzimuthGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CanSyncAzimuth", Success, "" );    
    root["Value"] = canSyncAzimuth;
    root.printTo(message);
    debugI( "CanSyncAzimuthGet: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;  
}

//'Slaved' indicates a hardware link between the telescope and dome  - the ASCOM link is not the same. 
void handleSlavedGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "Slaved", notImplemented, "Not Implemented" );    
    root["Value"] = slaved;
    root.printTo(message);
    debugI( "SlavedGet: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;  
}

void handleSlavedPut(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "Slaved", notImplemented, "Not Implemented" );    
    root["Value"] = false;
    root.printTo(message);
    debugI( "SlavedPut: %s", message.c_str() );
    server.send(200, F("application/json"), message);
    return ;  
}

void handleSlewingGet(void)
{
  String message;
  DynamicJsonBuffer jsonBuffer(256);
  JsonObject& root = jsonBuffer.createObject();

  String argsToSearchFor[] = {"clientID","clientTransactionID"};
  uint32_t clientID = 0;
  uint32_t transID = 0;
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
     clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();

  jsonResponseBuilder( root, clientID, transID, ++serverTransID, "Slewing", Success, "" );    
  root["Value"] = ( domeStatus == DOME_SLEWING ) ? true:false;
  root.printTo(message);
  debugI( "SlewingGet: %s", message.c_str() );
  server.send(200, F("application/json"), message);
  return ; 
}

void handleAbortSlewPut(void)
{
  String message;
  DynamicJsonBuffer jsonBuffer(256);
  JsonObject& root = jsonBuffer.createObject();

  String argsToSearchFor[] = {"clientID","clientTransactionID"};
  uint32_t clientID =0;
  uint32_t transID =0;
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
     clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();

  if( connected != clientID )
  {
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "AbortSlew", notConnected, "Client not connected" );
  }
  else if( domeStatus != DOME_SLEWING  )
  {
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "AbortSlew", invalidOperation, "Not slewing" );       
  }
  else
  {
    /* Just set status to DOME_ABORT and next top level loop will cause DOME_ABORT processing regardless of contents in 
       command stack. Next command will then be popped. Desirable behaviour ?
       We do 'abort' due to a failed movement, so we could put it to the top of the command stack but unless we 
       continually peek the next command, we wont see it until the current movement you actually want aborted 
       has finished
      //addDomeCmd( clientID, transID, "", CMD_DOME_ABORT, 0 );
    */
    domeStatus = DOME_ABORT;
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "AbortSlew", Success, "" );
  }
  root["value"] = connected; //return the current connected Client ID.
  root.printTo(message);
  debugI( "AbortSlewPut: %s", message.c_str() );    
  server.send(200, F("application/json"), message);
  return ;   
}

void handleShutterStatusGet(void)
{
  String message;
  DynamicJsonBuffer jsonBuffer(256);
  JsonObject& root = jsonBuffer.createObject();

  String argsToSearchFor[] = {"clientID","clientTransactionID"};
  uint32_t clientID =0;
  uint32_t transID =0;
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
     clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();

  //anyone can get the shutter status.
  jsonResponseBuilder( root, clientID, transID, ++serverTransID, "ShutterStatus", Success, "" );
  root["Value"] = shutterStatus;
  //0 = Open, 1 = Closed, 2 = Opening, 3 = Closing, 4 = Shutter status error
  //JsonArray& offsets = root.createNestedArray("Value");
  root.printTo(message);
  debugI( "ShutterStatusGet: %s", message.c_str() );      
  server.send(200, F("application/json"), message);
  return ;   
}

void handleCloseShutterPut(void)
{
  String message;
  DynamicJsonBuffer jsonBuffer(256);
  JsonObject& root = jsonBuffer.createObject();

  String argsToSearchFor[] = {"clientID","clientTransactionID"};
  uint32_t clientID =0;
  uint32_t transID =0;
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
     clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();

  if ( connected != clientID ) 
  {
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CloseShutter", notConnected, "Not connected" );           
  }
  //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
  else 
  {
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "CloseShutter", Success, "" );
    //Create new async command
    addShutterCmd( clientID, transID, "", CMD_SHUTTER_CLOSE, 0 ); 
  }
  //JsonArray& offsets = root.createNestedArray("Value");
  root.printTo(message);
  debugI( "CloseShutterPut: %s", message.c_str() );        
  server.send(200, F("application/json"), message);
  return ;   
}

//Kick off the find home operation ???
void handleFindHomePut(void)
{
   String message;
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();

   String argsToSearchFor[] = {"clientID","clientTransactionID"};
   uint32_t clientID = 0;
   uint32_t transID = 0;
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
     clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();

   if ( connected != clientID ) 
   {
     jsonResponseBuilder( root, clientID, transID, ++serverTransID, "FindHome", notConnected, "Not connected" );       
   }
   else
   {
      //Set command to move to home.
      addDomeCmd( clientID, transID, "", CMD_DOME_SLEW, homePosition ); 
      jsonResponseBuilder( root, clientID, transID, ++serverTransID, "FindHome", Success, "" );
   }
   root.printTo(message);
   debugI( "FindHomePut: %s", message.c_str() );        
   server.send(200, F("application/json"), message);
   return ;   
}

void handleOpenShutterPut(void)
{
   String message;
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();

   String argsToSearchFor[] = {"clientID","clientTransactionID"};
   uint32_t clientID = 0;
   uint32_t transID = 0;
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
     clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();
   
   //in this driver model,each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( connected != clientID ) 
      jsonResponseBuilder( root, clientID, transID, ++serverTransID, "OpenShutter", notConnected, "Client not connected" );    
   else if ( shutterStatus == SHUTTER_OPEN || shutterStatus == SHUTTER_OPENING) 
      jsonResponseBuilder( root, clientID, transID, ++serverTransID, "OpenShutter", Success, "" );    
   else if( shutterStatus == SHUTTER_CLOSING )
   {
      //Set command to open shutter.
      addShutterCmd( clientID, transID, "", CMD_SHUTTER_OPEN, 0 );
      jsonResponseBuilder( root, clientID, transID, ++serverTransID, "OpenShutter", Success, "" );    
   }
   else
   {
      jsonResponseBuilder( root, clientID, transID, ++serverTransID, "OpenShutter", invalidOperation, "Dome shutter not idle" );       
   }
   root.printTo(message);
   debugI( "OpenShutterPut: %s", message.c_str() );           
   server.send( 200, F("application/json"), message);
   return ;   
}

//Set park operates when the dome is at the desired park position
/*
 * No argument provided - uses current azimuth and altitude. 
 */
void handleSetParkPut(void)
{
   String message;
   int location = 0;
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();

   String argsToSearchFor[] = {"clientID","clientTransactionID",};
   uint32_t clientID = 0;
   uint32_t transID = 0;
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
     clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();
    
  if( connected != clientID )
      jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("SetPark"), notConnected, "Not connected" );    
  else
  {
    //Set new park location.
    parkPosition = currentAzimuth;
    addDomeCmd( clientID, transID, "parkPosition", CMD_DOMEVAR_SET, parkPosition );      
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("SetPark"), Success, "" );    
  }

   root.printTo(message);
   debugI( "SetParkPut: %s", message.c_str() );            
   server.send(200, F("application/json"), message);
   return ;      
}

//ParkPut causes the dome to move to the already stored Park position 
void handleParkPut(void)
{
   String message;
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();

   String argsToSearchFor[] = {"clientID","clientTransactionID",};
   uint32_t clientID = 0;
   uint32_t transID = 0;
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
     clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();
    
   addDomeCmd( clientID, transID, "", CMD_DOME_SLEW, parkPosition );      
   jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("Park"), Success, "" );    
   root.printTo(message);
   debugI( "ParkPut: %s", message.c_str() );
   server.send(200, "text/json", message);
   return ;      
}

void handleSlewToAltitudePut(void)
{
   String message;
   float location = 0.0F;
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();

   String argsToSearchFor[] = {"clientID","clientTransactionID","altitude"};
   uint32_t clientID = 0;
   uint32_t transID = 0;
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
     clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();
  
  if( hasArgIC( argsToSearchFor[2], server, false ) )
     location = (boolean) server.arg( argsToSearchFor[2]).toFloat();
   
  if( connected != clientID ) 
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("SlewToAltitude"), notConnected, "Client not connected" );    
  else if ( server.hasArg( argsToSearchFor[2] ) )
  {
    //Set new shutter altitude.
    normaliseFloat( location, SHUTTER_MAX_ALTITUDE );
    addShutterCmd( clientId, transID, "", CMD_SHUTTER_OPEN, location );
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("SlewToAltitude"), Success, "" );    
  }
  else
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("SlewToAltitude"), valueNotSet, F("Altitude not provided") );       
  root.printTo(message);
  debugI( "SlewToAltitudePut: %s", message.c_str() );    
  server.send(200, F("application/json"), message);
  return ;      
}

/*
 * The handler receives target Azimuth in real degrees
 */
void handleSlewToAzimuthPut(void)
{
   String message;
   
   String argsToSearchFor[] = {"clientID","clientTransactionID","Azimuth"};
   uint32_t clientID = 0;
   uint32_t transID = 0;
   
   float location = 0.0F;
   
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();
   
  if( hasArgIC( argsToSearchFor[0], server, false ) )
    clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();
  
  if( hasArgIC( argsToSearchFor[2], server, false ) )
    location = server.arg( argsToSearchFor[2]).toFloat();
     
  debugI( "SlewToAzimuthPut: %f", location );
 
  if( connected != clientID ) 
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("SlewToAzimuth"), notConnected, "Not connected" ); 
  else if( server.hasArg( argsToSearchFor[2] ) )
  {
    //Set new slew location.
    normaliseFloat( location, 360.0F );
    debugD( "SlewToAzimuthPut: %f", location );
    addDomeCmd( clientID, transID, "SlewToAzimuthPut", CMD_DOME_SLEW, location );
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("SlewToAzimuth"), Success, "" ); 
  }
  else 
  {
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "SlewToAzimuth", valueNotSet, "Azimuth argument not found" );
  }

  root.printTo(message);
  debugI( "SlewToAzimuthPut: %s", message.c_str() );    
  server.send(200, F("application/json"), message);
  return ;
}

/*
 * The handler receives target Azimuth in decimal degrees. 
 */
void handleSyncToAzimuthPut(void)
{
   String message;
   float location=0;
   int iTargetAzimuth = 0;

   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();

   String argsToSearchFor[] = {"clientID","clientTransactionID","Azimuth"};
   uint32_t clientID = 0;
   uint32_t transID = 0;
    
  if( hasArgIC( argsToSearchFor[0], server, false ) )
    clientID = (uint32_t)server.arg( argsToSearchFor[0] ).toInt();
  
  if( hasArgIC( argsToSearchFor[1], server, false ) )
     transID = (uint32_t)server.arg( argsToSearchFor[1] ).toInt();
  
  if( hasArgIC( argsToSearchFor[2], server, false ) )
     location = server.arg( argsToSearchFor[2]).toFloat();

  if( connected != clientID ) 
      jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("SyncToAzimuth"), notConnected, "Not connected" );    
  else if( server.hasArg( argsToSearchFor[2] ) )
  {
    //Set new (actual) azimuth location.
    normaliseFloat( location, 360.0F );
    
    //The offset is the difference between where the dome says it is currently pointing and where we are told it is pointing.
    azimuthSyncOffset = (float) ( location - bearing );
    debugD( "azimuthsync - input: %f, currentAz: %f, offset %f", location, currentAzimuth, azimuthSyncOffset);
    //Because we can go out of range we do this again
    normaliseFloat( azimuthSyncOffset, 360.0F );
    debugD( "azimuthsync - normalised offset: %f ",  azimuthSyncOffset );
    
    addDomeCmd( clientID, transID, "azimuthSyncOffset", CMD_DOMEVAR_SET, azimuthSyncOffset );
    //Tell the dome to move to the target azimuth using the updated correction - should be trivial.
    addDomeCmd( clientID, transID, "", CMD_DOME_SLEW, (int) getAzimuth( bearing ) );
    
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("SyncToAzimuth"), Success, "" );    
  }
  else
    jsonResponseBuilder( root, clientID, transID, ++serverTransID, "SyncToAzimuth", valueNotSet, "Argument not found" );       

  root.printTo(message);
  debugI( "SyncToAzimuthPut: %s", message.c_str() );    
  server.send(200, F("application/json"), message);
  return ;
}

#endif
