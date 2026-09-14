/*
File to be included into relevant device REST setup
*/
//Assumes Use of ARDUINO ESP8266WebServer for entry handlers
#if !defined _ESP8266_DomeHandler_h_
#define _ESP8266_DomeHandler_h_
#include "AlpacaErrorConsts.h"
#include "ESP8266_AscomDome.h"
#include "ASCOMAPIDOME_rest.h" //Definitions for functions
#include "JSONHelperFunctions.h"
#include "AlpacaRequestValidation.h"

#include <LinkedList.h>

bool hasArgIC(String &check, ESP8266WebServer &ref, bool caseSensitive);

//Parse the string supplied by ESP8266WebServer, preserving all 32 unsigned bits.
bool checkUint32(const String &inval, uint32_t &out)
{
  return parseAlpacaUint32(inval.c_str(), inval.length(), out);
}

bool validateClientTransactionIds(uint32_t &clientID, uint32_t &transID, const __FlashStringHelper *operation)
{
  String clientKey = F("ClientID");
  String transactionKey = F("ClientTransactionID");
  const bool hasClient = hasArgIC(clientKey, server, false);
  const bool hasTransaction = hasArgIC(transactionKey, server, false);
  const String clientText = hasClient ? server.arg(clientKey) : String();
  const String transactionText = hasTransaction ? server.arg(transactionKey) : String();
  const String operationName(operation);
  debugV("REST Arguments of ClientID: %s and ClientTransactionID: %s for operation %s\n",
         clientText.c_str(), transactionText.c_str(), operationName.c_str());

  const bool clientIDValid = checkUint32(clientText, clientID);
  const bool transIDValid = checkUint32(transactionText, transID);

  debugV("Resolved arguments of ClientID: %u and ClientTransactionID: %u for operation %s\n", clientID, transID, operationName.c_str());

  if (clientIDValid && transIDValid)
    return true;

  debugV("Failed argument validation check for uint_32t\n");

  String message;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, operation, invalidValue, F("Invalid ClientID or ClientTransactionID"));
  serializeJson(root, message);
  server.send(400, F("application/json"), message);
  return false;
}

void handleAltitudeGet(void)
{
  String message;
  //TEST
  uint32_t clientID = 0;
  uint32_t transID = 0;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  if (!validateClientTransactionIds(clientID, transID, F("AltitudeGet")))
    return;

  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("AltitudeGet"), Success, "");

  root["Value"] = altitude;
  //JsonArray& offsets = root.createNestedArray("Value");
  serializeJson(root, message);
  debugI("AltitudeGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleAtHomeGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("AtHome")))
    return;
  boolean tempHome = false;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("AtHome"), 0, "");
  float delta = currentAzimuth - homePosition;

  if ((abs(delta) <= acceptableAzimuthError) || ((360.0 - abs(delta)) <= acceptableAzimuthError))
  {
    tempHome = true;
  }
  else
  {
    tempHome = false;
  }
  root["Value"] = tempHome;
  // replaced .. root["Value"] = ( abs( currentAzimuth - homePosition ) <= acceptableAzimuthError )? atHome = true : atHome = false;
  serializeJson(root, message);
  debugI("AtHomeGet : %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleAtParkGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("AtPark")))
    return;
  boolean tempPark = false;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("AtPark"), 0, "");
  float delta = currentAzimuth - parkPosition;

  if ((abs(delta) <= acceptableAzimuthError) || ((360.0 - abs(delta)) <= acceptableAzimuthError))
  {
    tempPark = true;
  }
  else
  {
    tempPark = false;
  }
  root["Value"] = tempPark;
  serializeJson(root, message);
  debugI("AtParkGet: %s", message.c_str());

  return;
}

void handleAzimuthGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("Azimuth")))
    return;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("Azimuth"), 0, "");
  root["Value"] = normaliseFloat(currentAzimuth, 360.0);
  //JsonArray& offsets = root.createNestedArray("Value");
  serializeJson(root, message);
  debugI("AzimuthGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleCanFindHomeGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("CanFindHome")))
    return;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CanFindHome"), 0, "");
  root["Value"] = canFindHome;
  //JsonArray& offsets = root.createNestedArray("Value");
  serializeJson(root, message);
  debugI("CanFindHomeGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleCanParkGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("CanPark")))
    return;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CanPark"), 0, "");
  root["Value"] = canPark;
  //JsonArray& offsets = root.createNestedArray("Value");
  serializeJson(root, message);
  debugI("CanParkGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleCanSetAltitudeGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("CanSetAltitude")))
    return;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CanSetAltitude"), 0, "");
  root["Value"] = canSetAltitude;
  //JsonArray& offsets = root.createNestedArray("Value");
  serializeJson(root, message);
  debugI("CanSetAltitudeGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleCanSetAzimuthGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("CanSetAzimuth")))
    return;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CanSetAzimuth"), Success, "");
  root["Value"] = canSetAzimuth;
  serializeJson(root, message);
  debugI("CanSetAzimuthGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleCanSetParkGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("CanSetPark")))
    return;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CanSetPark"), 0, "");
  root["Value"] = canSetPark;
  serializeJson(root, message);
  debugI("CanSetParkGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleCanSetShutterGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("CanSetShutter")))
    return;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CanSetShutter"), Success, "");
  root["Value"] = canSetShutter;
  //JsonArray& offsets = root.createNestedArray("Value");
  serializeJson(root, message);
  debugI("CanSetShutterGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleCanSlaveGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("CanSlave")))
    return;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CanSlave"), Success, "");
  root["Value"] = canSlave;
  serializeJson(root, message);
  debugI("CanSlaveGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleCanSyncAzimuthGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("CanSyncAzimuth")))
    return;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CanSyncAzimuth"), Success, "");
  root["Value"] = canSyncAzimuth;
  serializeJson(root, message);
  debugI("CanSyncAzimuthGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

//'Slaved' indicates a hardware link between the telescope and dome  - the ASCOM link is not the same.
void handleSlavedGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("Slaved")))
    return;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  root["Value"] = slaved;
  serializeJson(root, message);
  debugI("SlavedGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleSlavedPut(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("Slaved")))
    return;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("Slaved"), notImplemented, F("Not Implemented"));
  root["Value"] = false;
  serializeJson(root, message);
  debugI("SlavedPut: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleSlewingGet(void)
{
  String message;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("Slewing")))
    return;

  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("Slewing"), Success, "");
  root["Value"] = (domeStatus == DOME_SLEWING || shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_CLOSING || shutterStatus == SHUTTER_ABORTING);
  serializeJson(root, message);
  debugD("SlewingGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleAbortSlewPut(void)
{
  String message;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("AbortSlew")))
    return;

#if defined ACCEPT_CONNECTED_CLIENT_ONLY
  if (connected != clientID)
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("AbortSlew"), notConnected, F("Client not connected"));
  }

  //01/06/2021 MH commented out  - throwing exception causes conform to get upset.
  //else if( domeStatus != DOME_SLEWING  )
  //{
  //  jsonResponseBuilder( root, clientID, transID, ++serverTransID, F("AbortSlew"), invalidOperation, F("Not slewing") );
  //}
  else
#endif
  {
    /* Just set status to DOME_ABORT and next top level loop will cause DOME_ABORT processing regardless of contents in
       command stack. Next command will then be popped. Desirable behaviour ?
       We do 'abort' due to a failed movement, so we could put it to the top of the command stack but unless we
       continually peek the next command, we wont see it until the current movement you actually want aborted
       has finished
      //addDomeCmd( clientID, transID, "", CMD_DOME_ABORT, 0 );
    */
    domeStatus = DOME_ABORT;
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("AbortSlew"), Success, "");
  }
  root["value"] = connected; //return the current connected Client ID.
  serializeJson(root, message);
  debugI("AbortSlewPut: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleShutterStatusGet(void)
{
  String message;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("ShutterStatus")))
    return;

  //anyone can get the shutter status.
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("ShutterStatus"), Success, "");

  root["Value"] = (shutterStatus <= SHUTTER_ERROR) ? static_cast<int>(shutterStatus) : static_cast<int>(SHUTTER_ERROR);
  //root.set<int>("Value", )
  //0 = Open, 1 = Closed, 2 = Opening, 3 = Closing, 4 = Shutter status error
  //JsonArray& offsets = root.createNestedArray("Value");
  serializeJson(root, message);
  debugI("ShutterStatusGet: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleCloseShutterPut(void)
{
  String message;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("CloseShutter")))
    return;

#if defined ACCEPT_CONNECTED_CLIENT_ONLY
  if (connected != clientID)
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CloseShutter"), notConnected, F("Not the connected client"));
  else
#endif
      if (shutterStatus == SHUTTER_CLOSED || shutterStatus == SHUTTER_CLOSING)
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CloseShutter"), Success, "");
    debugD("CloseShutterPut: Already closing or closed - skipped. ");
  }
  else if (shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_OPEN)
  {
    //Set command to close shutter.
    addShutterCmd(clientID, transID, "", CMD_SHUTTER_CLOSE, 0);
    debugD("CloseShutterPut: Added async command to close");
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CloseShutter"), Success, "");
  }
  else
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("CloseShutter"), invalidOperation, F("Dome shutter not idle or errored"));
  }

  //JsonArray& offsets = root.createNestedArray("Value");
  serializeJson(root, message);
  debugI("CloseShutterPut: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

//Kick off the find home operation ???
void handleFindHomePut(void)
{
  String message;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("FindHome")))
    return;

