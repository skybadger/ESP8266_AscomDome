/*
File to be included into relevant device REST setup 
*/
//Assumes Use of ARDUINO ESP8266WebServer for entry handlers
#if !defined _ESP8266_DomeCmds_h_ 
#define _ESP8266_DomeCmds_h_
#include "AlpacaErrorConsts.h"
#include "ESP8266_AscomDome.h"
#include "JSONHelperFunctions.h"
#include <LinkedList.h>

 //prototypes
 float normaliseFloat( float& input, float radix);
 int normaliseInt( int& input, int radix ); 
 
 //interfaces to other inputs. 
 int restQuery( String uri, String args, String& response, int method );
 float getAzimuth( float );
 void setupEncoder();
 bool setupCompass( String host );
 float getBearing( String host );
 int getShutterStatus( String host );
  
 //Status Event processing
 cmdItem_t* addDomeCmd( uint32_t clientId, uint32_t transId, String cmdName, enum domeCmd, int value );
 cmdItem_t* addShutterCmd( uint32_t clientId, uint32_t transId, String cmdName, enum shutterCmd, int value );
 void updateCmdResponseList( int transID );
 
 //dome state handlers
 void onDomeSlew();
 void onDomeAbort();
 void onDomeIdle();
 
 //Shutter state handlers
 void onShutterIdle();
 void shutterAltitude( int newAngle );
 void shutterSlew( enum shutterCmd setting );
 void shutterAbort( void );

  float normaliseFloat( float& input, float radix) 
  {
    float temp = input;
    float output = 0.0F;
    output = fmod( input, radix ) ;
    while( output < 0.0F ) //some compilers allow negative values with mod
    {
      output += radix;
    }
    debugV( "NormaliseFloat: in: %f, out: %f", temp, output );
    input = output;
    return output;
  }
  
  int normaliseInt( int& input, int radix )
  {
    int temp = input;
    int output = 0;
    output = input % radix;
    while( output < 0 ) //some compilers allow negative values with mod
    {
      output += radix;
    }
    debugV( "NormaliseInt: in: %i, out: %i", temp, output );
    input = output;
    return output;  
  }

  
  cmdItem_t* addDomeCmd( uint32_t clientId, uint32_t transId, String cmdName, enum domeCmd newCmd, int value )
  {
      //Create new command
      cmdItem_t* pCmd = (cmdItem_t*) calloc( sizeof( cmdItem_t ), 1); 
      //Add to list 
      pCmd->cmdName = cmdName;
      pCmd->cmd = (int) newCmd;
      pCmd->value = value;
      pCmd->clientId = clientId;
      pCmd->transId = transId; 
      domeCmdList->add( pCmd );
      
      debugI( "Cmd pushed: %i", newCmd ); 
      return pCmd;
  }

  cmdItem_t* addShutterCmd( uint32_t clientId, uint32_t transId, String cmdName, enum shutterCmd newCmd, int value )
  {
      //Create new command
      cmdItem_t* pCmd = (cmdItem_t*) calloc( sizeof( cmdItem_t ), 1); 
      //Add to list 
      pCmd->cmdName = cmdName;
      pCmd->cmd = (int) newCmd;
      pCmd->value = value;
      pCmd->clientId = clientId;
      pCmd->transId = transId; 
      shutterCmdList->add( pCmd );

      debugI( "Cmd pushed: %i", newCmd ); 
      return pCmd;
  }

  void freeCmd( cmdItem_t* ptr) 
  {
    ptr->cmdName = ""; //was a String - does it need setting to a null ptr to destroy without loss ? 
    free( ptr );
  }
  
  /* Handle the command list - we've already checked we are idle at this point.
   */
  void onDomeIdle( void )
  {
    cmdItem_t* pCmd = nullptr;
    
    enum domeCmd newCmd; 
    if ( domeCmdList->size() > 0 ) 
    {
      pCmd = domeCmdList->pop();
      debugI( "Popped new command: %i, value: %i", pCmd->cmd, pCmd->value );
      newCmd = (enum domeCmd ) pCmd->cmd;
      switch ( newCmd )
      {
        case CMD_DOME_SLEW:
          targetAzimuth = (float) pCmd->value;
          domeStatus = DOME_SLEWING;
          onDomeSlew();
          break;
        case CMD_DOME_ABORT:
          onDomeAbort();
          break;
        case CMD_DOMEVAR_SET:
          //Did this so that dome variables aren't updated while waiting for commands that preceeded them to execute.   
          if ( pCmd->cmdName == nullptr )
          {
            debugW( "DOME_VAR_SET nullptr command name");
          }
          else if ( pCmd->cmdName == "" )
          {
            debugW( "DOME_VAR_SET empty command name");            
          }
          else if( pCmd->cmdName.equalsIgnoreCase( "parkPosition") )
          {
              parkPosition = (float) pCmd->value;
              saveToEeprom();
          }
          else if ( pCmd->cmdName.equalsIgnoreCase( "homePosition") )
          {
              homePosition = (float) pCmd->value;
              saveToEeprom();
          }
          else if ( pCmd->cmdName.equalsIgnoreCase( "azimuthSyncOffset") )
          {
              azimuthSyncOffset = (float) pCmd->value;
              saveToEeprom();
          }
          domeStatus = DOME_IDLE;
          break;
        default:
          domeStatus = DOME_IDLE;
          break;
      }
    freeCmd( pCmd );
    }  
    return;
  }

  /* 
   *  Not to query rotating shutter but to include offsets in returned result. 
   */  
  float getAzimuth( float value )
  {
    value = ( value + azimuthSyncOffset );
    normaliseFloat( value, 360.0F );
    return value;
  }
  
  /*
   * Handle dome rotation slew speeds and directions 
   * Slow to crawl once near target azimuth
   * Calc minimum distance to select direction
   */
  void onDomeSlew( void )
  {
    float distance = 0;
    enum motorSpeed speed = MOTOR_SPEED_OFF;
    enum motorDirection direction = MOTOR_DIRN_CW; 
    float localAzimuth = 0.0F; 
      
    if( domeStatus != DOME_SLEWING )
      return;

    localAzimuth = getAzimuth( bearing );
    distance = targetAzimuth - localAzimuth ;
    debugD( "OnDomeSlew: current: %f, target: %f", localAzimuth, targetAzimuth );    

    //Whatever the state. 
    if( abs( localAzimuth - homePosition ) < acceptableAzimuthError )
      atHome = true;
    if( abs( localAzimuth - parkPosition ) < acceptableAzimuthError )
      atPark = true;

    //Work out speed to turn dome
    if( abs(distance) < acceptableAzimuthError )
    {
      //Stop!!
      //turn off motor
      myMotor.setSpeedDirection( speed = MOTOR_SPEED_OFF, direction );

      //Update the desired state to say we have finished.
      domeStatus = DOME_IDLE;

      debugI( "Dome stopped at target: %03f", localAzimuth);
      if ( lcdPresent )
        myLCD.writeLCD( 2, 0, "Slew complete." );

      //Update MQTT to record last known position
      publishFnStatus();
    }
    else if ( abs(distance) < slowAzimuthRange )
    {
      speed = MOTOR_SPEED_SLOW_SLEW;
      debugD( "Dome speed reduced - distance: %f", (float) distance);
      if( lcdPresent )
        myLCD.writeLCD( 2, 0, "Slew::Slow" );      
    }
    else
    {
      speed = MOTOR_SPEED_FAST_SLEW;
      debugD( "Dome speed fast - distance: %f", distance );    
      if ( lcdPresent ) 
        myLCD.writeLCD( 2, 0, "Slew::Fast" );
    }
    
    //Work out the direction. 
    direction = ( distance >= 0.0F ) ? MOTOR_DIRN_CW: MOTOR_DIRN_CCW;
    debugD( "Dome target distance & dirn: %f, %i", distance, direction );         

    if ( abs(distance) > 180.0F )
    {
      //Swap direction and go the short route. 
      direction = ( direction == MOTOR_DIRN_CW )? MOTOR_DIRN_CCW:MOTOR_DIRN_CW; 
      debugD( "Dome direction reversed to %i", direction );         
    }
      
    myMotor.setSpeedDirection( speed, direction );
    myMotor.getSpeedDirection();
    debugV("Motor returned - speed %u, direction: %u", ( uint8_t) myMotor.getSpeed(), (uint8_t) myMotor.getDirection() );
    return;
   }
  
 void updateCmdResponseList( int transID )
 {
    //TODO - Client presents transaction Id and we can check list for status enum and update list to show complete
    //Idea is that if we have a client come back to check, we answer. 
    //In practise client polls status so may drop this. 
    return;    
 }
  
  /*
   * Handler for domeAbort command
   * if an abort is called, this function clears down the async command stack
   * and puts the dome in a safe mode of not slewing and closed shutter. 
   * closed shutter seems fairly extreme though. 
   * @param void  
   * @return void 
   */ 
  void onDomeAbort( void )
  {
    /*clear down command list
    cmdItem_t* ptr = nullptr;
    while( domeCmdList->size() > 0 ) 
    {
      ptr = domeCmdList->pop();
      freeCmd(ptr);
    }
    */

    //turn off motor
    if( motorPresent )
      myMotor.setSpeedDirection( MOTOR_SPEED_OFF, MOTOR_DIRN_CW );
    debugW("Aborting- motor set to stop");
    
    //message to LCD  
    if( lcdPresent ) 
      myLCD.writeLCD( 2, 0, "ABORT - dome halted." );
    
    // ? close shutter ?
    //TODO add call to halt shutter actions here. 
    
    //Update status to idle to process normally.
    domeStatus = DOME_IDLE;
    publishFnStatus();
    return;
  }

