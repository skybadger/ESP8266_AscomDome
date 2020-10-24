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
 void updateCmdResponseList( int shutterStatus );
 void onShutterSlew();
 void onShutterIdle();
 void onDomeSlew();
 void onDomeAbort();
 void onDomeIdle();

  float normaliseFloat( float& input, float radix) 
  {
    float output = 0.0F;
    output = fmod( input, radix ) ;
    while( output < 0.0 ) //some compilers allow negative values with mod
    {
      output += radix;
    }
    return output;
  }
  
  int normaliseInt( int& input, int radix )
  {
    int output = 0.0;
    output = input % radix;
    while( output < 0.0 ) //some compilers allow negative values with mod
    {
      output += radix;
    }
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
    
    if ( domeCmdList->size() > 0 ) 
    {
      pCmd = domeCmdList->pop();
      debugI( "Popped new command: %i, value: %i", pCmd->cmd, pCmd->value );
      domeTargetStatus = pCmd->cmd;
      switch ( domeTargetStatus )
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
          if( pCmd->cmdName.equalsIgnoreCase( "parkPosition") )
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
          else if ( pCmd->cmdName.equalsIgnoreCase( "azimuthSyncOffset") )
          {
            
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
    int distance = 0;
    enum motorSpeed speed = MOTOR_SPEED_OFF;
    enum motorDirection direction = MOTOR_DIRN_CW; 
    float localAzimuth = 0.0F; 
      
    if( domeStatus != DOME_SLEWING )
      return;

    localAzimuth = getAzimuth( bearing );
    distance = targetAzimuth - localAzimuth ;
    debugD( "OnDomeSlew: current: %f, target: %f", localAzimuth, targetAzimuth );    

    //Work out speed to turn dome
    if( abs(distance) < acceptableAzimuthError )
    {
      //Stop!!
      //turn off motor
      myMotor.setSpeedDirection( speed = MOTOR_SPEED_OFF, direction );
      domeStatus = DOME_IDLE;
      if( abs( localAzimuth - homePosition ) < acceptableAzimuthError )
        atHome = true;
      if( abs( localAzimuth - parkPosition ) < acceptableAzimuthError )
        atPark = true;

      //Update the desired state to say we have finished.
      domeStatus = DOME_IDLE;

      debugI( "Dome stopped at target: %03f", localAzimuth);
      if ( lcdPresent )
        myLCD.writeLCD( 2, 0, "Slew complete." );
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
      debugD( "Dome speed fast - distance: %i", distance );    
      if ( lcdPresent ) 
        myLCD.writeLCD( 2, 0, "Slew::Fast" );
    }
    
    //Work out the direction. 
    direction = ( distance > 0 ) ? MOTOR_DIRN_CW: MOTOR_DIRN_CCW;
    debugD( "Dome target distance & dirn: %i, %i", distance, direction );         

    if ( abs(distance) > 180 )
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
  
 void updateCmdResponseList( int shutterStatus )
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
    debugW("Aborting- set motor to stop");
    
    //message to LCD  
    if( lcdPresent ) 
      myLCD.writeLCD( 2, 0, "ABORT received - dome halted." );
    
    // ? close shutter ?
    
    //Update status to idle to process normally.
    domeStatus = DOME_IDLE;
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
    debugI("OnShutterIdle - new command popped: %i", pCmd->cmd );  
    shutterTargetStatus = pCmd->cmd;
    switch ( shutterTargetStatus )
    {
       case CMD_SHUTTER_OPEN: 
       case CMD_SHUTTER_CLOSE:           
          onShutterSlew();
          break;
       case CMD_SHUTTER_ABORT:
          uri.concat( sensorHostname);
          uri.concat("/shutter");
          restQuery( uri, "status=abort", response, HTTP_PUT);
          if( lcdPresent) 
            myLCD.writeLCD( 2, 0, "DSH ABORT" );
          break;
       case CMD_SHUTTERVAR_SET:
       default:
        break;
    }
    freeCmd(pCmd); 
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

    debugI("OnShutterSlew - Shutter status update: %i", currentShutterStatus );
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
    debugW("Abort issued to shutter");
    updateCmdResponseList( shutterStatus );
    shutterStatus = SHUTTER_OPEN;//there is no idle state - so default to OPEN and let close if necessary
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
    debugD( "[HTTPClient response ] response code: %i, response: %s", response, output.c_str() );
    JsonObject& root = jsonBuff.parse( output );
    
    if ( response == HTTP_CODE_OK && root.success() && root.containsKey("bearing") )
    {
      value = (float) root.get<float>("bearing"); //initialise current setting
      status = true;
    }
    else
    {
      debugE( "Shutter compass bearing call not successful, response: %i", response );
      debugV( "bearing json content: %s", output.c_str() );
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
    HTTPClient hClient;
    hClient.setTimeout ( (uint16_t) 250 );    
    hClient.setReuse( true );    
    
    //debugI("restQuery request - uri: %s args:%s\n", host.c_str(), args.c_str() );
    startTime = millis();
    if ( hClient.begin( wClient, host) ) //host and args are separate, need to join them for a GET parameterised request.    //if ( hClient.begin( wClient, uri ) ) //uri must already have request args in it
    {
      if( method == HTTP_GET )
      {
        httpCode = hClient.GET();
        endTime = millis();
        //debugV( "Time for restQuery call(mS): %li\n", endTime-startTime );
      }
      else if ( method == HTTP_PUT ) //variables are added as headers
      {
        httpCode = hClient.PUT( args );        
        endTime = millis();
        //debugV("Time for restQuery call: %li mS\n", endTime-startTime );
      }
     
      // file found at server ?
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
      {
        response = hClient.getString();
        //debugV("HTTP rest query : %s\n", response.c_str() );
      }
      else 
      {
        //debugV("restQuery ... failed, error: %s\n", hClient.errorToString(httpCode).c_str() );
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
   */
  float getBearing( String host)
  {
    static int bearingRepeatCount = 0;
    static float lastBearing = 0.0F;
    const int bearingRepeatLimit = 10;
    float bearing = 0.0F;
    int response = 0;
    String outbuf = "";
    String path = "";
    DynamicJsonBuffer jsonBuff(256);    
    
    //Update the bearing
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION        
    path = String( "http://" );
    path += host;
    path += "/bearing";
    debugD("GetBearing using remote compass - host path: %s \n", host.c_str() );
#elif defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
    path = String( "http://" );
    path += host;
    path += "/encoder/bearing";    
    debugD("GetBearing using remote encoder - host path: %s \n", path.c_str() );
#endif  
    debugD("GetBearing setup - host uri: %s \n", host.c_str() );
    response = restQuery( path, "", outbuf, HTTP_GET );
    debugD("GetBearing response - code: %i, output: %s", response, outbuf.c_str() );
    
    //Sometimes we get a good HTTP code but still no body... 
    JsonObject& root = jsonBuff.parse( outbuf );
    if ( response == HTTP_CODE_OK && root.success() && root.containsKey( "bearing" ) )
    {            
        bearing = (float) root["bearing"];
        debugD(" GetBearing: bearing %f", bearing );     

#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION        
      if ( lastBearing == bearing ) //We do this to check for sensor freeze
      {
        bearingRepeatCount ++;
        debugW(" GetBearing: Sensor stuck %i detected @ %f\n", bearingRepeatCount, bearing );
        
        //reset the compass by resetting the target device
        if ( bearingRepeatCount > bearingRepeatLimit ) 
        {  
          uri = "http://";
          uri.concat( sensorHostname );
          uri.concat( "/encoder/bearing/reset" );
          
          response = restQuery( uri, "", outbuf, HTTP_PUT );
          debugW(" GetBearing: Compass reset attempted !");
          myLCD.writeLCD( 2, 0, "Compass reset !");
          bearingRepeatCount = 0;
        }
      }
      else //good value not stuck 
      {
        debugV(" GetBearing: Reading obtained %f\n", bearing );
        lastBearing = bearing;
        bearingRepeatCount = 0;
      }
#endif      
    }
    else //can't retrieve the bearing
    {
      //debugV( "GetBearing: response code: %i, parse success: %i, json data: %s", response, root.success(), outbuf.c_str() );
      debugW("GetBearing: no reading, using last  ");

      //hand back the current bearing, so we don't slew off into strange territory when we return 0.0f instead. 
      bearing = currentAzimuth - azimuthSyncOffset;
    }
    return bearing;
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
      //debugD("Shutter response: %i, parse result %i, json data %s", response, root.success(), outbuf.c_str() );
    }
  return value;
  }

#endif
