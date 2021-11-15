#if !defined _ASCOMDOME_EEPROM_HANDLER_H_
#define _ASCOMDOME_EEPROM_HANDLER_H_
//Header file for EEprom specific processing used by ASCOM DOME 

//Prototypes
void readFromEeprom( void );
void saveToEeprom( void );
void setupDefaults( void );
float readFloatFromEeprom( uint8_t addr );
void writeFloatToEeprom( uint8_t addr, float value );
int readIntFromEeprom( uint8_t addr );
void writeIntToEeprom( uint8_t addr, int value );

int eepromSize = 4 + ( 6 * MAX_NAME_LENGTH ) + sizeof( azimuthSyncOffset ) + sizeof( currentAzimuth ) + sizeof( homePosition ) + sizeof( parkPosition ) + sizeof( altitude ); 

/* TODO - make standalone rather than packed header
#include <EEPROM.h>
#include "EEPROMAnything.h"
extern ...
*/

void readFromEeprom()
{
  int addr = 0;
 
  if( ( (char) EEPROM.read(0) ) != '*' )
  {
    //read
    DEBUGSL1( F("Existing eeprom settings not found - setting up defaults..") );
    setupDefaults();      
    saveToEeprom();
    device.restart(); //restart to take advantage of new settings. 
  }  

  //Test readback of contents
  char ch;
  int i = 0;
  String input;
  for ( i = 0; i < eepromSize ; i++ )
  {
    ch = (char) EEPROM.read( i );
    if ( ch == '\0' )
      ch = '_';
    if ( (i % 25) == 0 )
      input.concat( "\n\r" );
    input.concat( ch );
  }

  Serial.printf_P( PSTR("EEPROM contents before: \n %s)\n"), input.c_str() );

  DEBUGSL1( F("Existing eeprom settings found - reading existing..") );
  addr = 1;
  EEPROMReadAnything( addr, azimuthSyncOffset );
  addr += ( sizeof( float) );
  EEPROMReadAnything( addr, currentAzimuth );
  targetAzimuth = currentAzimuth;
  addr += ( sizeof( float) );
  EEPROMReadAnything( addr, homePosition);
  addr += ( sizeof( int ) );
  EEPROMReadAnything( addr, parkPosition );
  addr += ( sizeof( int ) );
  EEPROMReadAnything( addr, altitude );
  addr += ( sizeof( int ) );

  //Make assumption that park position was where we stopped the dome last. 
  currentAzimuth = parkPosition;
  //TODO Set encoder current position

  //Now read back strings
  DEBUGS1( F("Starting address for strings: ") );DEBUGSL1( addr );
  
  if ( myHostname != nullptr ) 
      free ( myHostname );    
  myHostname = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
  EEPROMReadString( addr, myHostname, MAX_NAME_LENGTH );
  DEBUGS1( F("myHostname: ") );DEBUGSL1( myHostname );
    
  if ( thisID != nullptr ) 
    free ( thisID );    
  thisID = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );    
  strcpy( thisID, myHostname );
  DEBUGS1( F("thisID: ") );DEBUGSL1( thisID );
  addr+= MAX_NAME_LENGTH;
 #if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  if ( sensorHostname != nullptr ) 
    free ( sensorHostname );
  sensorHostname = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
  EEPROMReadString( addr, sensorHostname, MAX_NAME_LENGTH );
  addr+= MAX_NAME_LENGTH;
  DEBUGS1( F("sensorHostname: ") );DEBUGSL1( sensorHostname );
#endif
    
  if ( shutterHostname != nullptr ) 
    free ( shutterHostname );
  shutterHostname = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
  EEPROMReadString( addr, shutterHostname, MAX_NAME_LENGTH );
  addr+= MAX_NAME_LENGTH;
  DEBUGS1( F("shutterHostname: ") );DEBUGSL1( shutterHostname );
  
  if ( MQTTServerName != nullptr ) 
      free ( MQTTServerName );
  MQTTServerName = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
  EEPROMReadString( addr, MQTTServerName, MAX_NAME_LENGTH );
  addr+= MAX_NAME_LENGTH;
  DEBUGS1( F("MQTTServerName: ") );DEBUGSL1( MQTTServerName );
    
  if ( ascomName != nullptr ) 
      free ( ascomName );
  ascomName = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
  EEPROMReadString( addr, ascomName, MAX_NAME_LENGTH );
  addr+= MAX_NAME_LENGTH;
  DEBUGS1( F("ascomName: ") );DEBUGSL1( ascomName );  
  
  Serial.printf_P( PSTR("Values read from EEPROM are: myHostname: %s, sensorHostname:%s, shutterHostname: %s, MQTT: %s, ASCOM Name: %s\n"), myHostname, sensorHostname, shutterHostname, MQTTServerName, ascomName );

  //Test readback of contents
  for ( i = 0; i < eepromSize ; i++ )
  {
    ch = (char) EEPROM.read( i );
    if ( ch == '\0' )
      ch = '~';
    if ( (i % 25) == 0 )
      input.concat( "\n\r" );
    input.concat( ch );
  }
  Serial.printf_P( PSTR("EEPROM contents after: \n %s \n"), input.c_str() );

  DEBUGSL1( F("Read settings from EEPROM complete") );
}