#if defined ACCEPT_CONNECTED_CLIENT_ONLY
  if (connected != clientID)
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("FindHome"), notConnected, F("Not connected"));
  }
  else
#endif
  {
    //Set command to move to home.
    addDomeCmd(clientID, transID, "", CMD_DOME_SLEW, homePosition);
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("FindHome"), Success, "");
  }
  serializeJson(root, message);
  debugI("FindHomePut: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleOpenShutterPut(void)
{
  String message;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("OpenShutter")))
    return;

#if defined ACCEPT_CONNECTED_CLIENT_ONLY
  //in this driver model,each device has a separate ip address ,so can only be one device. hence ignore device-number
  if (connected != clientID)
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("OpenShutter"), notConnected, F("Not the connected client"));
  else
#endif
      if (shutterStatus == SHUTTER_OPEN)
  {
    //We do this because the shutter reports open for any state but closed and we may stop it at any point
    //Set command to open shutter.
    addShutterCmd(clientID, transID, "", CMD_SHUTTER_OPEN, 0);
    debugD("OpenShutterPut: Already open - requesting anyway. ");
  }
  else if (shutterStatus == SHUTTER_OPENING)
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("OpenShutter"), Success, "");
    debugD("OpenShutterPut: Already opening - ignoring additional request.");
  }
  else if (shutterStatus == SHUTTER_CLOSING || shutterStatus == SHUTTER_CLOSED)
  {
    //Set command to open shutter.
    addShutterCmd(clientID, transID, "", CMD_SHUTTER_OPEN, 0);
    debugD("OpenShutterPut: Added async command to open");
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("OpenShutter"), Success, "");
  }
  else
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("OpenShutter"), invalidOperation, F("Dome shutter not idle or errored"));
  }
  serializeJson(root, message);
  debugI("OpenShutterPut: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

//Set park operates when the dome is at the desired park position
/*
 * No argument provided - uses current azimuth and altitude.
 */
void handleSetParkPut(void)
{
  String message;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("SetPark")))
    return;

#if defined ACCEPT_CONNECTED_CLIENT_ONLY
  if (connected != clientID)
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SetPark"), notConnected, F("Not the connected client"));
  else
#endif
  {
    //Set new park location.
    parkPosition = currentAzimuth;
    addDomeCmd(clientID, transID, F("parkPosition"), CMD_DOMEVAR_SET, parkPosition);
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SetPark"), Success, "");
  }

  serializeJson(root, message);
  debugI("SetParkPut: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

//ParkPut causes the dome to move to the already stored Park position
void handleParkPut(void)
{
  String message;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("Park")))
    return;

#if defined ACCEPT_CONNECTED_CLIENT_ONLY
  if (connected != clientID)
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAltitude"), notConnected, F("Not the connected client"));
  //01/06/2021 added to respect canSetAltitude response for Conform
  else