//Function to issue commands to shutter when idle. Otherwise wait until state is open or closed. 
void onShutterIdle()
{
  String response = "";
  String uri = "http://";
//enum shutterCmd              { CMD_SHUTTER_ABORT=0, CMD_SHUTTER_OPEN=4, CMD_SHUTTER_CLOSE=5, CMD_SHUTTERVAR_SET };
  enum shutterCmd newCmd = CMD_SHUTTER_ABORT;
  cmdItem_t* pCmd = nullptr;
  
  switch ( shutterStatus )
  {
     case SHUTTER_ERROR:
     case SHUTTER_OPEN:
     case SHUTTER_CLOSED:
        //Get next command if there is one. 
        if ( shutterCmdList->size() > 0 ) 
        {
          pCmd = shutterCmdList->pop();
          newCmd = (enum shutterCmd) pCmd->cmd;
          debugI("OnShutterIdle - new command popped: %i", newCmd );  

          switch( newCmd )
          {
            case CMD_SHUTTER_ABORT: 
            case CMD_SHUTTER_OPEN:  
            case CMD_SHUTTER_CLOSE:
                shutterSlew( newCmd );
                updateCmdResponseList( pCmd->transId );            
                break;
            case CMD_SHUTTERVAR_SET:
                if( pCmd->cmdName.equalsIgnoreCase( "altitude" ) && (pCmd->value >= SHUTTER_MIN_ALTITUDE ) && ( pCmd->value <= SHUTTER_MAX_ALTITUDE ) )
                { //If its at either end then that is open or closed, not an altitude
                  if ( ( pCmd->value > SHUTTER_MIN_ALTITUDE ) && ( pCmd->value < SHUTTER_MAX_ALTITUDE) )  
                    shutterAltitude( pCmd->value );
                }
                updateCmdResponseList( pCmd->transId );                
                break;
            default:
              break;
          }
          debugI("State %s outcome for shutter ", shutterStateNames[ shutterStatus ] );
          free( pCmd ) ;
       }  
       break;
     
     //Wait for an idle state otherwise
     case SHUTTER_OPENING:
     case SHUTTER_CLOSING:
       debugI("Shutter still opening or closing - waiting for idle to check for new state" );
       break;
     default: 
       debugW("Unknown state %i requested for shutter ", targetShutterStatus );
       break;         
  }
}

