#include "DomeFunctionTrace.h"
#include "DomeHeapTrace.h"
#if !defined _ASCOMDOME_EEPROM_HANDLER_H_
#define _ASCOMDOME_EEPROM_HANDLER_H_
//Header file for EEprom specific processing used by ASCOM DOME

//Prototypes
/// @see readFromEeprom for the documented definition.
void readFromEeprom(void);
/// @see saveToEeprom for the documented definition.
bool saveToEeprom(void);
/// @see setupDefaults for the documented definition.
void setupDefaults(void);
/// @see readFloatFromEeprom for the documented definition.
float readFloatFromEeprom(uint8_t addr);
/// @see writeFloatToEeprom for the documented definition.
void writeFloatToEeprom(uint8_t addr, float value);
/// @see readIntFromEeprom for the documented definition.
int readIntFromEeprom(uint8_t addr);
/// @see writeIntToEeprom for the documented definition.
void writeIntToEeprom(uint8_t addr, int value);

//Keep the original offsets; reserve an extension after the legacy data.
int eepromSize = 512;
const int setupExtensionOffset = 320;
const int setupExtensionLength = 8 + MAX_NAME_LENGTH;
static_assert(1 + 5 * sizeof(uint32_t) + 5 * MAX_NAME_LENGTH <= setupExtensionOffset,
              "Legacy EEPROM data overlaps setup extension");
static_assert(setupExtensionOffset + setupExtensionLength <= 512, "Setup extension exceeds EEPROM buffer");

/**
 * @brief Fold the extension bytes into a checksum used to detect invalid persisted setup data.
 * @details Fold the extension bytes into a checksum used to detect invalid persisted setup data.
 * @param[in] bytes (const uint8_t *) Borrowed input bytes to checksum.
 * @param[in] length (size_t) Number of input bytes.
 * @return (uint16_t) CRC-16 value for the supplied bytes.
 */
uint16_t setupExtensionChecksum(const uint8_t *bytes, size_t length)
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("bytes storage=%p", (const void *)bytes);
                        debugV("length=%g", (double)length);
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  uint16_t crc = 0xffff;
  while (length--)
  {
    crc ^= *bytes++;
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc & 1) ? (crc >> 1) ^ 0xa001 : crc >> 1;
  }
  return crc;
}

/**
 * @brief Read the versioned EEPROM extension, validate its marker and checksum, and restore supported setup fields.
 * @details Read the versioned EEPROM extension, validate its marker and checksum, and restore supported setup fields.
 * @par Arguments
 * None.
 * @return (void) Observable effects are described above.
 */
void readSetupExtension()
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  uint8_t bytes[setupExtensionLength];
  for (int i = 0; i < setupExtensionLength; ++i)
    bytes[i] = EEPROM.read(setupExtensionOffset + i);
  const uint16_t crc = setupExtensionChecksum(bytes, setupExtensionLength - 2);
  const uint16_t port = bytes[3] | (uint16_t(bytes[4]) << 8);
  if (bytes[0] != 'D' || bytes[1] != 'S' || bytes[2] != 1 || port == 0 ||
      (bytes[5] & ~3) || bytes[6 + MAX_NAME_LENGTH - 1] != 0 ||
      bytes[setupExtensionLength - 2] != uint8_t(crc) || bytes[setupExtensionLength - 1] != uint8_t(crc >> 8))
    return;
  udpPort = port;
  parkDomeOnDisconnect = bytes[5] & 1;
  closeShutterOnDisconnect = bytes[5] & 2;
  memcpy(deviceLocation, bytes + 6, MAX_NAME_LENGTH);
}

/**
 * @brief Serialize the extended setup fields and checksum to the reserved EEPROM region.
 * @details Serialize the extended setup fields and checksum to the reserved EEPROM region.
 * @par Arguments
 * None.
 * @return (void) Observable effects are described above.
 */
void writeSetupExtension()
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  uint8_t bytes[setupExtensionLength] = {};
  bytes[0] = 'D';
  bytes[1] = 'S';
  bytes[2] = 1;
  bytes[3] = uint8_t(udpPort);
  bytes[4] = uint8_t(udpPort >> 8);
  bytes[5] = (parkDomeOnDisconnect ? 1 : 0) | (closeShutterOnDisconnect ? 2 : 0);
  memcpy(bytes + 6, deviceLocation, MAX_NAME_LENGTH);
  const uint16_t crc = setupExtensionChecksum(bytes, setupExtensionLength - 2);
  bytes[setupExtensionLength - 2] = uint8_t(crc);
  bytes[setupExtensionLength - 1] = uint8_t(crc >> 8);
  for (int i = 0; i < setupExtensionLength; ++i)
    EEPROM.write(setupExtensionOffset + i, bytes[i]);
}

