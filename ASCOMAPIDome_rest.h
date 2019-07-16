/*
File to be included into relevant device REST setup 
*/
//Assumes Use of ARDUINO ESP8266WebServer for entry handlers
#if !defined _ASCOM_Dome 
#define _ASCOM_Dome
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

//GET /dome/{device_number}/altitude           The dome altitude
//GET /dome/{device_number}/athome             Indicates whether the dome is in the home position.
//GET /dome/{device_number}/atpark             Indicates whether the telescope is at the park position
//GET /dome/{device_number}/azimuth            The dome azimuth
//GET /dome/{device_number}/canfindhome        Indicates whether the dome can find the home position.
//GET /dome/{device_number}/canpark            Indicates whether the dome can be parked.
//GET /dome/{device_number}/cansetaltitude     Indicates whether the dome altitude can be set
//GET /dome/{device_number}/cansetazimuth      Indicates whether the dome azimuth can be set
//GET /dome/{device_number}/cansetpark         Indicates whether the dome park position can be set
//GET /dome/{device_number}/cansetshutter      Indicates whether the dome shutter can be opened
//GET /dome/{device_number}/canslave           Indicates whether the dome supports slaving to a telescope
//GET /dome/{device_number}/cansyncazimuth     Indicates whether the dome azimuth position can be synched
//GET /dome/{device_number}/shutterstatus      Status of the dome shutter or roll-off roof
//GET /dome/{device_number}/slaved             Indicates whether the dome is slaved to the telescope
//PUT /dome/{device_number}/slaved             Sets whether the dome is slaved to the telescope
//GET /dome/{device_number}/slewing            Indicates whether the any part of the dome is moving
//PUT /dome/{device_number}/abortslew          Immediately cancel current dome operation.
//PUT /dome/{device_number}/closeshutter       Close the shutter or otherwise shield telescope from the sky.
//PUT /dome/{device_number}/findhome           Start operation to search for the dome home position.
//PUT /dome/{device_number}/openshutter        Open shutter or otherwise expose telescope to the sky.
//PUT /dome/{device_number}/park               Rotate dome in azimuth to park position.
//PUT /dome/{device_number}/setpark            Set the current azimuth, altitude position of dome to be the park position
//PUT /dome/{device_number}/slewtoaltitude     Slew the dome to the given altitude position.
//PUT /dome/{device_number}/slewtoazimuth      Slew the dome to the given azimuth position.
//PUT /dome/{device_number}/synctoazimuth      Synchronize the current position of the dome to the given azimuth.

void handleAltitudeGet( void);
void handleAtHomeGet( void);
void handleAtParkGet(void);
void handleAzimuthGet(void);
void handleCanFindHomeGet(void);
void handleCanParkGet(void);
void handleCanSetAltitudeGet(void);
void handleCanSetAzimuthGet(void);
void handleCanSetParkGet(void);
void handleCanSetShutterGet(void);
void handleCanSlaveGet(void);
void handleCanSyncAzimuthGet(void);
void handleShutterStatusGet(void);
void handleSlavedGet(void);
void handleSlavedPut(void);
void handleSlewingGet(void);
void handleAbortSlewPut(void);
void handleCloseShutterPut(void);
void handleFindHomePut(void);
void handleOpenShutterPut(void);
void handleParkPut(void);
void handleSetParkPut(void);
void handleSlewToAltitudePut(void);
void handleSlewToAzimuthPut(void);
void handleSyncToAzimuthPut(void);

void handleAltitudeGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "AltitudeGet", 0, "" );    


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
    THis is the sort of way we would like to behave
    */
    
    root["Value"] = altitude;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;
}

void handleAtHomeGet( void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "AtHome", 0, "" );    
    root["Value"] = atHome;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;
}

void handleAtParkGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "AtPark", 0, "" );    
    root["Value"] = atPark;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;
   
}

void handleAzimuthGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "Azimuth", 0, "" );    
    root["Value"] = azimuth;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;
}

void handleCanFindHomeGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanFindHome", 0, "" );    
    root["Value"] = canFindHome;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;
   
}

void handleCanParkGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanPark", 0, "" );    
    root["Value"] = canPark;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;
}

void handleCanSetAltitudeGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSetAltitude", 0, "" );    
    root["Value"] = canSetAltitude;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;  
}

void handleCanSetAzimuthGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSetAzimuth", 0, "" );    
    root["Value"] = canSetAzimuth;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;  
}

void handleCanSetParkGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSetPark", 0, "" );    
    root["Value"] = canSetPark;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;  
}

void handleCanSetShutterGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSetShutter", 0, "" );    
    root["Value"] = canSetShutter;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;  
}

void handleCanSlaveGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSlave", 0, "" );    
    root["Value"] = canSlave;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;  
}

void handleCanSyncAzimuthGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSyncAzimuth", 0, "" );    
    root["Value"] = canSyncAzimuth;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;  
}

//'Slaved' indicates a hardware link between the telescope and dome  - the ASCOM link is not the same. 
void handleSlavedGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "Slaved", 400, "Not Implemented" );    
    root["Value"] = slaved;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;  
}

void handleSlavedPut(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "Slaved", 400, "Not Implemented" );    
    root["Value"] = false;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;  
}

void handleSlewingGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "Slewing", 0, "" );    
    root["Value"] = ( domeStatus == DOME_SLEWING ) ? true:false;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ; 
}

void handleAbortSlewPut(void)
{
  String message;
  uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
  uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_SLEWING  )
    {
      //abort slew
      //Need to use shift not just add 
      domeCmdList.addCmd( clientID, transID, "CMD_DOME_ABORT", 0 );
      jsonResponseBuilder( root, clientID, transID, "AbortSlewing", 0, "" );
      abortFlag = true;
    }
    else
    {
      jsonResponseBuilder( root, clientID, transID, "AbortSlewing", 400, "Not slewing" );       
    }
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;   
}

void handleShutterStatusGet(void)
{
  String message;
  uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
  uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

  JsonObject& root = jsonBuffer.createObject();
  //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
  jsonResponseBuilder( root, clientID, transID, "ShutterStatus", 0, "" );
  root["Value"] = shutterStatus;

  //JsonArray& offsets = root.createNestedArray("Value");
  root.printTo(message);
  server.send(200, "text/json", message);
  return ;   
}

void handleCloseShutterPut(void)
{
  String message;
  uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
  uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

  JsonObject& root = jsonBuffer.createObject();
  //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
  if( shutterStatus == SHUTTER_OPEN ) //&& shutter open detected == !shutterclosed)
  {
    jsonResponseBuilder( root, clientID, transID, "CloseShutter", 0, "" );
    //Create new async command
    addShutterCmd( clientID, transID, CMD_SHUTTER_CLOSE, 0 ); 
  }
  else
  {
    jsonResponseBuilder( root, clientID, transID, "CloseShutter", 400, "Shutter Not open or not idle" );       
  }
  //JsonArray& offsets = root.createNestedArray("Value");
  root.printTo(message);
  server.send(200, "text/json", message);
  return ;   
}

//Kick off the find home operation ???
void handleFindHomePut(void)
{
   String message;
   uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
   uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_IDLE )
    {
      //Set command to move to home.
      //cmdItem_t* pCmd = 
      addDomeCmd( clientID, transID, CMD_DOME_HOME, 0 ); 
      jsonResponseBuilder( root, clientID, transID, "FindHome", 0, server.arg("FindHome") );
    }
    else
    {
      jsonResponseBuilder( root, clientID, transID, "FindHome", 400, "Dome not idle" );       
    }
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;   
}