void shutterSlew( enum shutterCmd setting )
{
    String outbuf = "";
    String uri = "http://";
    String arg = "shutter=";
    
    uri.concat( shutterHostname );
    uri.concat("/shutter");

    //debugD( "shutterSlew: setting: %s, %i", setting, setting.length() );
    switch( setting )
    {
      case CMD_SHUTTER_ABORT: arg.concat( "abort" );
        break;
      case CMD_SHUTTER_OPEN:  arg.concat( "open" );
        break;
      case CMD_SHUTTER_CLOSE: arg.concat( "close" );
        break;
      default: 
        debugE( "invalid shutterState provided to shutterSlew - ignoring" );
      break;
    }
    debugD( "calling shutter with args: %s", arg.c_str() );
      
    if( restQuery( uri, arg, outbuf, HTTP_PUT ) == HTTP_CODE_OK )
    {
      debugD("Successfully issued new state %s to shutter", arg.c_str() );
      if ( setting == CMD_SHUTTER_OPEN )
        shutterStatus = SHUTTER_OPENING;
      else if ( setting == CMD_SHUTTER_CLOSE )
        shutterStatus = SHUTTER_CLOSING;
      else
        shutterStatus = SHUTTER_ERROR;
      //Let normal status request update our state. 
    }
    else 
    {
      //what to do ?
      debugW("Failed to send new state Cmd to shutter");
      shutterStatus  = SHUTTER_ERROR;
    }
}