/* TODO - make standalone rather than packed header
#include <EEPROM.h>
#include "EEPROMAnything.h"
extern ...
*/

/**
 * @brief Load the legacy EEPROM layout and supported extension, using defaults where persisted data is invalid.
 * @details Load the legacy EEPROM layout and supported extension, using defaults where persisted data is invalid.
 * @par Arguments
 * None.
 * @return (void) Observable effects are described above.
 */
void readFromEeprom()
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  int addr = 0;

  if (((char)EEPROM.read(0)) != '*')
  {
    //read
    DEBUGSL1(F("Existing eeprom settings not found - setting up defaults.."));
    setupDefaults();
    saveToEeprom();
    device.restart(); //restart to take advantage of new settings.
  }

  //Test readback of contents
  char ch;
  int i = 0;
  String input;
  for (i = 0; i < eepromSize; i++)
  {
    ch = (char)EEPROM.read(i);
    if (ch == '\0')
      ch = '_';
    if ((i % 25) == 0)
      input.concat("\n\r");
    input.concat(ch);
  }

  Serial.printf_P(PSTR("EEPROM contents before: \n %s)\n"), input.c_str());

  DEBUGSL1(F("Existing eeprom settings found - reading existing.."));
  addr = 1;
  EEPROMReadAnything(addr, azimuthSyncOffset);
  addr += (sizeof(float));
  EEPROMReadAnything(addr, currentAzimuth);
  targetAzimuth = currentAzimuth;
  addr += (sizeof(float));
  EEPROMReadAnything(addr, homePosition);
  addr += (sizeof(int));
  EEPROMReadAnything(addr, parkPosition);
  addr += (sizeof(int));
  EEPROMReadAnything(addr, altitude);
  addr += (sizeof(int));

  //Make assumption that park position was where we stopped the dome last.
  currentAzimuth = parkPosition;
  //TODO Set encoder current position

  //Now read back strings
  DEBUGS1(F("Starting address for strings: "));
  DEBUGSL1(addr);

  if (myHostname != nullptr)
    free(myHostname);
  myHostname = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  EEPROMReadString(addr, myHostname, MAX_NAME_LENGTH);
  DEBUGS1(F("myHostname: "));
  DEBUGSL1(myHostname);

  if (thisID != nullptr)
    free(thisID);
  thisID = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  strcpy(thisID, myHostname);
  DEBUGS1(F("thisID: "));
  DEBUGSL1(thisID);
  addr += MAX_NAME_LENGTH;
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  if (sensorHostname != nullptr)
    free(sensorHostname);
  sensorHostname = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  EEPROMReadString(addr, sensorHostname, MAX_NAME_LENGTH);
  addr += MAX_NAME_LENGTH;
  DEBUGS1(F("sensorHostname: "));
  DEBUGSL1(sensorHostname);
#endif

  if (shutterHostname != nullptr)
    free(shutterHostname);
  shutterHostname = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  EEPROMReadString(addr, shutterHostname, MAX_NAME_LENGTH);
  addr += MAX_NAME_LENGTH;
  DEBUGS1(F("shutterHostname: "));
  DEBUGSL1(shutterHostname);

  if (MQTTServerName != nullptr)
    free(MQTTServerName);
  MQTTServerName = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  EEPROMReadString(addr, MQTTServerName, MAX_NAME_LENGTH);
  addr += MAX_NAME_LENGTH;
  DEBUGS1(F("MQTTServerName: "));
  DEBUGSL1(MQTTServerName);

  if (ascomName != nullptr)
    free(ascomName);
  ascomName = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  EEPROMReadString(addr, ascomName, MAX_NAME_LENGTH);
  addr += MAX_NAME_LENGTH;
  DEBUGS1(F("ascomName: "));
  DEBUGSL1(ascomName);

  Serial.printf_P(PSTR("Values read from EEPROM are: myHostname: %s, sensorHostname:%s, shutterHostname: %s, MQTT: %s, ASCOM Name: %s\n"), myHostname, sensorHostname, shutterHostname, MQTTServerName, ascomName);

  //Test readback of contents
  for (i = 0; i < eepromSize; i++)
  {
    ch = (char)EEPROM.read(i);
    if (ch == '\0')
      ch = '~';
    if ((i % 25) == 0)
      input.concat("\n\r");
    input.concat(ch);
  }
  Serial.printf_P(PSTR("EEPROM contents after: \n %s \n"), input.c_str());

  readSetupExtension();
  DEBUGSL1(F("Read settings from EEPROM complete"));
}

