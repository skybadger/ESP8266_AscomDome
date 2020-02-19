#if !defined _ASCOMDOME_EEPROM_HANDLER_H_
#define _ASCOMDOME_EEPROM_HANDLER_H_
//Header file for EEprom specific processing used by ASCOM DOME 

//Prototypes
void setupDefaults( void );
float readFloatFromEeprom( uint8_t addr );
void writeFloatToEeprom( uint8_t addr, float value );
int readIntFromEeprom( uint8_t addr );
void writeIntToEeprom( uint8_t addr, int value );
void readFromEeprom();
void saveToEeprom();

/* TODO - make standalone rather than packed header
#include <EEPROM.h>
#include "EEPRPMAnything.h"
extern ...
*/

//Function to setup ASCOM Dome default settings, use prior to reading the EEPROM settings which then override them.
  void setupDefaults()
  {
    int i = 0;
    char* defaultNameList[5] = { defaultHostname, defaultSensorHostname, defaultShutterHostname, mqtt_server, defaultHostname };      
    //Array of addresses (ie ptrs) of variables holding ptrs to char*
    char** targetNameList[5]  = { &myHostname, &sensorHostname, &shutterHostname, &MQTTHostname, &Name };
    char* temp = nullptr;
    
    //azimuth
    currentAzimuth = 0.0F;
    //azimuthOffset
    azimuthOffset = 0.0F;
    //homePosition
    homePosition = defaultHomePosition;
    //parkPosition
    parkPosition = defaultParkPosition;
      
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
    for ( i=0; i< sizeof(int); i++)
    {
       EEPROM.write( addr++, ptr[i] ); 
    }
    free(ptr);
  }

  void readFromEeprom()
  {
    int addr = 0;
     if( ( (char) EEPROM.read(0)) != '*' )
    {
      //read
      setupDefaults();      
      saveToEeprom();
    }  

    addr++;
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
    atHome = (currentAzimuth == homePosition );
    atPark = (currentAzimuth == parkPosition );
    
    //Now read back strings
    int i = 0, j=0;
    char inC = '_';
    char* tempBuf = nullptr;
    char* names[4] = { myHostname, sensorHostname, shutterHostname, MQTTHostname };
    
    tempBuf = (char*) malloc( nameLengthLimit );   
    for ( j=0; j < sizeof(names); j++ )
    {
      inC = '_';
      i=0;
      while( i < nameLengthLimit && inC != '\0' )
      {
        inC = (char) EEPROM.read( addr );
        tempBuf[i] = inC;
        addr++;
        i++;
      }      
      names[j] = ( char* ) malloc( i ); 
      strncpy( names[j], tempBuf, i);
    }
    return;
  }

  void saveToEeprom( void )
  {
    int addr = 0;
    String tempS = "";
    int i = 0, j=0;
    char inc = '_';
    char* tempBuf = nullptr;
    
    EEPROM.write( ++addr, currentAzimuth );
    addr += ( sizeof( float) );
    EEPROM.write( addr, azimuthOffset );
    addr += ( sizeof( float) );
    EEPROM.write( addr, homePosition);
    addr += ( sizeof( int ) );
    EEPROM.write( addr, parkPosition );
    addr += ( sizeof( int ) );
    EEPROM.write( addr, altitude );
    addr += ( sizeof( int ) );

    char* names[5] = { myHostname, sensorHostname, shutterHostname, MQTTHostname, Name };
    for ( j=0; j < sizeof(names) ; j++ )
    {
      for ( i=0; i< sizeof( names[j] ) ; i++ ) 
      {
        EEPROM.write( addr, names[j][i] );
      }
      addr += sizeof( names[j] );
    } 
    //commit changes
    EEPROM.commit();
  }
#endif