void shutterAltitude( int newAngle )
{
    String outbuf = "";
    String uri = "http:";
    String arg = "altitude=";
    
    uri.concat( shutterHostname );
    uri.concat("/shutter");
    arg.concat( newAngle );
        
    debugD(" Setting up to send new altitude to shutter");
    int response = restQuery( uri, arg, outbuf, HTTP_PUT);
    if( response  != HTTP_CODE_OK)
    {
      //what to do ?
      debugE("Failed to send new altitude to shutter, return HTTP code %i", response );
      //Preserve existing state - no change
      shutterStatus = SHUTTER_ERROR;
    }
    else 
    {
      debugD("Issued new altitude %i to shutter, currently at %i", newAngle, altitude );
      if( newAngle > altitude && newAngle <= SHUTTER_MAX_ALTITUDE )     
        shutterStatus = SHUTTER_OPENING;
      else if ( newAngle < altitude && ( newAngle >= SHUTTER_MIN_ALTITUDE ) ) 
        shutterStatus = SHUTTER_CLOSING;
      else if ( newAngle == altitude && ( newAngle > SHUTTER_MIN_ALTITUDE ) && ( newAngle <= SHUTTER_MAX_ALTITUDE ) )
        shutterStatus = SHUTTER_OPEN;
      else 
        shutterStatus = SHUTTER_CLOSED;
    }
}
  
  //Stop shutter moving. 
  void shutterAbort( void )
  {
    int response = 200;
    String outbuf = "";
    String uri = "http:";
    shutterCmdList->clear();
    uri.concat( sensorHostname );
    uri.concat("/shutter");

    response = restQuery( uri, "{\"status\":\"abort\"}", outbuf ,HTTP_PUT);
    if( response != HTTP_CODE_OK)
    {
      //what to do ?
      debugE("Failed to issue Abort to shutter, return HTTP response %i", response );
      shutterStatus = SHUTTER_ERROR;
    }
    else 
    {
      debugI("Abort issued to shutter");
      shutterStatus = SHUTTER_OPEN;
    }
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
#if defined DEBUG_ESP_HTTP_CLIENT      
    debugD( "[HTTPClient response ] response code: %i, response: %s", response, output.c_str() );
#endif    
    JsonObject& root = jsonBuff.parse( output );
    
    if ( response == HTTP_CODE_OK && root.success() && root.containsKey("bearing") )
    {
      value = (float) root.get<float>("bearing"); //initialise current setting
      status = true;
    }
    else
    {
      debugE( "Shutter compass bearing call not successful, response: %i", response );
#if defined DEBUG_ESP_HTTP_CLIENT      
      debugV( "bearing json content: %s", output.c_str() );
#endif      
      debugE( "JSON parsing status: %i", root.success() );
    }
    return status;
  }

  /*
   * Query a url for a parsed json output
   * check return code with object.success();
   */ 
  int restQuery( String host, String args, String& response, int method )
  {
    int httpCode = 0;
    long int startTime;
    long int endTime;
    //HTTPClient hClient;
    hClient.setTimeout ( (uint16_t) 250 );    
    hClient.setReuse( true );    
    
    debugD("restQuery request - uri:[ %s ], args:[ %s ], method: %i", host.c_str(), args.c_str(), method );
    startTime = millis();
    //host and args are separate, need to join them for a GET parameterised request.    
    //if ( hClient.begin( wClient, uri ) ) uri must already have request args in it
    if ( hClient.begin( wClient, host) ) 
    {
      if( method == HTTP_GET )
      {
        httpCode = hClient.GET();
        endTime = millis();
#if defined DEBUG_ESP_HTTP_CLIENT            
        debugV( "Time for restQuery GET call(mS): %li\n", endTime-startTime );
#endif        
      }
      else if ( method == HTTP_PUT ) //variables are added as headers
      {
        //hClient.addHeader("Content-Type", "application/json"); 
        hClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
        httpCode = hClient.PUT( args.c_str() );        
        endTime = millis();
#if defined DEBUG_ESP_HTTP_CLIENT      
        debugV("Time for restQuery PUT call: %li mS\n", endTime-startTime );
#endif        
      }
     
      // file found at server ?
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
      {
        response = hClient.getString();
#if defined DEBUG_ESP_HTTP_CLIENT      
        debugV("HTTP rest query response : %s\n", response.c_str() );
#endif        
      }
      else 
      {
#if defined DEBUG_ESP_HTTP_CLIENT      
        debugV("restQuery ... failed, error: %s\n", hClient.errorToString(httpCode).c_str() );
#endif        
        ;;
      } 
      hClient.end();
    }
    else
    {
       debugE("restQuery Unable to open connection");
    }  
    return httpCode;
  }
  
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  /*
   * Function to determine bearing of dome - just return a sane value to the caller. Let the caller handler sync offsets. 
   * Note that if we don this right we can point this at any device that returns a 'bearing' in the rest response as long as the url is
   * setup correctly in the setup pages. 
   * That means the 'host' string is no longer strictly just a hostname. 
   */
  float getBearing( String host)
  {
    static int bearingRepeatCount = 0;
    const int bearingRepeatLimit = 10;
    static float lastBearing = bearing;
    float localBearing = 0.0F;
    int response = 0;
    String outbuf = "";
    String path = "";
    DynamicJsonBuffer jsonBuff(256);    
    
    //Update the bearing
    path = String( "http://" );
    path += host;
    path += "/bearing";
#if defined DEBUG_ESP_HTTP_CLIENT      
    debugV("GetBearing using remote device - host path: %s \n", path.c_str() );
    debugV("GetBearing setup - host uri: %s \n", host.c_str() );
#endif    
    response = restQuery( path, "", outbuf, HTTP_GET );
#if defined DEBUG_ESP_HTTP_CLIENT      
    debugD("GetBearing response - code: %i, output: %s", response, outbuf.c_str() );
#endif
    
    //Sometimes we get a good HTTP code but still no body... 
    JsonObject& root = jsonBuff.parse( outbuf );
    if ( response == HTTP_CODE_OK && root.success() && root.containsKey( "bearing" ) )
    {            
        localBearing = (float) root["bearing"];
        lastBearing = localBearing;
#if defined DEBUG_ESP_HTTP_CLIENT      
        debugD(" GetBearing: localBearing %f", localBearing );     
#endif        

#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION        
      //this isnt the right thing to do if its due to not getting a rest response in time from a busy device. 
      /*
      if ( lastBearing == bearing ) //We do this to check for sensor freeze
      {
        bearingRepeatCount ++;
        debugW(" GetBearing: Sensor stuck %i detected @ %f\n", bearingRepeatCount, bearing );
        
        //reset the compass by resetting the target device
        if ( bearingRepeatCount > bearingRepeatLimit ) 
        {  
          uri = "http://";
          uri.concat( host );
          uri.concat( "/reset" );
          
          response = restQuery( uri, "", outbuf, HTTP_PUT );
          debugW(" GetBearing: Compass reset attempted !");
          if (myLCD.present ) 
            myLCD.writeLCD( 2, 0, "Compass reset !");
          bearingRepeatCount = 0;
        }
      }
      else //good value not stuck 
      */
      {
#if defined DEBUG_ESP_HTTP_CLIENT      
        debugV(" GetBearing: Reading obtained %f\n", localBearing );
#endif       
        bearingRepeatCount = 0;
      }
#endif      
    }
    else //can't retrieve the bearing
    {
#if defined DEBUG_ESP_HTTP_CLIENT      
      debugV( "GetBearing: response code: %i, parse success: %i, json data: %s", response, root.success(), outbuf.c_str() );
#endif      
      debugW("GetBearing: no reading, using last  ");
      localBearing = lastBearing;
    }
    return localBearing;
  }