void saveToEeprom( void )
{
  int addr = 1;
  String tempS = "";
  int i = 0;
  
  DEBUGSL1( F("saveToEeprom entered") );
 
  addr = 1;
  EEPROMWriteAnything( addr, azimuthSyncOffset );
  addr += ( sizeof( float) );
  EEPROMWriteAnything( addr, currentAzimuth );
  addr += ( sizeof( float) );
  EEPROMWriteAnything( addr, homePosition);
  addr += ( sizeof( int ) );
  EEPROMWriteAnything( addr, parkPosition );
  addr += ( sizeof( int ) );
  EEPROMWriteAnything( addr, altitude );
  addr += ( sizeof( int ) );
  
  //Strings 
  DEBUGS1( F("Starting address for strings: ") );DEBUGSL1( addr );
  size_t sLen = strlen( myHostname );
  DEBUGS1( F("myHostname length: ") );DEBUGSL1( sLen );
  EEPROMWriteString( addr, myHostname, (size_t)MAX_NAME_LENGTH );
  addr += MAX_NAME_LENGTH;
  DEBUGS1( F("myHostname: ") );DEBUGSL1( myHostname );

#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION  
  EEPROMWriteString( addr, sensorHostname, (size_t)MAX_NAME_LENGTH ) ;
  addr += MAX_NAME_LENGTH;
  DEBUGS1( F("sensorHostname: ") );DEBUGSL1( sensorHostname );
#endif 
    
  EEPROMWriteString( addr, shutterHostname, (size_t)MAX_NAME_LENGTH ) ;
  addr += MAX_NAME_LENGTH;
  DEBUGS1( F("shutterHostname: ") );DEBUGSL1( shutterHostname );
  
  EEPROMWriteString( addr, MQTTServerName, (size_t)MAX_NAME_LENGTH ) ;
  addr += MAX_NAME_LENGTH;
  DEBUGS1( F("MQTTServerName: ") );DEBUGSL1( MQTTServerName );
  
  EEPROMWriteString( addr, ascomName, (size_t)MAX_NAME_LENGTH ) ;
  addr += MAX_NAME_LENGTH;
  DEBUGS1( F("ascomName: ") );DEBUGSL1( ascomName );

  //Write character to say the data is saved. 
  EEPROM.put( 0, '*' );
  Serial.printf_P( PSTR("Saved values are: myHostname: %s, sensorHostname:%s, shutterHostname: %s, MQTT: %s, thisID: %s ASCOM name: %s\n"), myHostname, sensorHostname, shutterHostname, MQTTServerName, thisID, ascomName );
  //commit changes
  EEPROM.commit();
  DEBUGSL1( F("Save to EEprom complete") );
}

//Function to setup ASCOM Dome default settings, use prior to reading the EEPROM settings which then override them.
  void setupDefaults()
  {
    DEBUGSL1( F("setupDefaults entered") );

    azimuthSyncOffset = 0.0F; //+ve values means the dome is further round N through E than returned from the raw reading. 
    currentAzimuth = 0.0F;
    targetAzimuth = 0.0F;
    azimuth = 0; 
    currentAltitude = 0;
    targetAltitude = 0;
    altitude = 0;
    //atPark = false; Dynamically determined
    //atHome = false;
    abortFlag = false; 
    homePosition = defaultHomePosition;
    parkPosition = defaultParkPosition;
    connected = NOT_CONNECTED;
    slaved = false;
    slewing = false;  //not really used - we use the live state to provide this 
    domeStatus = DOME_IDLE;
    shutterStatus = SHUTTER_CLOSED;
      
    //Strings 
    if ( myHostname != nullptr ) 
       free ( myHostname );
    myHostname = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
    strcpy( myHostname, String( defaultHostname).c_str() ); //we do this to make use of the F() for putting const char* into PROGMEM
    
    //MQTT thisID copied from hostname
    if ( thisID != nullptr ) 
       free ( thisID );
    thisID = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );       
    strcpy ( thisID, String( myHostname ).c_str() );
    
    DEBUGS1( F("myHostname :") );DEBUGSL1( myHostname );

