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
    DEBUGSL1( "Existing eeprom settings not found - setting up defaults.." );
    setupDefaults();      
    saveToEeprom();
  }  

  DEBUGSL1( "Existing eeprom settings found - reading existing.." );
  addr = 1;
  currentAzimuth = (float) readFloatFromEeprom( addr );
  targetAzimuth = currentAzimuth;
  addr += ( sizeof( float) );
  azimuthOffset = (float) readFloatFromEeprom( addr );
  addr += ( sizeof( float) );
  homePosition = (int) readIntFromEeprom( addr );
  addr += ( sizeof( int ) );
  parkPosition = (int) readIntFromEeprom( addr );
  addr += ( sizeof( int ) );
  altitude = (int) readIntFromEeprom( addr );
  addr += ( sizeof( int ) );
  atHome = (currentAzimuth <= (homePosition +2) && currentAzimuth >= (homePosition -2) );
  atPark = (currentAzimuth <= (parkPosition +2) && currentAzimuth >= (parkPosition -2) );
  
  //Now read back strings
  if ( myHostname != nullptr ) 
      free ( myHostname );
  myHostname = (char*) calloc( nameLengthLimit, sizeof( char)  );
  EEPROMReadString( addr, myHostname, nameLengthLimit );
  addr+= nameLengthLimit;
  
  if ( sensorHostname != nullptr ) 
      free ( sensorHostname );
  sensorHostname = (char*) calloc( nameLengthLimit, sizeof( char)  );
  EEPROMReadString( addr, sensorHostname, nameLengthLimit );
  addr+= nameLengthLimit;
  
  if ( shutterHostname != nullptr ) 
      free ( shutterHostname );
  shutterHostname = (char*) calloc( nameLengthLimit, sizeof( char)  );
  EEPROMReadString( addr, shutterHostname, nameLengthLimit );
  addr+= nameLengthLimit;
  
  if ( MQTTServerName != nullptr ) 
      free ( MQTTServerName );
  MQTTServerName = (char*) calloc( nameLengthLimit, sizeof( char)  );
  EEPROMReadString( addr, MQTTServerName, nameLengthLimit );
  addr+= nameLengthLimit;
  
  if ( Name != nullptr ) 
      free ( Name );
  Name = (char*) calloc( nameLengthLimit, sizeof( char)  );
  EEPROMReadString( addr, Name, nameLengthLimit );
  addr+= nameLengthLimit;
  Serial.printf( "Values read from EEPROM are: myHostname: %s, sensorHostname:%s, shutterHostname: %s, MQTT: %s, thisID: %s\n", myHostname, sensorHostname, shutterHostname, MQTTServerName, Name );

  DEBUGSL1( "Read settings from EEPROM complete" );
}

void saveToEeprom( void )
{
  int addr = 1;
  String tempS = "";
  int i = 0, j=0;
  char inc = '_';
  char* tempBuf = nullptr;
  
  DEBUGSL1( "saveToEeprom entered" );
  
  addr = 1;
  EEPROM.put( addr, currentAzimuth );
  addr += ( sizeof( float) );
  EEPROM.put( addr, azimuthOffset );
  addr += ( sizeof( float) );
  EEPROM.put( addr, homePosition);
  addr += ( sizeof( int ) );
  EEPROM.put( addr, parkPosition );
  addr += ( sizeof( int ) );
  EEPROM.put( addr, altitude );
  addr += ( sizeof( int ) );

  //Strings 
  EEPROMWriteString( addr, myHostname, nameLengthLimit ) ;
  addr += nameLengthLimit;
  EEPROMWriteString( addr, sensorHostname, nameLengthLimit ) ;
  addr += nameLengthLimit;
  EEPROMWriteString( addr, shutterHostname, nameLengthLimit ) ;
  addr += nameLengthLimit;
  EEPROMWriteString( addr, MQTTServerName, nameLengthLimit ) ;
  addr += nameLengthLimit;
  EEPROMWriteString( addr, Name, nameLengthLimit ) ;
  addr += nameLengthLimit;
  
  Serial.printf( "Saved values are: myHostname: %s, sensorHostname:%s, shutterHostname: %s, MQTT: %s, thisID: %s\n", myHostname, sensorHostname, shutterHostname, MQTTServerName, Name );
  //Write character to say the data is worth reading. 
  EEPROM.put( 0, '*' );

  //commit changes
  EEPROM.commit();
  DEBUGSL1( "Save to EEprom complete" );
}