void handleOpenShutterPut(void)
{
   String message;
   uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
   uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

   JsonObject& root = jsonBuffer.createObject();
   //in this driver model,each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( shutterStatus != SHUTTER_OPEN && shutterStatus != SHUTTER_OPENING )
   {
      //Set command to open shutter.
      addShutterCmd( clientID, transID, CMD_SHUTTER_OPEN, 0 );
      jsonResponseBuilder( root, clientID, transID, "OpenShutter", 0, "" );    
   }
   else
   {
      jsonResponseBuilder( root, clientID, transID, "FindHome", 400, "Dome shutter not idle or not closed" );       
   }
   root.printTo(message);
   server.send( 200, "text/json", message);
   return ;   
}

//Set park operates when the dome is at the desired park position
void handleSetParkPut(void)
{
   String message;
   uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
   uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
   int location = 0;

   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_IDLE && server.hasArg("location") )
    {
      //Set new park location.
      //...
      location = (int) server.arg("location").toInt();
      location %= 360;
      parkPosition = location;
      jsonResponseBuilder( root, clientID, transID, F("SetPark"), 0, "" );    
    }
   else
    {
      jsonResponseBuilder( root, clientID, transID, F("SetPark"), 400, F("Dome not idle") );       
    }
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;      
}

//ParkPut causes the dome to move to the already stored Park position 
void handleParkPut(void)
{
   String message;
   uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
   uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. Hence ignore device-number
   if( domeStatus == DOME_IDLE )
    {
      addDomeCmd( clientID, transID, CMD_DOME_SLEW, parkPosition );      
      jsonResponseBuilder( root, clientID, transID, F("SetPark"), 0, "" );    
    }
   else
    {
      jsonResponseBuilder( root, clientID, transID, F("Park"), 400, F("Dome not idle, command queued") );       
    }
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;      
}

void handleSlewToAltitudePut(void)
{
   String message;
   uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
   uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
   int location = 0;

   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_IDLE && server.hasArg("altitude") )
    {
      //Set new shutter altitude.
      location = server.arg("altitude").toInt();
      location %= 110;
      //cmdItem_t* pCmd = domeCmdList->add( clientID, transID, CMD_DOMEVAR_SET, location );
      targetAltitude = location;
      jsonResponseBuilder( root, clientID, transID, F("SetPark"), 0, "" );    
    }
   else
    {
      jsonResponseBuilder( root, clientID, transID, F("SetPark"), 400, F("Dome not idle") );       
    }
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;      
}

void handleSlewToAzimuthPut(void)
{
   String message;
   uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
   uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
   int iTargetAzimuth = 0;
   
   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_IDLE && server.hasArg("Azimuth") )
    {
      //Set new park location.
      //...
      iTargetAzimuth = (int) server.arg("Azimuth").toInt();
      iTargetAzimuth %= 360;
      targetAzimuth = (float) iTargetAzimuth;
      jsonResponseBuilder( root, clientID, transID, F("SlewToAzimuth"), 0, "" );    
    }
   else
    {
      jsonResponseBuilder( root, clientID, transID, F("SetPark"), 400, F("Dome not idle") );       
    }
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;
}

void handleSyncToAzimuthPut(void)
{
   String message;
   uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
   uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
   int iTargetAzimuth = 0;
   float azimuthOffset = 0.0F;
   
   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_IDLE && server.hasArg(F("Azimuth") ) )
    {
      //Set new (actual) azimuth location.
      //...
      iTargetAzimuth = (int) server.arg( F("Azimuth") ).toInt();
      iTargetAzimuth %= 360;
      ///the offset is the difference between where the dome is currently pointing and where we are told it is pointing.
      azimuthOffset = (float) ( iTargetAzimuth - currentAzimuth );
      //Tell the dome to move to the target azimuth using the updated correction
      addDomeCmd( clientID, transID, CMD_DOME_SLEW, iTargetAzimuth );
      jsonResponseBuilder( root, clientID, transID, F("SyncToAzimuth"), 0, "" );    
    }
   else
    {
      jsonResponseBuilder( root, clientID, transID, F("SyncToAzimuth"), 400, F("Dome not idle") );       
    }
    root.printTo(message);
    server.send(200, F("text/json"), message);
    return ;
}
#endif