#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
    if ( sensorHostname != nullptr ) 
       free ( sensorHostname );
    sensorHostname = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
    strcpy(  sensorHostname, String( defaultSensorHostname).c_str() );
    DEBUGS1( F("sensorHostname :") );DEBUGSL1( sensorHostname );
#endif

    if ( shutterHostname != nullptr ) 
       free ( shutterHostname );
    shutterHostname = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
    strcpy(  shutterHostname, String( defaultShutterHostname).c_str() );
    DEBUGS1( F("shutterHostname :") );DEBUGSL1( shutterHostname );

    if ( MQTTServerName != nullptr )
        free ( MQTTServerName );
    MQTTServerName = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
    strcpy( MQTTServerName, String( mqtt_server).c_str() );
    DEBUGS1( F("MQTTServerName :") );DEBUGSL1( MQTTServerName );

    if ( ascomName != nullptr ) 
        free ( ascomName );
    ascomName = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
    strcpy( ascomName, String( defaultAscomName).c_str() );
    DEBUGS1( F("ascomName :") );DEBUGSL1( ascomName );
    
    //Serial.printf_P( PSTR("Default values are: myHostname: %s, sensorHostname:%s, shutterHostname: %s, MQTT: %s, thisID: %s\n"), myHostname, sensorHostname, shutterHostname, MQTTServerName, Name );
    /* Revisit this later. I'm sure it used to work 
    //Array of addresses (ie ptrs) of variables holding ptrs to char*
    const char* defaultNameList[5] = { defaultHostname, defaultSensorHostname, defaultShutterHostname, (const char*)mqtt_server, defaultHostname };      
    char* targetNameList[5]  = { myHostname, sensorHostname, shutterHostname, MQTTServerName, Name };
    char* temp = nullptr;

        //Serial.printf( "Sizeof( defaultNameList)/sizeof(char*) is %i\n", sizeof(defaultNameList)/sizeof(char*) );
    //Serial.printf( "MAX_NAME_LENGTH is %i\n", MAX_NAME_LENGTH );
    for ( i = 0; i < (sizeof(defaultNameList)/sizeof( char*)); i++ )
    {
      //Set Ascom device name as configured by dome setup  - initialise to hostname
      if ( *targetNameList[i] != nullptr ) 
        free ( *targetNameList[i] );
      *targetNameList[i] = (char*) calloc( MAX_NAME_LENGTH, sizeof( char)  );
      if( *targetNameList[i] != nullptr )
        strncpy(  *targetNameList[i], defaultNameList[i], MAX_NAME_LENGTH );
      Serial.printf( "Source default  %i is %s\n", i, defaultNameList[i] );
      Serial.printf( "Updated default %i to %s\n", i, *targetNameList[i] );
    }
    //Serial.println("finished");  
    Serial.printf( "New names are %s, %s, %s, %s, %s\n", myHostname, sensorHostname, shutterHostname, MQTTHostname, Name );

    Serial.printf( "New names are %s, %s, %s, %s, %s\n", myHostname, sensorHostname, shutterHostname, MQTTServerName, Name );
    */
    DEBUGSL1( F("SetupDefaults complete") );
  }
  
  float readFloatFromEeprom( uint8_t addr )
  {
    unsigned int i=0;
    float value = 0.0F;
    uint8_t* ptr = (uint8_t*) malloc(sizeof(float) );
    
    for ( i=0; i< sizeof(float); i++)
    {
      ptr[i] = EEPROM.read( addr++ ); 
    }
    value = *((float*)ptr);
    free(ptr);
    return value;
  }

  void writeFloatToEeprom( uint8_t addr, float value )
  {
    unsigned int i=0;
    uint8_t* ptr = (uint8_t*) malloc(sizeof(float) );
    
    *((float*) ptr) = value;
    for ( i=0; i< sizeof(float); i++)
    {
       EEPROM.write( addr++, ptr[i] ); 
    }
    free(ptr);
  }

  int readIntFromEeprom( uint8_t addr )
  {
    unsigned int i=0;
    int value = 0.0F;
    uint8_t* ptr = (uint8_t*) malloc(sizeof(int) );
    
    for ( i=0; i< sizeof(int); i++)
    {
      ptr[i] = EEPROM.read( addr++ ); 
    }
    value = *((int*)ptr);
    free(ptr);
    return value;
  }

  void writeIntToEeprom( uint8_t addr, int value )
  {
    unsigned int i=0;
    uint8_t* ptr = (uint8_t*) malloc(sizeof(int) );
    
    *((int*) ptr) = value;
    for ( i=0; i < sizeof(int) ; i++, addr++ )
    {
       EEPROM.write( addr, ptr[i] ); 
    }
    free(ptr);
  }

#endif