//Function to setup ASCOM Dome default settings, use prior to reading the EEPROM settings which then override them.
  void setupDefaults()
  {
    int i = 0;
    
    DEBUGSL1( "setupDefaults entered" );
    //azimuth
    currentAzimuth = 0.0F;
    //azimuthOffset
    azimuthOffset = 0.0F;
    //homePosition
    homePosition = defaultHomePosition;
    //parkPosition
    parkPosition = defaultParkPosition;
      
    //Strings 
    if ( myHostname != nullptr ) 
        free ( myHostname );
    myHostname = (char*) calloc( nameLengthLimit, sizeof( char)  );
    strncpy(  myHostname, defaultHostname, nameLengthLimit );

    if ( sensorHostname != nullptr ) 
        free ( sensorHostname );
    sensorHostname = (char*) calloc( nameLengthLimit, sizeof( char)  );
    strncpy(  sensorHostname, defaultSensorHostname, nameLengthLimit );

    if ( shutterHostname != nullptr ) 
        free ( shutterHostname );
    shutterHostname = (char*) calloc( nameLengthLimit, sizeof( char)  );
    strncpy(  shutterHostname, defaultShutterHostname, nameLengthLimit );

    if ( MQTTServerName != nullptr ) 
        free ( MQTTServerName );
    MQTTServerName = (char*) calloc( nameLengthLimit, sizeof( char)  );
    strncpy(  MQTTServerName, mqtt_server, nameLengthLimit );
    
    if ( Name != nullptr ) 
        free ( Name );
    Name = (char*) calloc( nameLengthLimit, sizeof( char)  );
    strncpy(  Name, myHostname, nameLengthLimit );
    Serial.printf( "Default values are: myHostname: %s, sensorHostname:%s, shutterHostname: %s, MQTT: %s, thisID: %s\n", myHostname, sensorHostname, shutterHostname, MQTTServerName, Name );
    /* Revisit this later. I'm sure it used to work 
    //Array of addresses (ie ptrs) of variables holding ptrs to char*
    const char* defaultNameList[5] = { defaultHostname, defaultSensorHostname, defaultShutterHostname, (const char*)mqtt_server, defaultHostname };      
    char* targetNameList[5]  = { myHostname, sensorHostname, shutterHostname, MQTTServerName, Name };
    char* temp = nullptr;

        //Serial.printf( "Sizeof( defaultNameList)/sizeof(char*) is %i\n", sizeof(defaultNameList)/sizeof(char*) );
    //Serial.printf( "nameLengthLimit is %i\n", nameLengthLimit );
    for ( i = 0; i < (sizeof(defaultNameList)/sizeof( char*)); i++ )
    {
      //Set Ascom device name as configured by dome setup  - initialise to hostname
      if ( *targetNameList[i] != nullptr ) 
        free ( *targetNameList[i] );
      *targetNameList[i] = (char*) calloc( nameLengthLimit, sizeof( char)  );
      if( *targetNameList[i] != nullptr )
        strncpy(  *targetNameList[i], defaultNameList[i], nameLengthLimit );
      Serial.printf( "Source default  %i is %s\n", i, defaultNameList[i] );
      Serial.printf( "Updated default %i to %s\n", i, *targetNameList[i] );
    }
    //Serial.println("finished");  
    Serial.printf( "New names are %s, %s, %s, %s, %s\n", myHostname, sensorHostname, shutterHostname, MQTTHostname, Name );

    Serial.printf( "New names are %s, %s, %s, %s, %s\n", myHostname, sensorHostname, shutterHostname, MQTTServerName, Name );
    */
    DEBUGSL1( "SetupDefaults complete" );
  }
  
  float readFloatFromEeprom( uint8_t addr )
  {
    int i=0;
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
    int i=0;
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
    int i=0;
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
    int i=0;
    uint8_t* ptr = (uint8_t*) malloc(sizeof(int) );
    
    *((int*) ptr) = value;
    for ( i=0; i < sizeof(int); i++, addr++ )
    {
       EEPROM.write( addr, ptr[i] ); 
    }
    free(ptr);
  }

#endif
