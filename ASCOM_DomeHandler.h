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

cmdItem_t* addDomeCmd( uint32_t clientId, uint32_t transId, enum domeCmd, int value );
cmdItem_t* addShutterCmd( uint32_t clientId, uint32_t transId, enum shutterCmd, int value );
bool hasArgIC( String& check, ESP8266WebServer& ref, bool caseSensitive );
void onShutterSlew();
void onShutterIdle();
void onDomeSlew();
void onDomeAbort();
void onDomeIdle();
int restQuery( String uri, String args, String& response, int method );
float getAzimuth( float );
void setupEncoder();
bool setupCompass( String host );
int getShutterStatus( String host );
float getBearing( String host );

void handleAltitudeGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
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
    This is the sort of way we would like to behave
    */
    
    root["Value"] = altitude;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    DEBUGSL1( message );    
    server.send(200, "text/json", message);
    return ;
}

void handleAtHomeGet( void)
{
     String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "AtHome", 0, "" );    
    root["Value"] = ( abs( currentAzimuth - homePosition ) <= acceptableAzimuthError/2 )? atHome = true : atHome = false;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    DEBUGSL1( message );
    server.send(200, "text/json", message);
    return ;
}

void handleAtParkGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "AtPark", 0, "" );    
    root["Value"] = ( abs( currentAzimuth - parkPosition) < acceptableAzimuthError/2 ) ? atPark = true : atPark = false;
    root.printTo(message);

    server.send(200, "text/json", message);
    return ;
}

void handleAzimuthGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "Azimuth", 0, "" );    
    root["Value"] = currentAzimuth;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    DEBUGSL1( message );
    server.send(200, "text/json", message);
    return ;
}

void handleCanFindHomeGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
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

    DynamicJsonBuffer jsonBuff(256);
    JsonObject& root = jsonBuff.createObject();

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
    DynamicJsonBuffer jsonBuffer(256);
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
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSetAzimuth", Success, "" );    
    root["Value"] = canSetAzimuth;
    root.printTo(message);
#ifdef DEBUG_ESP_HTTP_SERVER
DEBUG_OUTPUT.printf( "handleCanSetAzimuthGet: output:%s\n", message.c_str() );
#endif
    server.send(200, "text/json", message);
    return ;  
}

void handleCanSetParkGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSetPark", 0, "" );    
    root["Value"] = canSetPark;
    root.prettyPrintTo(message);
#ifdef DEBUG_ESP_HTTP_SERVER
DEBUG_OUTPUT.printf( "handleCanSetParkGet: output:%s\n", message.c_str() );
#endif
    server.send(200, "text/json", message);
    return ;  
}

void handleCanSetShutterGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSetShutter", Success, "" );    
    root["Value"] = canSetShutter;
    //JsonArray& offsets = root.createNestedArray("Value");
    root.printTo(message);
    DEBUGSL1( message );
    server.send(200, "text/json", message);
    return ;  
}

void handleCanSlaveGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSlave", Success, "" );    
    root["Value"] = canSlave;
    root.printTo(message);
    DEBUGSL1( message );
    server.send(200, "text/json", message);
    return ;  
}

void handleCanSyncAzimuthGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "CanSyncAzimuth", Success, "" );    
    root["Value"] = canSyncAzimuth;
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
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "Slaved", notImplemented, "Not Implemented" );    
    root["Value"] = slaved;
    root.printTo(message);
    DEBUGSL1( message );
    server.send(200, "text/json", message);
    return ;  
}

void handleSlavedPut(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();

    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "Slaved", notImplemented, "Not Implemented" );    
    root["Value"] = false;
    root.printTo(message);
    server.send(200, "text/json", message);
    return ;  
}

void handleSlewingGet(void)
{
    String message;
    uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
    uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    jsonResponseBuilder( root, clientID, transID, "Slewing", Success, "" );    
    root["Value"] = ( domeStatus == DOME_SLEWING ) ? true:false;
    root.printTo(message);
    DEBUGSL1( message );
    server.send(200, "text/json", message);
    return ; 
}