#endif
  {
    addDomeCmd(clientID, transID, "", CMD_DOME_SLEW, parkPosition);
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("Park"), Success, "");
  }

  serializeJson(root, message);
  debugI("ParkPut: %s", message.c_str());
  server.send(200, "application/json", message);
  return;
}

void handleSlewToAltitudePut(void)
{
  String message;
  float location = 0.0F;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  String argsToSearchFor[] = {"clientID", "clientTransactionID", "altitude"};
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("SlewToAltitude")))
    return;

  if (hasArgIC(argsToSearchFor[2], server, false))
    location = (boolean)server.arg(argsToSearchFor[2]).toFloat();

#if defined ACCEPT_CONNECTED_CLIENT_ONLY
  if (connected != clientID)
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAltitude"), notConnected, F("Not the connected client"));
  //01/06/2021 added to respect canSetAltitude response for Conform
  else
#endif
      if (canSetAltitude == false)
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAltitude"), notImplemented, F("Slew to altitude not implemented (yet)"));
  }
  else if (server.hasArg(argsToSearchFor[2]))
  {
    //01/06/2021 Added to align with slewtoAzimuth for Confirm compliance.
    if (location < SHUTTER_MIN_ALTITUDE || location > SHUTTER_MAX_ALTITUDE)
    {
      String shutterError = "New altitude out of range 0<=value<=";
      shutterError.concat(SHUTTER_MAX_ALTITUDE);
      jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAltitude"), invalidValue, shutterError.c_str());
    }
    else
    { //Set new shutter altitude.
      normaliseFloat(location, SHUTTER_MAX_ALTITUDE);
      addShutterCmd(clientID, transID, "altitude", CMD_SHUTTERVAR_SET, location);
      jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAltitude"), Success, "");
    }
  }
  else
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAltitude"), valueNotSet, F("Altitude not provided"));

  serializeJson(root, message);
  debugI("SlewToAltitudePut: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

/*
 * The handler receives target Azimuth in real degrees
 */
void handleSlewToAzimuthPut(void)
{
  String message;

  String argsToSearchFor[] = {"clientID", "clientTransactionID", "Azimuth"};
  uint32_t clientID = 0;
  uint32_t transID = 0;

  float location = 0.0F;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  if (!validateClientTransactionIds(clientID, transID, F("SlewToAzimuth")))
    return;

  if (hasArgIC(argsToSearchFor[2], server, false))
    location = server.arg(argsToSearchFor[2]).toFloat();

  debugI("SlewToAzimuthPut: %f", location);

#if defined ACCEPT_CONNECTED_CLIENT_ONLY
  if (connected != clientID)
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAzimuth"), notConnected, F("Not the connected client"));
  }
  else
#endif
      if (canSetAzimuth == false)
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAzimuth"), notImplemented, F("Not supported by dome"));
  }
  else if (server.hasArg(argsToSearchFor[2]))
  {
    //01/06/2021 Added to comply with Conform requirements
    if (location < 0.0F || location > 360.0F)
      jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAzimuth"), invalidValue, F("New azimuth out of 0<= value<=360 range"));
    else
    {
      //Set new slew location.
      normaliseFloat(location, 360.0F);
      debugD("SlewToAzimuthPut: %f", location);
      addDomeCmd(clientID, transID, F("SlewToAzimuthPut"), CMD_DOME_SLEW, location);
      jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAzimuth"), Success, "");
    }
  }
  else
  {
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SlewToAzimuth"), valueNotSet, F("Azimuth argument not found"));
  }

  serializeJson(root, message);
  debugI("SlewToAzimuthPut: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

/*
 * The handler receives target Azimuth in decimal degrees.
 */
void handleSyncToAzimuthPut(void)
{
  String message;
  float location = 0;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  String argsToSearchFor[] = {"clientID", "clientTransactionID", "Azimuth"};
  uint32_t clientID = 0;
  uint32_t transID = 0;

  if (!validateClientTransactionIds(clientID, transID, F("SyncToAzimuth")))
    return;

  if (hasArgIC(argsToSearchFor[2], server, false))
    location = server.arg(argsToSearchFor[2]).toFloat();

#if defined ACCEPT_CONNECTED_CLIENT_ONLY
  if (connected != clientID)
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SyncToAzimuth"), notConnected, F("Not the connected client"));
  else
#endif
      if (server.hasArg(argsToSearchFor[2]))
  {
    //enforce limits by exception to satisfy Conform compliance
    if (location < 0.0F || location > 360.0F)
      jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SyncToAzimuth"), invalidValue, F("Sync to value outside range 0<=value<=360"));
    else
    {
      //Set new (actual) azimuth location.
      normaliseFloat(location, 360.0F);

      //The offset is the difference between where the dome says it is currently pointing and where we are told it is pointing.
      azimuthSyncOffset = (float)(location - bearing);
      debugD("azimuthsync - input: %3.2f, currentAz: %3.2f, offset %3.2f", location, currentAzimuth, azimuthSyncOffset);
      //Because we can go out of range we do this again
      normaliseFloat(azimuthSyncOffset, 360.0F);
      debugD("azimuthsync - normalised offset: %f ", azimuthSyncOffset);

      addDomeCmd(clientID, transID, F("azimuthSyncOffset"), CMD_DOMEVAR_SET, int(azimuthSyncOffset));

      //01/06/2021 Removed - results in potential unexpected motion after sync.
      //Tell the dome to move to the target azimuth using the updated correction - should be trivial.
      //addDomeCmd( clientID, transID, "", CMD_DOME_SLEW, (int) getAzimuth( bearing ) );

      jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SyncToAzimuth"), Success, "");
    }
  }
  else
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("SyncToAzimuth"), valueNotSet, F("Argument not found"));

  serializeJson(root, message);
  debugI("SyncToAzimuthPut: %s", message.c_str());
  server.send(200, F("application/json"), message);
  return;
}

