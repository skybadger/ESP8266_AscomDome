#ifndef ASCOM_DOME_SETUP_MODERN_H
#define ASCOM_DOME_SETUP_MODERN_H

//Shared layout and field handling follow the Switch driver's device/driver pages.
//Only this registration function knows which driver is installed on the hardware.
#include <math.h>
#include <stdlib.h>

enum SetupKind
{
  SETUP_TEXT,
  SETUP_HOST,
  SETUP_PEER,
  SETUP_INT,
  SETUP_FLOAT,
  SETUP_BOOL
};
struct SetupField
{
  const char *key;
  const char *label;
  const char *help;
  SetupKind kind;
  bool management;
  double minimum;
  double maximum;
};
static const SetupField setupFields[] = {
    {"hostname", "Device hostname", "Saved for the next restart. Use letters, digits and hyphens.", SETUP_HOST, true, 0, 0},
    {"location", "Device location", "Reported by the Alpaca management API. May be left blank.", SETUP_TEXT, true, 0, 0},
    {"udpport", "UDP discovery listening port", "Default 32227. Clients must discover on the same port. Replies return to the requesting client; HTTP remains on port 80.", SETUP_INT, true, 1, 65535},
    {"mqttserver", "MQTT server", "Hostname or IPv4 address. Used on the next MQTT reconnect or device restart.", SETUP_PEER, true, 0, 0},
    {"ascomname", "Dome name", "Name reported for this driver in the configured device list.", SETUP_TEXT, false, 0, 0},
    {"shutterhostname", "Shutter controller", "Hostname or IPv4 address, without http:// or a path.", SETUP_PEER, false, 0, 0},
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
    {"sensorhostname", "Bearing sensor", "Hostname or IPv4 address, without http:// or a path.", SETUP_PEER, false, 0, 0},
#endif
    {"homeposition", "Home position (degrees)", "Whole degrees from 0 to 359. Saving does not move the dome.", SETUP_INT, false, 0, 359},
    {"parkposition", "Park position (degrees)", "Whole degrees from 0 to 359. Saving does not move the dome.", SETUP_INT, false, 0, 359},
    {"syncoffset", "Bearing offset (degrees)", "Added to the raw bearing. This sets the offset directly, not an absolute sync position.", SETUP_FLOAT, false, -360, 360},
    {"parkondisconnect", "Park on disconnect", "Park the dome when its connected client disconnects.", SETUP_BOOL, false, 0, 1},
    {"closeondisconnect", "Close shutter on disconnect", "Close the shutter when its connected client disconnects.", SETUP_BOOL, false, 0, 1}};

String setupEscape(const String &input)
{
  String result;
  result.reserve(input.length() + 16);
  for (size_t i = 0; i < input.length(); ++i)
  {
    switch (input[i])
    {
    case '&':
      result += F("&amp;");
      break;
    case '<':
      result += F("&lt;");
      break;
    case '>':
      result += F("&gt;");
      break;
    case '"':
      result += F("&quot;");
      break;
    case '\'':
      result += F("&#39;");
      break;
    default:
      result += input[i];
    }
  }
  return result;
}

char *setupTextStorage(const String &key)
{
  if (key == "hostname")
    return myHostname;
  if (key == "location")
    return deviceLocation;
  if (key == "mqttserver")
    return MQTTServerName;
  if (key == "ascomname")
    return ascomName;
  if (key == "shutterhostname")
    return shutterHostname;
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
  if (key == "sensorhostname")
    return sensorHostname;
#endif
  return nullptr;
}

String setupValue(const SetupField &field)
{
  const String key(field.key);
  if (field.kind <= SETUP_PEER)
  {
    const char *value = setupTextStorage(key);
    return value ? String(value) : String();
  }
  if (key == "udpport")
    return String(udpPort);
  if (key == "homeposition")
    return String(homePosition);
  if (key == "parkposition")
    return String(parkPosition);
  if (key == "syncoffset")
    return String(azimuthSyncOffset, 6);
  if (key == "parkondisconnect")
    return parkDomeOnDisconnect ? "true" : "false";
  return closeShutterOnDisconnect ? "true" : "false";
}