/**
 * @brief Write current configuration and its extension to EEPROM and commit the emulated flash storage.
 * @details Write current configuration and its extension to EEPROM and commit the emulated flash storage.
 * @par Arguments
 * None.
 * @return (bool) True when EEPROM.commit succeeds; false when the flash commit fails.
 */
bool saveToEeprom(void)
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  DOME_HEAP_SCOPE("saveToEeprom");
  int addr = 1;
  String tempS = "";
  int i = 0;

  DEBUGSL1(F("saveToEeprom entered"));

  addr = 1;
  EEPROMWriteAnything(addr, azimuthSyncOffset);
  addr += (sizeof(float));
  EEPROMWriteAnything(addr, currentAzimuth);
  addr += (sizeof(float));
  EEPROMWriteAnything(addr, homePosition);
  addr += (sizeof(int));
  EEPROMWriteAnything(addr, parkPosition);
  addr += (sizeof(int));
  EEPROMWriteAnything(addr, altitude);
  addr += (sizeof(int));

  //Strings
  DEBUGS1(F("Starting address for strings: "));
  DEBUGSL1(addr);
  size_t sLen = strlen(myHostname);
  DEBUGS1(F("myHostname length: "));
  DEBUGSL1(sLen);
  EEPROMWriteString(addr, myHostname, (size_t)MAX_NAME_LENGTH);
  addr += MAX_NAME_LENGTH;
  DEBUGS1(F("myHostname: "));
  DEBUGSL1(myHostname);

#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  EEPROMWriteString(addr, sensorHostname, (size_t)MAX_NAME_LENGTH);
  addr += MAX_NAME_LENGTH;
  DEBUGS1(F("sensorHostname: "));
  DEBUGSL1(sensorHostname);
#endif

  EEPROMWriteString(addr, shutterHostname, (size_t)MAX_NAME_LENGTH);
  addr += MAX_NAME_LENGTH;
  DEBUGS1(F("shutterHostname: "));
  DEBUGSL1(shutterHostname);

  EEPROMWriteString(addr, MQTTServerName, (size_t)MAX_NAME_LENGTH);
  addr += MAX_NAME_LENGTH;
  DEBUGS1(F("MQTTServerName: "));
  DEBUGSL1(MQTTServerName);

  EEPROMWriteString(addr, ascomName, (size_t)MAX_NAME_LENGTH);
  addr += MAX_NAME_LENGTH;
  DEBUGS1(F("ascomName: "));
  DEBUGSL1(ascomName);

  //Write character to say the data is saved.
  EEPROM.put(0, '*');
  Serial.printf_P(PSTR("Saved values are: myHostname: %s, sensorHostname:%s, shutterHostname: %s, MQTT: %s, thisID: %s ASCOM name: %s\n"), myHostname, sensorHostname, shutterHostname, MQTTServerName, thisID, ascomName);
  //commit changes
  writeSetupExtension();
  const bool saved = EEPROM.commit();
  DEBUGSL1(F("Save to EEprom complete"));
  return saved;
}

//Function to setup ASCOM Dome default settings, use prior to reading the EEPROM settings which then override them.
/**
 * @brief Populate the dome configuration with default names, positions, network settings and movement options.
 * @details Populate the dome configuration with default names, positions, network settings and movement options.
 * @par Arguments
 * None.
 * @return (void) Observable effects are described above.
 */
void setupDefaults()
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  DEBUGSL1(F("setupDefaults entered"));

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
  slewing = false; //not really used - we use the live state to provide this
  domeStatus = DOME_IDLE;
  shutterStatus = SHUTTER_CLOSED;

  //Strings
  if (myHostname != nullptr)
    free(myHostname);
  myHostname = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  strcpy(myHostname, String(defaultHostname).c_str()); //we do this to make use of the F() for putting const char* into PROGMEM

  //MQTT thisID copied from hostname
  if (thisID != nullptr)
    free(thisID);
  thisID = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  strcpy(thisID, String(myHostname).c_str());

  DEBUGS1(F("myHostname :"));
  DEBUGSL1(myHostname);

