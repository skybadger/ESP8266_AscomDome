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
 int restQuery( String uri, String path, String args, String& response, enum HTTPMethod method );
 float getAzimuth( float );
 void setupEncoder();
 bool setupCompass( String host );
 float getBearing( String host );
 int getShutterStatus( String host , enum shutterState& );
  
 //Status Event processing
 cmdItem_t* addDomeCmd( uint32_t clientId, uint32_t transId, String cmdName, enum domeCmd, int value );
 cmdItem_t* addShutterCmd( uint32_t clientId, uint32_t transId, String cmdName, enum shutterCmd, int value );
 void updateCmdResponseList( int transID );
 void freeCmd( cmdItem_t* cmd );
 
 //dome state handlers
 void onDomeSlew();
 void onDomeAbort();
 void onDomeIdle();
 
 //Shutter state handlers
 void onShutterIdle();
 int shutterAltitude( int newAngle );
 int shutterSlew( enum shutterCmd setting );
 //int shutterAbort( void ); //deprecated

  //This detects for domeLocked state handled in OnDomeSlew
  static int domeLockDetectedCount = 0;
  //This is to ensure domeLocked handling isn't invoked due to a missing or repeat reading due to being unable to access remote devices. 
  static int domeLockDetectedEntryCount = 5;
  
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

 /*
 * @brief create a new dome command list entry
 */
  cmdItem_t* addDomeCmd( uint32_t clientId, uint32_t transId, String cmdName, enum domeCmd newCmd, int value )
  {
      //Create new command
      cmdItem_t* pCmd = (cmdItem_t*) calloc( sizeof( cmdItem_t ), 1); 

      //Create copy buffer for the command name
      char* cptr = (char*) calloc( sizeof( char), cmdName.length() + 1 );
      strncpy( cptr, cmdName.c_str(), cmdName.length() );
      cptr[cmdName.length()] = '\0';

      //Add to list 
      pCmd->cmdName = cptr;
      pCmd->cmd = (int) newCmd;
      pCmd->value = value;
      pCmd->clientId = clientId;
      pCmd->transId = transId; 
      
      domeCmdList->add( pCmd );
      debugI( "Cmd added: %i", (int) newCmd ); 
      return pCmd;
  }