void handleAbortSlewPut(void)
{
  String message;
  uint32_t clientID = (uint32_t)server.arg("ClientID").toInt();
  uint32_t transID = (uint32_t)server.arg("ClientTransactionID").toInt();
  DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_SLEWING  )
    {
      //abort slew
      //Need to use shift not just add 
      addDomeCmd( clientID, transID, CMD_DOME_ABORT, 0 );
      jsonResponseBuilder( root, clientID, transID, "AbortSlew", Success, "" );
      abortFlag = true;
    }
    else
    {
      jsonResponseBuilder( root, clientID, transID, "AbortSlew", 400, "Not slewing" );       
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
  DynamicJsonBuffer jsonBuffer(256);
  JsonObject& root = jsonBuffer.createObject();
  //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
  jsonResponseBuilder( root, clientID, transID, "ShutterStatus", 0, "" );
  root["Value"] = shutterStatus;
//0 = Open, 1 = Closed, 2 = Opening, 3 = Closing, 4 = Shutter status error
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
  DynamicJsonBuffer jsonBuffer(256);
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
   DynamicJsonBuffer jsonBuffer(256);
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
   DynamicJsonBuffer jsonBuffer(256);
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
      jsonResponseBuilder( root, clientID, transID, "OpenShutter", 400, "Dome shutter not idle or not closed" );       
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
   String argToSearchFor = "location";
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_IDLE && hasArgIC( argToSearchFor, server, false) )
    {
      //Set new park location.
      //...
      location = (int) server.arg( argToSearchFor ).toInt();
      location %= 360;
      parkPosition = location;
      jsonResponseBuilder( root, clientID, transID, F("SetPark"), 0, "" );    
    }
   else
    {
      jsonResponseBuilder( root, clientID, transID, F("SetPark"), valueNotSet, F("Dome not idle") );       
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
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();

   //in this driver model, each device has a separate ip address ,so can only be one device. Hence ignore device-number
   addDomeCmd( clientID, transID, CMD_DOME_SLEW, parkPosition );      
   jsonResponseBuilder( root, clientID, transID, F("Park"), 0, "" );    
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
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   String argToSearchFor = "altitude";
   if( domeStatus == DOME_IDLE && hasArgIC( argToSearchFor, server, false) )
    {
      //Set new shutter altitude.
      location = server.arg( argToSearchFor ).toInt();
      location %= 110;
      addShutterCmd( clientId, transId, CMD_SHUTTER_OPEN, targetAltitude );

      //cmdItem_t* pCmd = domeCmdList->add( clientID, transID, CMD_DOMEVAR_SET, location );
      targetAltitude = location;
      jsonResponseBuilder( root, clientID, transID, F("SlewToAzimuth"), 0, "" );    
    }
   else
    {
      jsonResponseBuilder( root, clientID, transID, F("SlewToAzimuth"), valueNotSet, F("Dome not idle") );       
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
   String argToSearchFor = "Azimuth";
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_IDLE && hasArgIC( argToSearchFor, server, false ) )
    {
      //Set new park location.
      //...
      iTargetAzimuth = (int) server.arg( argToSearchFor ).toInt();
      iTargetAzimuth %= 360;
      iTargetAzimuth += 360;
      iTargetAzimuth %= 360;
      targetAzimuth = (float) iTargetAzimuth;
      addDomeCmd( clientID, transID, CMD_DOME_SLEW, parkPosition );      

      jsonResponseBuilder( root, clientID, transID, F("SlewToAzimuth"), 0, "" );    
    }
   else
    {
      jsonResponseBuilder( root, clientID, transID, "SlewToAzimuth", valueNotSet, "Dome not idle" );       
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
   String argToSearchFor = "Azimuth";
   DynamicJsonBuffer jsonBuffer(256);
   JsonObject& root = jsonBuffer.createObject();
   //in this driver model, each device has a separate ip address ,so can only be one device. hence ignore device-number
   if( domeStatus == DOME_IDLE && hasArgIC( argToSearchFor, server, false ) )
   {
      //Set new (actual) azimuth location.
      //...
      iTargetAzimuth = (int) server.arg( argToSearchFor ).toInt();
      iTargetAzimuth %= 360;
      iTargetAzimuth += 360;
      iTargetAzimuth %= 360;
            
      ///the offset is the difference between where the dome is currently pointing and where we are told it is pointing.
      azimuthSyncOffset = (float) ( iTargetAzimuth - currentAzimuth );
      //Tell the dome to move to the target azimuth using the updated correction
      addDomeCmd( clientID, transID, CMD_DOME_SLEW, iTargetAzimuth );
      jsonResponseBuilder( root, clientID, transID, F("SyncToAzimuth"), 0, "" );    
   }
   else
   {
      jsonResponseBuilder( root, clientID, transID, "SyncToAzimuth", valueNotSet, "Dome not idle" );       
    }
    root.printTo(message);
    server.send(200, F("text/json"), message);
    return ;
}

  /* Handle the command list - we've already checked we are idle at this point.
   */
  void onDomeIdle( void )
  {
    cmdItem_t* pCmd = nullptr;
    
    if ( domeCmdList->size() > 0 ) 
    {
      pCmd = domeCmdList->pop();
  
      switch ( pCmd->cmd )
      {
        case CMD_DOME_SLEW:
          domeStatus = DOME_SLEWING;
          onDomeSlew();
          break;
        case CMD_DOME_ABORT:
          onDomeAbort();
          break;
        case CMD_DOMEVAR_SET:
          //Did this so that dome variables aren't updated while waiting for commands that preceeded them to execute.    
          default:
        break;
      }
    }  
    return;
  }

  /* 
   *  Not to query rotating shutter but to include offsets in returned result. 
   */  
  float getAzimuth( float value )
  {
    value = ( value + azimuthSyncOffset );
    if ( value < 0.0F ) 
    {
      value += 360.0;      
    }
    else if (value > 360.0F )
      value -= 360.0F;

    return value;
  }
  
  /*
   * Handle dome rotation slew speeds and directions 
   * slow to crawl once near target azimuth
   * Calc minimum distance to select direction
   */
  void onDomeSlew( void )
  {
    int distance = 0;
    enum motorSpeed speed = MOTOR_SPEED_OFF;
    enum motorDirection direction = MOTOR_DIRN_CW;
    
    int localAzimuth = 0; 
    
    if( domeStatus != DOME_SLEWING )
      return;

    localAzimuth = getAzimuth( bearing );
    distance = targetAzimuth - localAzimuth ;
    distance %= 360;
    
    //Work out speed to turn dome
    if( abs(distance) < acceptableAzimuthError )
    {
      //Stop!!
      //turn off motor
      myMotor.setSpeedDirection( speed = MOTOR_SPEED_OFF, direction );
      domeStatus = DOME_IDLE;
      if( abs( localAzimuth - homePosition ) < acceptableAzimuthError )
        atHome = true;
      DEBUGSL1( "Dome stopped at target");
      //myLCD.write( 2, 0, "Slew complete." );
    }
    else if ( abs(distance < slowAzimuthRange ) )   
    {
      speed = MOTOR_SPEED_SLOW_SLEW;
      DEBUGS1( "Dome target distance: ");
      DEBUGSL1( distance );
      DEBUGSL1( "Slew::Slow");
      //myLCD.write( 2, 0, "Slew::Slow" );      
    }
    else
    {
      speed = MOTOR_SPEED_FAST_SLEW;
      DEBUGS1( "Dome target distance: ");
      DEBUGSL1( distance );
      DEBUGSL1( "Dome speed set to fast slew");    
      //myLCD.write( 2, 0, "Slew::Fast" );
    }
    
    //Work out the direction. 
    direction = ( distance > 0 ) ? MOTOR_DIRN_CW: MOTOR_DIRN_CCW;
    if ( abs( distance ) > 180 )
    {
      DEBUGS1( "Dome target distance: ");
      DEBUGSL1( distance );

      //Swap direction and go the short route. 
      if( direction == MOTOR_DIRN_CW)
      {
         direction = MOTOR_DIRN_CCW;
          DEBUGSL1( "Dome direction reversed to CCW");         
      }
      else
      {
        direction = MOTOR_DIRN_CW;
          DEBUGSL1( "Dome direction reversed to CW");               
      }
    }
        
    myMotor.setSpeedDirection( speed, direction );
    return;
   }
  
 void updateCmdResponseList( int shutterStatus )
 {
    //TODO - check list for status enum and update list to show complete
    //Idea is that if we have a client come back to check, we answer. 
    //In practise client polls status so may drop this. 
    return;    
 }
  
  /*
   * Handler for domeAbort command
   */ 
  void onDomeAbort( void )
  {
    //clear down command list
    domeCmdList->clear();//TODO memory error- needs to delete all mallocs properly.
    //turn off motor
    myMotor.setSpeedDirection( MOTOR_SPEED_OFF, MOTOR_DIRN_CW );
    //message to LCD  
    myLCD.writeLCD( 2, 0, "ABORT received - dome halted." );
    // ? close shutter ?
    return;
  }

void onShutterIdle()
{
  String response = "";
  String uri = "http://";

  //Get next command if there is one. 
  cmdItem_t* pCmd = nullptr;
  if ( shutterCmdList->size() > 0 ) 
  {
    pCmd = shutterCmdList->pop();
  
    uri.concat( sensorHostname);
    uri.concat("/shutter");
    
    switch ( pCmd->cmd )
    {
       case CMD_SHUTTER_OPEN: 
       case CMD_SHUTTER_CLOSE: 
          onShutterSlew();
          break;
       case CMD_SHUTTER_ABORT:
          restQuery( uri, "status=abort", response, HTTP_PUT);
          myLCD.writeLCD( 2, 0, "DSH ABORT" );
          break;
       case CMD_SHUTTERVAR_SET:
       default:
        break;
    }
  }
}
  
  /* Function to hande shutter slew command. 
     Uses rest aPi to talk to shutter controller on rotating dome section
     Could instead be the roll off roof controller but you might build that into this, non-rotating controller instead.  
   */
  void onShutterSlew()
  {
    //check shutter state
    int currentShutterStatus = SHUTTER_ERROR;
    String outbuf;
    String uri = "http://";
    int response = 0;
    DynamicJsonBuffer jsonBuff(256);

    uri.concat( shutterHostname );
    uri.concat( "/shutter" );
    response = restQuery( uri, "" , outbuf, HTTP_GET );
    JsonObject& root = jsonBuff.parse( outbuf );
    
    if ( response == 200 && root.success() && root.containsKey( "status" ) )
      currentShutterStatus = root["status"];

    switch ( currentShutterStatus )
    {
      case SHUTTER_OPENING:
      if ( currentShutterStatus == SHUTTER_OPEN  )
      {
        shutterStatus = SHUTTER_OPEN;
        //Should now update command response list...
        updateCmdResponseList( shutterStatus );
      }
        break;
      case SHUTTER_CLOSING:
      if ( currentShutterStatus == SHUTTER_CLOSED  )
      {
        shutterStatus = SHUTTER_CLOSED;
        //Should now update command response list...
        updateCmdResponseList( shutterStatus );
      }
        break;
      
      // do nothing for these - they are static, unless they have unexpectedly changed
      case SHUTTER_OPEN:
      if( currentShutterStatus != SHUTTER_OPEN )
        shutterStatus = SHUTTER_ERROR;
        //What are we going to do about it ?
      case SHUTTER_CLOSED:
      if( currentShutterStatus != SHUTTER_CLOSED )
      {
        shutterStatus = SHUTTER_ERROR;
        //Should now update command response list...
        updateCmdResponseList( shutterStatus );        
      }
      case SHUTTER_ERROR:  //bad state
        //What are we going to do about it ?
        //Alert
        //Send email 
        //etc
        break;
      default:
        break;
    }  
  }

  void onShutterAbort( void )
  {
    String outbuf = "";
    String uri = "http:";
    shutterCmdList->clear();
    uri.concat( sensorHostname );
    uri.concat("/shutter");
    if( restQuery( uri, "status=abort", outbuf ,HTTP_PUT) != HTTP_CODE_OK)
    {
      //what to do ?
    }
    // updateCmdResponseList( shutterStatus );        
  } 
  
  void setupEncoder()
  {
    //TODO 
    return;
  }
 
  //Detect compass service and initialise azimuth bearing of dome
  bool setupCompass( String targetHost )
  {
    bool status = false;
    String compassURL = "http://";
    String output = "";
    int response = 0;
    float value;
    DynamicJsonBuffer jsonBuff(256);
    
    compassURL.concat( sensorHostname );
    compassURL.concat( "/bearing" );
    response = restQuery( compassURL, "", output, HTTP_GET );
    DEBUG_HTTPCLIENT( "[HTTPClient response ]  rsponse code: %i, response: %s\n", response, output.c_str() );
    JsonObject& root = jsonBuff.parse( output );
    
    if ( response == HTTP_CODE_OK && root.success() && root.containsKey("bearing") )
    {
      value = (float) root.get<float>("bearing"); //initialise current setting
      status = true;
    }
    else
    {
      Serial.print( "Shutter compass bearing call not successful, response: " );    Serial.println( response );
      Serial.print( "JSON parsing status: " );    Serial.println( root.success() );
    }
    return status;
  }

  /*
   * Query a url for a parsed json output
   * check return code with object.success();
   */ 
  int restQuery( String uri, String args, String& response, int method )
  {
    int httpCode = 0;
    long int startTime;
    long int endTime;
    HTTPClient hClient;
    hClient.setTimeout ( (uint16_t) 250 );    
    hClient.setReuse( false );    
    
    DEBUG_HTTPCLIENT("restQuery request [%i] uri: %s args:%s\n", method, uri.c_str(), args.c_str() );
    startTime = millis();
    if ( hClient.begin( wClient, uri ) ) //uri must already have request args in it
    {
      if( method == HTTP_GET )
      {
        httpCode = hClient.GET();
        endTime = millis();
        DEBUG_HTTPCLIENT( "Time for restQuery call(mS): %li\n", endTime-startTime );
      }
      else if ( method == HTTP_PUT ) //variables are added as headers
      {
        httpCode = hClient.PUT( args );        
        endTime = millis();
        DEBUG_HTTPCLIENT("Time for restQuery call: %li mS\n", endTime-startTime );
      }
     
      // file found at server ?
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
      {
        response = hClient.getString();
        DEBUG_HTTPCLIENT("Response: %s\n", response.c_str() );
      }
      else 
      {
        DEBUG_HTTPCLIENT("restQuery ... failed, error: %s\n", hClient.errorToString(httpCode).c_str() );
        ;;
      } 
      hClient.end();
    }
    else
    {
       DEBUG_HTTPCLIENT("restQuery Unable to open connection");
    }  
    return httpCode;
  }
  
  /*
   * Function to determine bearing of dome - just return a sane value to the caller. Let the caller handler sync offsets. 
   */
  float getBearing( String host)
  {
    static int bearingRepeatCount = 0;
    static float lastBearing = 0.0F;
    const int bearingRepeatLimit = 10;
    float bearing = 0.0F;
    int response = 0;
    String outbuf = "";
    DynamicJsonBuffer jsonBuff(256);    
    
    //Update the bearing
    String uri = "http://";
    uri.concat( host );
    uri.concat( "/bearing" );    
    response = restQuery( uri, "", outbuf, HTTP_GET );
    DEBUG_HTTPCLIENT("GetBearing response code: %i, response: %s\n", response, outbuf.c_str() );
    JsonObject& root = jsonBuff.parse( outbuf );
    
    if ( response == HTTP_CODE_OK && root.success() && root.containsKey( "bearing" ) )
    {
      DEBUG_HTTPCLIENT(" GetBearing: successful: %f \n", bearing );
      bearing = (float) root.get<float>("bearing");
      if( bearing >= 0.0F && bearing <= 360.0F )
        currentAzimuth = bearing;
      if ( lastBearing == bearing ) //We do this to check for sensor freeze
      {
        bearingRepeatCount ++;
        DEBUG_HTTPCLIENT(" GetBearing: Sensor stuck %i detected @ %f\n", bearingRepeatCount, bearing );
        
        //reset the compass by resetting the target device
        if ( bearingRepeatCount > bearingRepeatLimit ) 
        {  
          uri = "http://";
          uri.concat( sensorHostname );
          uri.concat( "/bearing/reset" );
          
          response = restQuery( uri, "", outbuf, HTTP_PUT );
          DEBUG_HTTPCLIENT(" GetBearing: Compass reset attempted !");
          myLCD.writeLCD( 2, 0, "Compass reset !");
          bearingRepeatCount = 0;
        }
      }
      else //good value not stuck 
      {
        DEBUG_HTTPCLIENT(" GetBearing: Reading obtained %f\n", bearing );
        lastBearing = bearing;
        bearingRepeatCount = 0;
      }
    }
    else //can't retrieve the bearing
    {
      //hand back the current bearing, so we don;t slew off into strange territory when we return 0.0f instead. 
      DEBUG_HTTPCLIENT("GetBearing: no reading, using last  ");
      bearing = currentAzimuth - azimuthSyncOffset;
    }
    return bearing;
  }
  
  /*
   * Rest call to retrieve shutter Status. 
   */ 
  int getShutterStatus( String host )
  {
    String outbuf;
    int value = 0;
    DynamicJsonBuffer jsonBuff(256);

    String uri = String( "http://" );
    uri.concat( host );
    uri.concat( "/shutter" );
    int response = restQuery( uri , "", outbuf, HTTP_GET  );
    JsonObject& root = jsonBuff.parse( outbuf ); 
    if ( response == 200 && root.success() && root.containsKey("status") )
    {
      value = ( enum shutterState) root.get<int>("status");    
    }
    else
    {
      value = SHUTTER_ERROR;
      DEBUG_HTTPCLIENT("Shutter controller call not successful, response: %i\n", response );
      DEBUG_HTTPCLIENT("Shutter JSON parsing status: %i\n", root.success() );
    }
  return value;
  }
#endif