#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  if (sensorHostname != nullptr)
    free(sensorHostname);
  sensorHostname = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  strcpy(sensorHostname, String(defaultSensorHostname).c_str());
  DEBUGS1(F("sensorHostname :"));
  DEBUGSL1(sensorHostname);
#endif

  if (shutterHostname != nullptr)
    free(shutterHostname);
  shutterHostname = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  strcpy(shutterHostname, String(defaultShutterHostname).c_str());
  DEBUGS1(F("shutterHostname :"));
  DEBUGSL1(shutterHostname);

  if (MQTTServerName != nullptr)
    free(MQTTServerName);
  MQTTServerName = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  strcpy(MQTTServerName, String(mqtt_server).c_str());
  DEBUGS1(F("MQTTServerName :"));
  DEBUGSL1(MQTTServerName);

  if (ascomName != nullptr)
    free(ascomName);
  ascomName = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
  strcpy_P(ascomName, defaultAscomName);
  DEBUGS1(F("ascomName :"));
  DEBUGSL1(ascomName);

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
  DEBUGSL1(F("SetupDefaults complete"));
}

/**
 * @brief Read the bytes of a float from the requested EEPROM address.
 * @details Read the bytes of a float from the requested EEPROM address.
 * @param[in] addr (uint8_t) Starting EEPROM byte address.
 * @return (float) Float reconstructed from the stored bytes.
 */
float readFloatFromEeprom(uint8_t addr)
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("addr=%g", (double)addr);
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  unsigned int i = 0;
  float value = 0.0F;
  uint8_t *ptr = (uint8_t *)malloc(sizeof(float));

  for (i = 0; i < sizeof(float); i++)
  {
    ptr[i] = EEPROM.read(addr++);
  }
  value = *((float *)ptr);
  free(ptr);
  return value;
}

/**
 * @brief Write the bytes of a float to the requested EEPROM address.
 * @details Write the bytes of a float to the requested EEPROM address; the caller controls committing storage.
 * @param[in] addr (uint8_t) Starting EEPROM byte address.
 * @param[in] value (float) Operation value or field text.
 * @return (void) Observable effects are described above.
 */
void writeFloatToEeprom(uint8_t addr, float value)
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("addr=%g", (double)addr);
                        debugV("value=%g", (double)value);
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  unsigned int i = 0;
  uint8_t *ptr = (uint8_t *)malloc(sizeof(float));

  *((float *)ptr) = value;
  for (i = 0; i < sizeof(float); i++)
  {
    EEPROM.write(addr++, ptr[i]);
  }
  free(ptr);
}

/**
 * @brief Read the stored integer bytes at the requested EEPROM address.
 * @details Read the stored integer bytes at the requested EEPROM address.
 * @param[in] addr (uint8_t) Starting EEPROM byte address.
 * @return (int) Integer reconstructed from the stored bytes.
 */
int readIntFromEeprom(uint8_t addr)
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("addr=%g", (double)addr);
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  unsigned int i = 0;
  int value = 0.0F;
  uint8_t *ptr = (uint8_t *)malloc(sizeof(int));

  for (i = 0; i < sizeof(int); i++)
  {
    ptr[i] = EEPROM.read(addr++);
  }
  value = *((int *)ptr);
  free(ptr);
  return value;
}

/**
 * @brief Write the integer bytes at the requested EEPROM address.
 * @details Write the integer bytes at the requested EEPROM address; the caller controls committing storage.
 * @param[in] addr (uint8_t) Starting EEPROM byte address.
 * @param[in] value (int) Operation value or field text.
 * @return (void) Observable effects are described above.
 */
void writeIntToEeprom(uint8_t addr, int value)
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("addr=%g", (double)addr);
                        debugV("value=%g", (double)value);
                        debugV("config azimuth=%g home=%d park=%d", (double)currentAzimuth, (int)homePosition, (int)parkPosition);
                      });
  unsigned int i = 0;
  uint8_t *ptr = (uint8_t *)malloc(sizeof(int));

  *((int *)ptr) = value;
  for (i = 0; i < sizeof(int); i++, addr++)
  {
    EEPROM.write(addr, ptr[i]);
  }
  free(ptr);
}

#endif