#else //if local interface in use
   //write something for the local case of local hardware encoder interface
 float getBearing( )
 {
   int localPosition = 0;
   localPosition = myEnc.read();
   if( localPosition != lastPosition )
   {
      if( localPosition < 0 )
      {
        localPosition = encoderTicksPerRevolution + localPosition;
        myEnc.write( localPosition );
        position = localPosition;
      }
      else if ( localPosition > encoderTicksPerRevolution )
      {
         localPosition = localPosition - encoderTicksPerRevolution;
        myEnc.write( localPosition );
        position = localPosition;
      }
   lastPosition = position;
   }
   return position/encoderTicksPerRevolution * 360.0F;
 } 
#endif //Not remote

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
    if ( response == HTTP_CODE_OK && root.success() && root.containsKey("status") )
    {
      value = ( enum shutterState) root.get<int>("status");    
    }
    else
    {
      value = SHUTTER_ERROR;
      debugW("Shutter controller call not successful");
      debugV("Shutter response: %i, parse result %i, json data %s", response, root.success(), outbuf.c_str() );

#if defined DEBUG_ESP_HTTP_CLIENT      
      debugV("Shutter response: %i, parse result %i, json data %s", response, root.success(), outbuf.c_str() );
#endif      
    }
  return value;
  }

#endif