void handleConnectPut(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("Connect")))
    return;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  if (connectionStatus == CONNECTION_CONNECTED && connected != clientID)
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("Connect"), invalidOperation, F("Another client is already connected"));
  else
  {
    pendingConnectionClientID = clientID;
    connectionStatus = (connected == clientID) ? CONNECTION_CONNECTED : CONNECTION_CONNECTING;
    connectionStateChangedAt = millis();
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("Connect"), Success, "");
  }
  serializeJson(root, message);
  server.send(200, F("application/json"), message);
}

void handleDisconnectPut(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("Disconnect")))
    return;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  if (connected != NOT_CONNECTED && connected != clientID)
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("Disconnect"), notConnected, F("This client does not own the connection"));
  else
  {
    pendingConnectionClientID = clientID;
    connectionStatus = (connected == NOT_CONNECTED) ? CONNECTION_DISCONNECTED : CONNECTION_DISCONNECTING;
    connectionStateChangedAt = millis();
    jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("Disconnect"), Success, "");
  }
  serializeJson(root, message);
  server.send(200, F("application/json"), message);
}

void handleConnectingGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("Connecting")))
    return;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("Connecting"), Success, "");
  root["Value"] = (connectionStatus == CONNECTION_CONNECTING || connectionStatus == CONNECTION_DISCONNECTING);
  serializeJson(root, message);
  server.send(200, F("application/json"), message);
}

void handleDeviceStateGet(void)
{
  String message;
  uint32_t clientID = 0;
  uint32_t transID = 0;
  if (!validateClientTransactionIds(clientID, transID, F("DeviceState")))
    return;

  JsonDocument snapshotDoc;
  JsonObject snapshot = snapshotDoc.to<JsonObject>();
  appendDomeStatusFields(snapshot);

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, clientID, transID, ++serverTransID, F("DeviceState"), Success, "");
  JsonArray values = root["Value"].to<JsonArray>();
  for (JsonPair pair : snapshot)
  {
    JsonObject stateValue = values.add<JsonObject>();
    stateValue["Name"] = pair.key();
    stateValue["Value"].set(pair.value());
  }

  serializeJson(root, message);
  server.send(200, F("application/json"), message);
}

#endif