//Validate the complete input before modifying any variable or EEPROM byte.
bool setupValid(const SetupField &field, const String &value)
{
  if (field.kind <= SETUP_PEER)
  {
    if (value.length() >= MAX_NAME_LENGTH || (!value.length() && String(field.key) != "location"))
      return false;
    for (size_t i = 0; i < value.length(); ++i)
    {
      const unsigned char ch = value[i];
      if (ch < 32 || ch == 127)
        return false;
      if (field.kind == SETUP_HOST || field.kind == SETUP_PEER)
      {
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '-' || (field.kind == SETUP_PEER && ch == '.')))
          return false;
      }
    }
    if (field.kind == SETUP_HOST || field.kind == SETUP_PEER)
    {
      if (value[0] == '-' || value[value.length() - 1] == '-' || value[0] == '.' ||
          value[value.length() - 1] == '.' || value.indexOf("..") >= 0 ||
          value.indexOf(".-") >= 0 || value.indexOf("-.") >= 0)
        return false;
    }
    return true;
  }
  if (field.kind == SETUP_BOOL)
    return value == "true" || value == "false";
  if (!value.length())
    return false;
  //Disallow whitespace, hex, NaN and partially parsed numbers.
  for (size_t i = 0; i < value.length(); ++i)
  {
    const char ch = value[i];
    if (!(ch >= '0' && ch <= '9') && !(ch == '-' && i == 0 && field.minimum < 0) &&
        !(ch == '.' && field.kind == SETUP_FLOAT))
      return false;
  }
  char *end = nullptr;
  const double number = strtod(value.c_str(), &end);
  return end != value.c_str() && *end == '\0' && isfinite(number) &&
         number >= field.minimum && number <= field.maximum &&
         (field.kind != SETUP_INT || floor(number) == number);
}

void setupAssign(const SetupField &field, const String &value)
{
  const String key(field.key);
  if (field.kind <= SETUP_PEER)
  {
    char *destination = setupTextStorage(key);
    memset(destination, 0, MAX_NAME_LENGTH);
    memcpy(destination, value.c_str(), value.length());
  }
  else if (key == "udpport")
    udpPort = value.toInt();
  else if (key == "homeposition")
    homePosition = value.toInt();
  else if (key == "parkposition")
    parkPosition = value.toInt();
  else if (key == "syncoffset")
    azimuthSyncOffset = value.toFloat();
  else if (key == "parkondisconnect")
    parkDomeOnDisconnect = value == "true";
  else if (key == "closeondisconnect")
    closeShutterOnDisconnect = value == "true";
}