/*
 * @brief create a new shutter command list entry
 */ 
  cmdItem_t* addShutterCmd( uint32_t clientId, uint32_t transId, String cmdName, enum shutterCmd newCmd, int value )
  {
      //Create new command
      cmdItem_t* pCmd = (cmdItem_t*) calloc( sizeof( cmdItem_t ), 1); 

      //Create copy buffer for the command name
      char* cptr = (char*) calloc( sizeof( char ), cmdName.length() +1 );
      strncpy( cptr, cmdName.c_str(), cmdName.length() );
      cptr[cmdName.length()] = '\0';   
      
      //Add to list 
      pCmd->cmdName = cptr;
      pCmd->cmd = (int) newCmd;
      pCmd->value = value;
      pCmd->clientId = clientId;
      pCmd->transId = transId; 
      shutterCmdList->add( pCmd );

      debugI( "Cmd added: %i", (int) newCmd ); 
      return pCmd;
  }

  void freeCmd( cmdItem_t* ptr) 
  {
    //Safely release cmd memory
    //Clean up any memory dependencies
    if( ptr->cmdName != nullptr )
      free( ptr->cmdName );

    if( ptr != nullptr ) 
      free( (cmdItem_t*) ptr ); 
    
    debugV( "Cmd freed" ); 
    return;
  }
  
  /* Handle the command list - we've already checked we are idle at this point.
   */
  void onDomeIdle( void )
  {
    cmdItem_t* pCmd = nullptr;    
    enum domeCmd newCmd; 
    
    if ( domeCmdList->size() > 0 ) 
    {
      pCmd = domeCmdList->shift();
      debugI( "Popped next command: %i, value: %i", (int) pCmd->cmd, (int) pCmd->value );
      newCmd = (enum domeCmd ) pCmd->cmd;
      String cmd = "";
      switch ( newCmd )
      {
        // HOME and PARK are handled by slew commands but these entries satify the compiler warnings. 
        case CMD_DOME_HOME:
          targetAzimuth = (float) homePosition;
          domeLockDetectedCount = false;
          onDomeSlew();
          domeStatus = DOME_SLEWING;
          break;

        case CMD_DOME_PARK:
          targetAzimuth = (float) parkPosition;
          domeLockDetectedCount = false;
          onDomeSlew();
          domeStatus = DOME_SLEWING;
          break;
          
        case CMD_DOME_SLEW:
          targetAzimuth = (float) pCmd->value;
          domeLockDetectedCount = false;
          onDomeSlew();
          domeStatus = DOME_SLEWING;         
          break;

        case CMD_DOME_ABORT:
          onDomeAbort();
          domeStatus = DOME_IDLE;
          targetAzimuth = currentAzimuth;
          break;
        
        case CMD_DOMEVAR_SET:
          //Did this so that dome variables aren't updated while waiting for commands that preceeded them to execute.   
          if ( pCmd->cmdName == nullptr )
          {
            debugW( "DOME_VAR_SET nullptr command name");
          }
          else 
          {
            cmd = String( pCmd->cmdName );
            debugD( "DOME_VAR_SET new command name: %s", pCmd->cmdName );            
          }
          
          if ( strlen( pCmd->cmdName) == 0 )
          {
            debugW( "DOME_VAR_SET empty command name");            
          }
          else if( cmd.equalsIgnoreCase( F("parkPosition")) )
          {
              parkPosition = (int) pCmd->value;
              debugI( "DOME_VAR_SET new park position : %i ", parkPosition );
              saveToEeprom();
          }
          else if ( cmd.equalsIgnoreCase( F("homePosition")) )
          {
              homePosition = (int) pCmd->value;
              debugI( "DOME_VAR_SET new home position : %i ", homePosition );
              saveToEeprom();
          }
          else if ( cmd.equalsIgnoreCase( F("azimuthSyncOffset") ) )
          {
              azimuthSyncOffset = (float) pCmd->value;
              debugI( "DOME_VAR_SET new sync offset : %f ", azimuthSyncOffset  );
              saveToEeprom();
              //01/06/2021 Updated to comply with requirement of updated Azimuth to be available after Sync. 
              //May not be fast enough. 
              currentAzimuth = bearing + azimuthSyncOffset;
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
    static float startAzimuth = 0.0F;
    static float lastAzimuth = 0.0F;
    static const float minMovementLimit = 0.1F;
    
    float distance = 0.0F;
    enum motorSpeed speed = MOTOR_SPEED_OFF;
    enum motorDirection direction = MOTOR_DIRN_CW; 
    float localAzimuth = 0.0F; 
          
    //getAzimuth keeps the value in range and applies any sync offset to the raw bearing. 
    localAzimuth = getAzimuth( bearing );
    //359-1 = 358
    //1-359 = -358
    
    //normalise keeps the distance in range 0-360
    distance = (targetAzimuth - localAzimuth);
    //distance = normaliseFloat( distance, 360.0 );
    debugD( "OnDomeSlew: current: %03.2f, target: %03.2f, distance %03.2f", localAzimuth, targetAzimuth, distance );

    //looking at the options for a-b < D where valid D might be small +ve, -ve or large +ve and large -ve    
    //Updated 29/09/21 to address slow speed when starting near 0 and inaccurate home/park determination
    float delta = 0.0F;

    //Have we finished ?
    delta = abs(distance);
    if( delta < acceptableAzimuthError || (360.0F - delta) < acceptableAzimuthError )
    {
      //Stop!!
      //turn off motor
      myMotor.setSpeedDirection( speed = MOTOR_SPEED_OFF, direction );

      //Update the desired state to say we have finished.
      domeStatus = DOME_IDLE;
      slewing = false;

      debugI( "Dome stopped at target: %03.1f", localAzimuth);
      if ( lcdPresent )
        myLCD.writeLCD( 2, 0, "Slew complete." );

      lastAzimuth = localAzimuth;
      startAzimuth = lastAzimuth; //the last place we successfully arrived at due to a slew. 
      domeLockDetectedCount = false;
      slewing = false;
      return; 
    }
    
    //01/06/2021 Amended to manage the zero hunting issue where the fast speed is selected for a small reversal slew. 
    //check to see whether we mean : 
    //from 4 to 356 = 352 or -8 after reversal or 
    //from 356 to 4 = -352 or +8 after reversal

    //we're not there yet so work out the direction. We may not yet be moving post IDLE state
    //Work out direction and speed to turn dome
    debugD( "OnDomeSlew: raw distance : %f, direction : %i", distance, (int) direction  );    
    if( ( distance >= 0.0F ) && ( distance <= 180.0F ) ) 
    {
      direction = MOTOR_DIRN_CW;
    }
    else if( distance > 180.0F )
    {
      direction = MOTOR_DIRN_CCW;
      distance = 360 - distance;
    }
    else if ( ( distance > -180.0F ) && ( distance < 0.0F ) )
    {
      direction = MOTOR_DIRN_CCW;      
    }
    else //distance <= -180.0F
    {
      direction = MOTOR_DIRN_CW;
      distance = 360 + distance;
    }  
    
    //Set the speed based on distance to move. 
    if ( abs(distance) < slowAzimuthRange )
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
    debugD( "OnDomeSlew: raw distance : %f, direction : %i, speed: %i", distance, (int) direction, (int) speed  );      
    myMotor.setSpeedDirection( speed, direction );
    slewing = true;
    
    //Finally - check whether we are currently stalled
    //Criterion - no motion since last check when we are supposed to be slewing. 
    //However this might be the first call to onSlew after Idle so shouldnt check too soon. 
    //This will get called for every OnDomeSlew so we need to use the domeLockDetectedCount flag to see if its already handled. 
    //A regular abort will clear the flag
    //The flag is also clear on entry 
    if ( abs(localAzimuth - lastAzimuth ) < minMovementLimit  && domeStatus == DOME_SLEWING ) 
    {
        domeLockDetectedCount++;      
    }
    if ( domeLockDetectedCount > domeLockDetectedEntryCount ) 
    {     
      int clientId = 100;
      int transId = 1000;
      int slewTarget = 0;
      int orgTarget = int( targetAzimuth );
      
      
      debugW( "OnDomeSlew: DOMELOCK detected slewing to %i", (int) targetAzimuth ); 
      //Add a slew to outside of the |slowSlewRange+1| distance in the direction we came from. 
      if ( direction == MOTOR_DIRN_CW ) 
      {
        slewTarget = int( localAzimuth ) + (slowAzimuthRange +1 ); 
        normaliseInt( slewTarget , 360 );
      }
      else
      {
        slewTarget = int( localAzimuth) - ( slowAzimuthRange +1);
        slewTarget = normaliseInt( slewTarget,360 );
      }
      //slewTarget = normaliseInt( int( localAzimuth) - slowAzimuthRange -1, 360 );  
      addDomeCmd( clientId, transId, "", CMD_DOME_SLEW, slewTarget );         
      debugW( "OnDomeSlew: DOMELOCK - Added slew to reverse from lock @ %i: direction : %i", (int) localAzimuth, (int) direction );      
      
      //Add another slew to current +/ |2*(slowAzimuthRange ) + 1| - ie fast past the obstruction
      if ( direction == MOTOR_DIRN_CW ) 
      {
        slewTarget = int( localAzimuth ) - ( 2 * (slowAzimuthRange +1) );
        slewTarget = normaliseInt( slewTarget, 360 );
      }
      else 
      {
        slewTarget = int( localAzimuth) + ( 2 * slowAzimuthRange  + 1);
        slewTarget = normaliseInt( slewTarget, 360 );
      }
      addDomeCmd( clientId, transId, "", CMD_DOME_SLEW, slewTarget );
      debugW( "OnDomeSlew: DOMELOCK - Added slew to slew at speed past lock: direction : %i", (int) direction);      
      
      //Finally - add slew to get to original desired target position.
      addDomeCmd( clientId, transId, "", CMD_DOME_SLEW, orgTarget ); 
      debugW( "OnDomeSlew: DOMELOCK - Added slew to original target after lock: direction : %i", (int) direction  );      
      
      //Finally - abort this slew so we can process the ones just added - dont call onDomeAbort() as that resets the domeLock flag. 
      //turn off motor
      if( motorPresent )
        myMotor.setSpeedDirection( MOTOR_SPEED_OFF, MOTOR_DIRN_CW );
        debugW("Aborting- motor set to stop");
          
      //Update status to idle to process normally.
      domeStatus = DOME_IDLE;
    }
    
    //Update our persistent record of last place
    lastAzimuth = localAzimuth;
        
    myMotor.getSpeedDirection();
    debugV("Motor returned - speed %u, direction: %u", ( uint) myMotor.getSpeed(), (uint) myMotor.getDirection() );
    
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
    
    //Remove domeLock flag so we can detect it again in the OnSlew handler. 
    domeLockDetectedCount = 0;
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
  String cmd;
  
  switch ( shutterStatus )
  {
     case SHUTTER_ERROR:
     case SHUTTER_OPEN:
     case SHUTTER_CLOSED:
        //Get next command if there is one. 
        if ( shutterCmdList->size() > 0 ) 
        {
          pCmd = shutterCmdList->shift();
          newCmd = (enum shutterCmd) pCmd->cmd;
          debugI("OnShutterIdle - new command read: %s", shutterCmdNames[(int)newCmd] );  

          switch( newCmd )
          {
            case CMD_SHUTTER_ABORT: 
            case CMD_SHUTTER_OPEN:  
            case CMD_SHUTTER_CLOSE:
                if( shutterSlew( newCmd ) != HTTP_CODE_OK )
                {
                  //put it back into head of list to retry next time around.
                  shutterCmdList->unshift( pCmd );
                }
                else //else consume and free 
                {
                  //Clean up 
                  updateCmdResponseList( pCmd->transId );            
                  freeCmd( pCmd ) ;
                  //clear down
                  if( newCmd == CMD_SHUTTER_ABORT )
                    shutterCmdList->clear();           //TODO this is a memory leak - fortunately rare and under our control
                }
                break;
            case CMD_SHUTTERVAR_SET:
                if( pCmd->cmdName != nullptr && strlen( pCmd->cmdName ) > 0 ) 
                {
                  cmd = String( pCmd->cmdName );
                  if( cmd.equalsIgnoreCase( F("altitude") ) && ( (int) pCmd->value >= SHUTTER_MIN_ALTITUDE ) && ( (int) pCmd->value <= SHUTTER_MAX_ALTITUDE ) )
                  { //If its at either end then that is open or closed, not an altitude
                     if( shutterAltitude( (int)pCmd->value ) != HTTP_CODE_OK )
                     {
                        //In range but call failed - try again by reloading for next time. 
                        shutterCmdList->unshift( pCmd );
                     }
                     else
                     {
                        updateCmdResponseList( pCmd->transId );                
                        freeCmd( pCmd ) ;
                     }                     
                  }
                  else
                  {
                    debugW( "Shutter cmd altitude out of range: %i ", (int) pCmd->value);                    
                    freeCmd( pCmd ) ; //Ignore and delete
                  }
                }
                break;
            default:
              break;
          }
          debugI("State outcome for shutter: %s ", shutterStateNames[ (int) shutterStatus ] );
       }  
       break;
     
     //Wait for an idle state otherwise
     case SHUTTER_OPENING:
     case SHUTTER_CLOSING:
       debugI("Shutter opening or closing - waiting for idle to check for new state" );
       break;
     default: 
       debugW("Unknown state requested for shutter: %s ", shutterStateNames[(int) targetShutterStatus] );
       break;         
  }
}

/*
 * return HTTP_CODE_OK (200) for successful setting. 
 */
int shutterSlew( enum shutterCmd setting )
{
    String outbuf = "";
    String arg = "shutter=";
    int errorCode = 200;
    
    debugD( "shutterSlew: setting: %s", shutterCmdNames[(int) setting] );
    switch( setting )
    {
      case CMD_SHUTTER_ABORT: arg.concat( F("abort") );
        break;
      case CMD_SHUTTER_OPEN:  arg.concat( F("open") );
        break;
      case CMD_SHUTTER_CLOSE: arg.concat( F("close") );
        break;
      default: 
        debugE( "invalid shutterState provided to shutterSlew: %i - ignoring", (int) setting );
      break;
    }   

    errorCode = restQuery( shutterHostname, "/shutter", arg, outbuf, HTTPMethod::HTTP_PUT );  
    if( errorCode == HTTP_CODE_OK )
    {
      debugV("Successfully issued new state '%s' to shutter", arg.c_str() );
      if ( setting == CMD_SHUTTER_OPEN )
        shutterStatus = SHUTTER_OPENING;
      else if ( setting == CMD_SHUTTER_CLOSE )
        shutterStatus = SHUTTER_CLOSING;
      else if( setting == CMD_SHUTTER_ABORT )
      {
         if( shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_OPEN || shutterStatus == SHUTTER_CLOSING )
           shutterStatus = SHUTTER_OPEN;
         else 
           shutterStatus = SHUTTER_CLOSED;
      }
    }
    else 
    {
      debugW("Failed to send new state Cmd to shutter, error %d", (int) errorCode );
    }
    return errorCode;
}

/*
 * Return HTTP_CODE_OK (200) for success or other HTTP query response code for failure 
 */
int shutterAltitude( int newAngle )
{
    String outbuf = "";
    String path = "/shutter"; 
    String args  =  "shutter=";
    char charbuff[10];
    args.concat ( String( itoa( newAngle, charbuff, 10 ) ));
        
    debugD(" Setting up to send new altitude to shutter");
    int response = restQuery( shutterHostname, path , args, outbuf, HTTPMethod::HTTP_PUT);
    if( response == HTTP_CODE_OK )
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
    else 
    {
      //what to do ?
      debugW("Failed to send new altitude to shutter, return HTTP code %i", response );
      //Preserve existing state - no change
    }
    return response;
}
  
  /*
   * Stop shutter moving. 
   * Return HTTP_CODE_OK for good reponse and HTTP query codes for anything else. 
   * Now handled in shutterSlew above. 

  int shutterAbort( void )
  {
    int response = 200;
    String outbuf = "";
    String uri = "http:";
    //shutterCmdList->clear();
    uri.concat( sensorHostname );
    uri.concat("/shutter");

    debugV("Abort issued to shutter.");
    response = restQuery( uri, "{\"status\":\"abort\"}", outbuf , HTTPMethod::HTTP_PUT);
    if( response == HTTP_CODE_OK)
    {
      debugV("Abort successful.");
      if( shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_CLOSING || shutterStatus == SHUTTER_OPEN )
        shutterStatus = SHUTTER_OPEN;
      else
      {
        shutterStatus = SHUTTER_CLOSED;
      }
    }
    else 
    {
      //retain existing state
      debugW("Failed to issue Abort to shutter, return HTTP response %i", response );
    }
    return response;
  } 
*/  
  void setupEncoder()
  {
    //TODO if we ever have a locally-attached encoder. 
    return;
  }
 
  //Detect compass service and initialise azimuth bearing of dome
  bool setupCompass( String targetHost )
  {
    bool status = false;
    String output = "";
    int response = 0;
    float value;
    DynamicJsonBuffer jsonBuff(256);
    
    response = restQuery( targetHost , "/bearing" , "", output, HTTPMethod::HTTP_GET );
    debugD( "[HTTPClient response ] response code: %i, response: %s", (int) response, output.c_str() );
    JsonObject& root = jsonBuff.parse( output );
    
    if ( response == HTTP_CODE_OK && root.success() && root.containsKey(F("bearing")) )
    {
      value = (float) root.get<float>("bearing"); //initialise current setting
      status = true;
    }
    else
    {
      debugE( "Shutter compass bearing call not successful, response: %i", (int) response );
      debugV( "bearing json content: %s", output.c_str() );
    }
    return status;
  }
 
  
  /*
   * Query a url for a parsed json output
   * check return code with object.success();
   */ 
  int restQuery( String host, String uri, String args, String& response, enum HTTPMethod method)
  {
    int httpCode = 0;
    long int startTime;
    long int endTime;
    WiFiClient wclient;
    HTTPClient httpClient; 
    hClient.setTimeout ( (uint16_t) 5000 );    
    //hClient.setReuse( HTTP_CLIENT_REUSE );    
    
    debugD("restQuery request - uri:%s, path:%s, args:%s, method:%i", host.c_str(), uri.c_str(), args.c_str(), (enum HTTPMethod) method );
   
    IPAddress resolvedIP;

    if (WiFi.hostByName(host.c_str(), resolvedIP))
    {
        debugI("DNS: %s -> %s",              host.c_str(),              resolvedIP.toString().c_str());
    } 
    else 
    {
        debugE("DNS lookup failed for %s", host.c_str());
    }
    
    //host and args are separate, need to join them and add a protocol and args for a GET parameterised request.    
    host.trim();
    uri.trim();
    args.trim();
    if ( host == nullptr || host.length() == 0 || host == "" )
      return HTTP_CODE_BAD_REQUEST; 
    if ( uri.length() == 0 || uri == "" ) 
      uri = "/"; 

    String path = String( host );
    if ( !path.startsWith( F("http://") ) && !path.startsWith( F("https://") ) )
      path = String( F("http://") ) + path;

    int authorityStart = path.indexOf( F("://") );
    authorityStart = ( authorityStart >= 0 ) ? authorityStart + 3 : 0;
    int pathStart = path.indexOf( '/', authorityStart );
    if ( pathStart >= 0 )
    {
      String baseUri = path.substring( pathStart );
      while ( baseUri.length() > 1 && baseUri.endsWith( F("/") ) )
        baseUri.remove( baseUri.length() - 1 );
      while ( path.length() > authorityStart && path.endsWith( F("/") ) )
        path.remove( path.length() - 1 );
      if ( uri == baseUri )
        uri = "/";
      else if ( uri.startsWith( baseUri + F("/") ) )
        uri.remove( 0, baseUri.length() );
    }

    if ( uri != F("/") )
    {
      while ( path.length() > authorityStart && path.endsWith( F("/") ) )
        path.remove( path.length() - 1 );
    }
    path.concat( uri );
    if ( args.length() != 0 && method == HTTPMethod::HTTP_GET ) 
    {
      path = path.concat("?");
      path = path.concat( args );
    }

    debugI("restQuery request - path:%s", path.c_str() );
    startTime = millis();
    if ( httpClient.begin( wclient, path ) ) 
    {
      switch( method ) 
      {
        case HTTPMethod::HTTP_GET: 
        httpCode = httpClient.GET();  
        break;
      
      case HTTPMethod::HTTP_PUT:
        hClient.addHeader(F("Content-Type"), F("application/x-www-form-urlencoded") );
        httpCode = httpClient.PUT( args );             
        break;
      
      default: 
        debugE("Unable to open connection to '%s' of unsupported type: %i", path.c_str(), method );
        response = "";
        break;
      }//end switch
      endTime = millis();    
#if defined DEBUG_ESP_HTTP_CLIENT            
      debugV( "Time for restQuery call(mS): %li\n", (long int) endTime-startTime );
#endif        

      // file found at server ?
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
      {
        response = httpClient.getString();
#if defined DEBUG_ESP_HTTP_CLIENT      
        debugV("HTTP rest query response : %s\n", response.c_str() );
#endif        
      }
      else 
      {
        debugW("restQuery ... failed, error: %s\n", httpClient.errorToString(httpCode).c_str() );
        response = "";
      } 
    }//end hclient.
    else //hclient.begin() failed 
    {
      debugE("restQuery ... httpCient.begin failed, \n" );
      response = "";
      httpCode = -1;      
    }

    httpClient.end();
    return httpCode;
  }
  
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  /*
   * Function to determine bearing of dome - just return a sane value to the caller. Let the caller handler sync offsets. 
   * Note that if we do this right we can point this at any device that returns a 'bearing' in the rest response as long as the url is
   * setup correctly in the setup pages. 
   * That means the 'host' string is no longer strictly just a hostname. 
   */
  float getBearing( String host)
  {
    long int duration = millis();
    static int bearingRepeatCount = 0;
    const int bearingRepeatLimit = 10;
    static float lastBearing = bearing;
    float localBearing = 0.0F;
    int response = 0;
    String outbuf = "";
    String path = "";
    DynamicJsonBuffer jsonBuff(250);    
    
    debugV("GetBearing setup - host uri: %s \n", host.c_str() );
    response = restQuery( host, "/encoder/bearing", "", outbuf, HTTP_GET );
    
    JsonObject& root = jsonBuff.parse( outbuf );
    //Sometimes we get a good HTTP code but still no body... 
    if ( response == HTTP_CODE_OK ) 
    {            
      if( root.success() && root.containsKey( "bearing" ) )
      {
        localBearing = (float) root["bearing"];
        lastBearing = localBearing;
        debugD(" GetBearing: localBearing %f", localBearing );     

#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION        
        //this isnt the right thing to do if its due to not getting a rest response in time from a busy device. 
        if ( lastBearing == bearing ) //We do this to check for sensor freeze
        {
          bearingRepeatCount ++;
          debugW(" GetBearing: Sensor stuck %i detected @ %f\n", bearingRepeatCount, bearing );
          
          //reset the compass by resetting the target device
          if ( bearingRepeatCount > bearingRepeatLimit ) 
          {  
            response = restQuery( host, "/reset", outbuf, HTTP_PUT );
            debugW(" GetBearing: Compass reset attempted !");
            if (myLCD.present ) 
              myLCD.writeLCD( 2, 0, "Compass reset !");
            bearingRepeatCount = 0;
          }
        }
        else
        {
          bearingRepeatCount = 0;            
        }
#endif      
      }
      else //can't retrieve the bearing
      {
        debugV( "Response code: %i, parse success: %i, json data: %s", response, (int) root.success(), outbuf.c_str() );
        debugW( "No reading, using last: %f ", lastBearing );
        localBearing = lastBearing;
      }
    }//HTTP_CODE_OK
    else //Hvent got a good response code
    {
      debugV( "Response code: %i, parse success: %i, json data: %s", response, (int) root.success(), outbuf.c_str() );
      debugW( "No reading, using last: %f ", lastBearing );
      localBearing = lastBearing;
    }
   
    duration = millis()-duration;
    debugI( "request duration %li", (long int) duration);
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
     updated to address request failure by not going straight to ERROR. 
   */ 
  int getShutterStatus( String host, enum shutterState& outputState )
  {
    String outbuf;
    long int duration = millis();
    enum shutterState value = SHUTTER_ERROR;
    DynamicJsonBuffer jsonBuff(250);

    int response = restQuery( host , "/status", "", outbuf, HTTP_GET  );
    
    if( response == HTTP_CODE_OK ) 
    {
      JsonObject& root = jsonBuff.parse( outbuf ); 
      if ( root.success() && root.containsKey("status") )
      {
        value = ( enum shutterState) root.get<int>("status");
      }
      else
      {
        value = shutterStatus; //Report the last response in the meantime. 
        debugW("Shutter controller call not successful, buffer: %s", outbuf.c_str() );
        debugV("Shutter response: %i, parse result %i, json data %s", (int) response, (int) root.success(), outbuf.c_str() );   
      }
    }

    duration = millis() - duration;
    debugI( "Shutter query duration: %li", (long int) duration );
    outputState = value;
    return response;
  }

#endif