void sendSetupPage(bool management, int status = 200, const String &message = "")
{
  server.sendHeader(F("Cache-Control"), F("no-store"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(status, F("text/html; charset=utf-8"), "");
  server.sendContent(F("<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                       "<title>Skybadger dome setup</title><style>"
                       "*{box-sizing:border-box}body{margin:0;background:#f2f5f8;color:#172b3a;font:16px system-ui,sans-serif}"
                       "header{background:#173c50;color:white;padding:24px}main{max-width:900px;margin:auto;padding:20px}"
                       "nav{display:flex;gap:20px;flex-wrap:wrap}nav a{color:#d5f3ff}h1{font-size:1.6rem}h2{font-size:1.2rem}"
                       "form,.card{background:white;border:1px solid #d2dce4;border-radius:8px;padding:18px;margin:14px 0}"
                       "label{display:block;font-weight:600;margin-bottom:8px}input,select,button{font:inherit;padding:10px;border:1px solid #8b9faa;border-radius:4px}"
                       "input,select{width:min(100%,460px)}button{background:#176685;color:white;cursor:pointer;margin:8px 0}"
                       "p{line-height:1.5}.help{color:#465f6d;font-size:.9rem}.notice{border-left:5px solid #176685;padding:16px;background:#e3f2f8}"
                       "a{color:#115878}:focus-visible{outline:3px solid #db8b13;outline-offset:3px}"
                       "@media(max-width:550px){input,select,button{width:100%}}"
                       "</style></head><body><header><nav><a href='/setup'>Device management</a>"
                       "<a href='/setup/v1/dome/1/setup'>Dome setup</a><a href='/status'>Live status (JSON)</a></nav>"));
  server.sendContent(management ? F("<h1>Device management</h1>") : F("<h1>Dome driver setup</h1>"));
  server.sendContent(F("</header><main>"));
  if (message.length())
    server.sendContent(String(F("<p class='notice' role='status'>")) + setupEscape(message) + F("</p>"));
  if (management)
  {
    server.sendContent(String(F("<section class='card'><h2>Installed driver</h2><p><a href='/setup/v1/dome/1/setup'>")) +
                       setupEscape(ascomName ? ascomName : "Dome") + F("</a> &middot; Dome 1</p><p>This hardware runs one driver at a time.</p><p>Running hostname: ") +
                       setupEscape(WiFi.hostname()) + F(" &middot; IP: ") + WiFi.localIP().toString() +
                       F("</p><p><a href='/management/v1/description'>Management information</a> &middot; <a href='/management/v1/configureddevices'>Configured devices</a></p></section>"));
  }
  else
  {
    server.sendContent(F("<p>Read and update the saved dome settings below. Settings can be changed when the dome and shutter are stationary and no commands are queued.</p>"
                         "<p><a href='/setup/v1/dome/1/config'>Read dome settings as JSON</a></p>"));
    server.sendContent(String(F("<section class='card'><h2>Driver information</h2><p>Device: Dome 1 &middot; Version: ")) +
                       setupEscape(DriverVersion) + F("</p><p>Unique ID: ") + setupEscape(GUID) +
                       F("</p><p>Capabilities are fixed by this firmware. See <a href='/setup/v1/dome/1/config'>dome settings and capabilities</a> for the supported features.</p></section>"));
  }
  const String action = management ? "/setup/config" : "/setup/v1/dome/1/config";
  for (const auto &field : setupFields)
  {
    if (field.management != management)
      continue;
    const String value = setupEscape(setupValue(field));
    String row = F("<form method='post' action='");
    row += action;
    row += F("'><input type='hidden' name='field' value='");
    row += field.key;
    row += F("'><label for='");
    row += field.key;
    row += F("'>");
    row += field.label;
    row += F("</label>");
    if (field.kind == SETUP_BOOL)
    {
      row += F("<select name='value' id='");
      row += field.key;
      row += F("'>");
      row += value == "true" ? F("<option value='true' selected>Enabled</option><option value='false'>Disabled</option>") : F("<option value='true'>Enabled</option><option value='false' selected>Disabled</option>");
      row += F("</select>");
    }
    else
    {
      row += F("<input name='value' id='");
      row += field.key;
      row += F("' value='");
      row += value;
      row += F("' type='");
      row += field.kind <= SETUP_PEER ? "text" : "number";
      row += F("' ");
      if (String(field.key) != "location")
        row += F("required ");
      if (field.kind <= SETUP_PEER)
      {
        row += F("maxlength='");
        row += MAX_NAME_LENGTH - 1;
        row += '\'';
      }
      else
      {
        row += F("min='");
        row += String(field.minimum, 0);
        row += F("' max='");
        row += String(field.maximum, 0);
        row += F("' step='");
        row += field.kind == SETUP_FLOAT ? "any" : "1";
        row += '\'';
      }
      row += '>';
    }
    row += F(" <button type='submit'>Save</button><p class='help'>");
    row += field.help;
    row += F("</p></form>");
    server.sendContent(row);
  }
  if (management)
    server.sendContent(F("<section class='card'><h2>Maintenance</h2><p>Restart to apply a saved hostname or MQTT server change.</p>"
                         "<form method='post' action='/setup/restart'><button type='submit'>Restart device</button></form>"
                         "<a href='/update'>Update firmware</a> &middot; <a href='/setup/config'>Read management settings as JSON</a></section>"));
  server.sendContent(F("</main></body></html>"));
  server.sendContent("");
}

bool setupBusy()
{
  return domeStatus == DOME_SLEWING || domeStatus == DOME_ABORT ||
         shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_CLOSING || shutterStatus == SHUTTER_ABORTING ||
         (domeCmdList && domeCmdList->size()) || (shutterCmdList && shutterCmdList->size());
}

void handleSetupConfig(bool management)
{
  if (server.method() == HTTP_GET)
  {
    JsonDocument doc;
    for (const auto &field : setupFields)
    {
      if (field.management != management)
        continue;
      const String value = setupValue(field);
      if (field.kind == SETUP_BOOL)
        doc[field.key] = value == "true";
      else if (field.kind == SETUP_INT)
        doc[field.key] = value.toInt();
      else if (field.kind == SETUP_FLOAT)
        doc[field.key] = value.toFloat();
      else
        doc[field.key] = value;
    }
    if (management)
    {
      doc["runningHostname"] = WiFi.hostname();
      doc["httpPort"] = 80;
    }
    else
    {
      doc["deviceType"] = "Dome";
      doc["deviceNumber"] = instanceNumber;
      doc["uniqueID"] = GUID;
      doc["driverVersion"] = DriverVersion;
      JsonObject capabilities = doc["capabilities"].to<JsonObject>();
      capabilities["canFindHome"] = canFindHome;
      capabilities["canPark"] = canPark;
      capabilities["canSetAzimuth"] = canSetAzimuth;
      capabilities["canSetAltitude"] = canSetAltitude;
      capabilities["canSetPark"] = canSetPark;
      capabilities["canSetShutter"] = canSetShutter;
      capabilities["canSlave"] = canSlave;
      capabilities["canSyncAzimuth"] = canSyncAzimuth;
    }
    String body;
    serializeJson(doc, body);
    server.sendHeader(F("Cache-Control"), F("no-store"));
    server.send(200, F("application/json"), body);
    return;
  }
  if (server.method() != HTTP_POST && server.method() != HTTP_PUT)
  {
    server.sendHeader(F("Allow"), F("GET, POST, PUT"));
    server.send(405, F("text/plain"), F("Use GET to read, POST or PUT to save."));
    return;
  }
  if (server.args() != 2 || !server.hasArg("field") || !server.hasArg("value"))
  {
    sendSetupPage(management, 400, F("Supply exactly one field and value."));
    return;
  }
  const String key = server.arg("field"), value = server.arg("value");
  for (const auto &field : setupFields)
  {
    if (key != field.key || field.management != management)
      continue;
    if (!setupValid(field, value))
    {
      sendSetupPage(management, 400, String(F("Invalid value for ")) + field.label + F(". Check the limits below."));
      return;
    }
    if (setupBusy())
    {
      sendSetupPage(management, 409, F("Wait until movement and queued commands have finished before saving settings."));
      return;
    }
    if (field.kind <= SETUP_PEER && !setupTextStorage(key))
    {
      sendSetupPage(management, 503, F("Configuration memory is unavailable."));
      return;
    }
    const String previous = setupValue(field);
    if (previous == value)
    {
      sendSetupPage(management, 200, F("Value unchanged."));
      return;
    }
    //Rebind first so a failure cannot leave a persisted but unavailable discovery port.
    if (key == "udpport")
    {
      Udp.stop();
      if (!Udp.begin(value.toInt()))
      {
        Udp.begin(udpPort);
        sendSetupPage(management, 503, F("Could not open the discovery port. Settings unchanged."));
        return;
      }
    }
    setupAssign(field, value);
    if (!saveToEeprom())
    {
      setupAssign(field, previous);
      if (key == "udpport")
      {
        Udp.stop();
        Udp.begin(udpPort);
      }
      sendSetupPage(management, 500, F("Saving to flash failed. Previous running setting restored; retry the save."));
      return;
    }
    if (key == "syncoffset")
      currentAzimuth = getAzimuth(bearing);
    if (key == "hostname")
      sendSetupPage(management, 200, F("Saved. Restart the device to apply the hostname."));
    else if (key == "mqttserver")
      sendSetupPage(management, 200, F("Saved. The MQTT server is used on the next reconnect or restart."));
    else
      sendSetupPage(management, 200, F("Saved and applied."));
    return;
  }
  sendSetupPage(management, 400, F("Unknown setting for this page."));
}

void handleModernConfiguredDevices()
{
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  jsonResponseBuilder(root, server.arg("ClientID").toInt(), server.arg("ClientTransactionID").toInt(),
                      ++serverTransID, "ConfiguredDevices", Success, "");
  JsonObject entry = doc["Value"].to<JsonArray>().add<JsonObject>();
  entry["DeviceName"] = ascomName;
  entry["DeviceType"] = DriverType;
  entry["DeviceNumber"] = instanceNumber;
  entry["UniqueID"] = GUID;
  String body;
  serializeJson(doc, body);
  server.send(200, F("application/json"), body);
}

void handleModernName()
{
  String body;
  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  uint32_t client = 0, transaction = 0;
  if (!validateCommonRequest(body, root, client, transaction))
    return;
  jsonResponseBuilder(root, client, transaction, ++serverTransID, F("Name"), Success, "");
  root["Value"] = ascomName;
  serializeJson(doc, body);
  sendCommonResponse(body);
}

void registerModernSetupRoutes()
{
  server.on("/", HTTP_GET, []()
            {
              sendSetupPage(true);
            });
  server.on("/setup", HTTP_GET, []()
            {
              sendSetupPage(true);
            });
  server.on("/setup/v1/dome/1/setup", HTTP_GET, []()
            {
              sendSetupPage(false);
            });
  server.on("/setup/config", HTTP_ANY, []()
            {
              handleSetupConfig(true);
            });
  server.on("/setup/v1/dome/1/config", HTTP_ANY, []()
            {
              handleSetupConfig(false);
            });
  server.on("/setup/restart", HTTP_POST, []()
            {
              if (setupBusy())
              {
                sendSetupPage(true, 409, F("Wait for movement and queued commands to finish before restarting."));
                return;
              }
              server.send(200, F("text/html"), F("<!doctype html><html lang='en'><title>Restarting</title><p>Device restarting. Reopen setup using its saved hostname or IP address.</p></html>"));
              delay(100);
              device.restart();
            });
}
#endif
