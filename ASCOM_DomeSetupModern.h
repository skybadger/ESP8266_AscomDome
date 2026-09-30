#ifndef ASCOM_DOME_SETUP_MODERN_H
#define ASCOM_DOME_SETUP_MODERN_H

//Shared layout and field handling follow the Switch driver's device/driver pages.
//Only this registration function knows which driver is installed on the hardware.
#include <math.h>
#include <stdlib.h>
#include "DomeBearingSync.h"

void sendSetupPage(bool management, int status, const String &message);
#include "ASCOM_DomeControl.h"

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
// String pointers below refer to flash; access them through FPSTR().
static const char setup_hostnameKey[] PROGMEM = "hostname";
static const char setup_hostnameLabel[] PROGMEM = "Device hostname";
static const char setup_hostnameHelp[] PROGMEM = "Saved for the next restart. Use letters, digits and hyphens.";
static const char setup_locationKey[] PROGMEM = "location";
static const char setup_locationLabel[] PROGMEM = "Device location";
static const char setup_locationHelp[] PROGMEM = "Reported by the Alpaca management API. May be left blank.";
static const char setup_udpportKey[] PROGMEM = "udpport";
static const char setup_udpportLabel[] PROGMEM = "UDP discovery listening port";
static const char setup_udpportHelp[] PROGMEM = "Default 32227. Clients must discover on the same port. Replies return to the requesting client; HTTP remains on port 80.";
static const char setup_mqttserverKey[] PROGMEM = "mqttserver";
static const char setup_mqttserverLabel[] PROGMEM = "MQTT server";
static const char setup_mqttserverHelp[] PROGMEM = "Hostname or IPv4 address. Used on the next MQTT reconnect or device restart.";
static const char setup_ascomnameKey[] PROGMEM = "ascomname";
static const char setup_ascomnameLabel[] PROGMEM = "Dome name";
static const char setup_ascomnameHelp[] PROGMEM = "Name reported for this driver in the configured device list.";
static const char setup_shutterhostnameKey[] PROGMEM = "shutterhostname";
static const char setup_shutterhostnameLabel[] PROGMEM = "Shutter controller";
static const char setup_shutterhostnameHelp[] PROGMEM = "Hostname or IPv4 address, without http:// or a path.";
static const char setup_sensorhostnameKey[] PROGMEM = "sensorhostname";
static const char setup_sensorhostnameLabel[] PROGMEM = "Bearing sensor";
static const char setup_sensorhostnameHelp[] PROGMEM = "Hostname or IPv4 address, without http:// or a path.";
static const char setup_homepositionKey[] PROGMEM = "homeposition";
static const char setup_homepositionLabel[] PROGMEM = "Home position (degrees)";
static const char setup_homepositionHelp[] PROGMEM = "Whole degrees from 0 to 359. Saving does not move the dome.";
static const char setup_parkpositionKey[] PROGMEM = "parkposition";
static const char setup_parkpositionLabel[] PROGMEM = "Park position (degrees)";
static const char setup_parkpositionHelp[] PROGMEM = "Whole degrees from 0 to 359. Saving does not move the dome.";
static const char setup_syncoffsetKey[] PROGMEM = "syncoffset";
static const char setup_syncoffsetLabel[] PROGMEM = "Bearing offset (degrees)";
static const char setup_syncoffsetHelp[] PROGMEM = "Added to the raw bearing. This sets the offset directly, not an absolute sync position.";
static const char setup_parkondisconnectKey[] PROGMEM = "parkondisconnect";
static const char setup_parkondisconnectLabel[] PROGMEM = "Park on disconnect";
static const char setup_parkondisconnectHelp[] PROGMEM = "Park the dome when its connected client disconnects.";
static const char setup_closeondisconnectKey[] PROGMEM = "closeondisconnect";
static const char setup_closeondisconnectLabel[] PROGMEM = "Close shutter on disconnect";
static const char setup_closeondisconnectHelp[] PROGMEM = "Close the shutter when its connected client disconnects.";
static const char setup_bearingKey[] PROGMEM = "bearing";
static const char setup_bearingLabel[] PROGMEM = "Known current bearing";
static const char setup_bearingHelp[] PROGMEM = "";

static const SetupField setupFields[] = {
    {setup_hostnameKey, setup_hostnameLabel, setup_hostnameHelp, SETUP_HOST, true, 0, 0},
    {setup_locationKey, setup_locationLabel, setup_locationHelp, SETUP_TEXT, true, 0, 0},
    {setup_udpportKey, setup_udpportLabel, setup_udpportHelp, SETUP_INT, true, 1, 65535},
    {setup_mqttserverKey, setup_mqttserverLabel, setup_mqttserverHelp, SETUP_PEER, true, 0, 0},
    {setup_ascomnameKey, setup_ascomnameLabel, setup_ascomnameHelp, SETUP_TEXT, false, 0, 0},
    {setup_shutterhostnameKey, setup_shutterhostnameLabel, setup_shutterhostnameHelp, SETUP_PEER, false, 0, 0},
#if defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION || defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION
    {setup_sensorhostnameKey, setup_sensorhostnameLabel, setup_sensorhostnameHelp, SETUP_PEER, false, 0, 0},
#endif
    {setup_homepositionKey, setup_homepositionLabel, setup_homepositionHelp, SETUP_INT, false, 0, 359},
    {setup_parkpositionKey, setup_parkpositionLabel, setup_parkpositionHelp, SETUP_INT, false, 0, 359},
    {setup_syncoffsetKey, setup_syncoffsetLabel, setup_syncoffsetHelp, SETUP_FLOAT, false, -360, 360},
    {setup_parkondisconnectKey, setup_parkondisconnectLabel, setup_parkondisconnectHelp, SETUP_BOOL, false, 0, 1},
    {setup_closeondisconnectKey, setup_closeondisconnectLabel, setup_closeondisconnectHelp, SETUP_BOOL, false, 0, 1}};

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
  const String key(FPSTR(field.key));
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
    if (value.length() >= MAX_NAME_LENGTH || (!value.length() && String(FPSTR(field.key)) != "location"))
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
  const String key(FPSTR(field.key));
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

void sendDomeControlForms()
{
  server.sendContent(String(F("<section class='card' id='bearing-sync'><h2>Synchronise current bearing</h2>"
                              "<p>Enter the dome's known physical bearing. This updates its calibration without moving the dome.</p>"
                              "<form method='post' action='/setup/v1/dome/1/sync#feedback'><label for='sync-bearing'>Known current bearing (&deg;)</label>"
                              "<input id='sync-bearing' name='bearing' type='number' min='0' max='360' step='any' inputmode='decimal' required value='")) +
                     String(currentAzimuth, 3) + F("'><button type='submit'>Synchronise bearing</button>"
                                                   "<p class='help'>Use when stationary with no pending commands. A fresh external sensor reading is required; the calculated offset is saved.</p></form></section>"));
  server.sendContent(String(F("<section class='card' id='dome-controls'><h2>Manual movement</h2><p>Current azimuth: ")) + String(currentAzimuth, 1) +
                     F("&deg; &middot; Active target: ") + String(targetAzimuth, 1) + F("&deg; &middot; ") +
                     setupEscape(domeStateNames[static_cast<int>(domeStatus)]) +
                     F("</p><p><a href='/setup/v1/dome/1/setup'>Refresh status</a></p>"));
#if !defined _ENABLE_DOME
  server.sendContent(F("<p class='notice'>Dome movement is disabled in this firmware build.</p>"));
#endif
#if !defined _ENABLE_SHUTTER
  server.sendContent(F("<p class='notice'>Shutter movement is disabled in this firmware build.</p>"));
#endif
  server.sendContent(String(F("<form method='post' action='/setup/v1/dome/1/control#feedback'>"
                              "<input type='hidden' name='action' value='slew'><label for='dome-target'>Target dome position (&deg;)</label>"
                              "<input id='dome-target' name='value' type='number' min='0' max='360' step='1' inputmode='numeric' required value='")) +
                     String(targetAzimuth, 0) + F("'><button type='submit'>Slew to target</button></form>"
                                                  "<form method='post' action='/setup/v1/dome/1/control#feedback'><p>Jog from the active or last queued target. East increases azimuth; west decreases it.</p>"
                                                  "<div class='button-row'><button name='action' value='west'>Jog west &minus;") +
                     String(domeUiJogDegrees) +
                     F("&deg;</button> <button name='action' value='east'>Jog east +") + String(domeUiJogDegrees) + F("&deg;</button></div></form>"));
  server.sendContent(String(F("<h3 id='shutter-controls'>Shutter</h3><p>")) + setupEscape(shutterStateNames[static_cast<int>(shutterStatus)]) +
                     F("</p><form method='post' action='/setup/v1/dome/1/control#feedback'>"
                       "<div class='button-row'><button name='action' value='open'>Open shutter</button> <button name='action' value='close'>Close shutter</button></div></form>"
                       "<form method='post' action='/setup/v1/dome/1/control#feedback'><input type='hidden' name='action' value='altitude'>"
                       "<label for='shutter-target'>Move shutter to altitude (&deg;)</label><input id='shutter-target' name='value' type='number' min='") +
                     String(SHUTTER_MIN_ALTITUDE) + F("' max='") + String(SHUTTER_MAX_ALTITUDE) + F("' step='1' inputmode='numeric' required value='") +
                     String(targetAltitude, 0) + F("'><button type='submit'>Move to altitude</button>"
                                                   "<p class='help'>Opens or closes toward the entered angle. This is a movement command, not a saved opening limit. Requires altitude support on the remote shutter.</p></form>"
                                                   "<p class='help'>Commands are queued in order. Opening or closing waits for any current shutter movement to complete.</p></section>"));
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
                       ".jump-links a{color:#115878}[id]{scroll-margin-top:20px}"
                       "nav a{display:inline-flex;align-items:center;min-height:44px;padding:8px}nav{gap:8px 16px}"
                       "main{overflow-wrap:anywhere}input,select{min-width:0;max-width:100%}button{min-height:44px;white-space:normal}"
                       ".card form{border:0;border-top:1px solid #d2dce4;border-radius:0;padding:16px 0 0;margin:16px 0 0}"
                       ".button-row{display:flex;flex-wrap:wrap;gap:12px}.button-row button{flex:1 1 140px;margin:4px 0}"
                       "form,.card{background:white;border:1px solid #d2dce4;border-radius:8px;padding:18px;margin:14px 0}"
                       "label{display:block;font-weight:600;margin-bottom:8px}input,select,button{font:inherit;padding:10px;border:1px solid #8b9faa;border-radius:4px}"
                       "input,select{width:min(100%,460px)}button{background:#176685;color:white;cursor:pointer;margin:8px 0}"
                       "p{line-height:1.5}.help{color:#465f6d;font-size:.9rem}.notice{border-left:5px solid #176685;padding:16px;background:#e3f2f8}"
                       "a{color:#115878}:focus-visible{outline:3px solid #db8b13;outline-offset:3px}"
                       "@media(max-width:550px){header{padding:16px}main{padding:12px}form,.card{padding:12px}input,select,button{width:100%}.button-row button{width:auto}}"
                       "</style></head><body><header><nav><a href='/setup'>Device management</a>"
                       "<a href='/setup/v1/dome/1/setup'>Dome setup</a><a href='/status'>Live status (JSON)</a></nav>"));
  server.sendContent(management ? F("<h1>Device management</h1>") : F("<h1>Dome driver setup</h1>"));
  server.sendContent(F("</header><main>"));
  if (!management)
    server.sendContent(F("<nav class='card jump-links' aria-label='Jump to section'><strong>Jump to:</strong>"
                         "<a href='#bearing-sync'>Synchronise bearing</a><a href='#dome-controls'>Dome controls</a>"
                         "<a href='#shutter-controls'>Shutter controls</a><a href='#driver-information'>Driver information</a>"
                         "<a href='#configuration'>Configuration</a></nav>"));
  if (message.length())
    server.sendContent(String(F("<p id='feedback' tabindex='-1' class='notice' role='status'>")) + setupEscape(message) + F("</p>"));
  if (management)
  {
    server.sendContent(String(F("<section class='card'><h2>Installed driver</h2><p><a href='/setup/v1/dome/1/setup'>")) +
                       setupEscape(ascomName ? ascomName : "Dome") + F("</a> &middot; Dome 1</p><p>This hardware runs one driver at a time.</p><p>Running hostname: ") +
                       setupEscape(WiFi.hostname()) + F(" &middot; IP: ") + WiFi.localIP().toString() +
                       F("</p><p><a href='/management/v1/description'>Management information</a> &middot; <a href='/management/v1/configureddevices'>Configured devices</a></p></section>"));
  }
  else
  {
    sendDomeControlForms();
    server.sendContent(F("<p>Read and update the saved dome settings below. Settings can be changed when the dome and shutter are stationary and no commands are queued.</p>"
                         "<p><a href='/setup/v1/dome/1/config'>Read dome settings as JSON</a></p>"));
    server.sendContent(String(F("<section class='card' id='driver-information'><h2>Driver information</h2><p>Device: Dome 1 &middot; Version: ")) +
                       setupEscape(FPSTR(DriverVersion)) + F("</p><p>Unique ID: ") + setupEscape(FPSTR(GUID)) +
                       F("</p><p>Capabilities are fixed by this firmware. See <a href='/setup/v1/dome/1/config'>dome settings and capabilities</a> for the supported features.</p></section>"));
  }
  const String action = management ? "/setup/config" : "/setup/v1/dome/1/config";
  server.sendContent(F("<h2 id='configuration'>Configuration</h2>"));
  for (const auto &field : setupFields)
  {
    if (field.management != management)
      continue;
    const String value = setupEscape(status >= 400 && server.arg("field") == FPSTR(field.key) && server.hasArg("value") ? server.arg("value") : setupValue(field));
    String row = F("<form id='setting-");
    row += FPSTR(field.key);
    row += F("' method='post' action='");
    row += action;
    row += F("#feedback");
    row += F("'><input type='hidden' name='field' value='");
    row += FPSTR(field.key);
    row += F("'><label for='");
    row += FPSTR(field.key);
    row += F("'>");
    row += FPSTR(field.label);
    row += F("</label>");
    if (field.kind == SETUP_BOOL)
    {
      row += F("<select name='value' id='");
      row += FPSTR(field.key);
      row += F("'>");
      row += value == "true" ? F("<option value='true' selected>Enabled</option><option value='false'>Disabled</option>") : F("<option value='true'>Enabled</option><option value='false' selected>Disabled</option>");
      row += F("</select>");
    }
    else
    {
      row += F("<input name='value' id='");
      row += FPSTR(field.key);
      row += F("' value='");
      row += value;
      row += F("' type='");
      row += field.kind <= SETUP_PEER || field.minimum < 0 ? "text" : "number";
      row += F("' ");
      if (String(FPSTR(field.key)) != "location")
        row += F("required ");
      if (field.kind <= SETUP_PEER)
      {
        if (field.kind == SETUP_HOST || field.kind == SETUP_PEER)
          row += F("autocapitalize='none' spellcheck='false' ");
        row += F("maxlength='");
        row += MAX_NAME_LENGTH - 1;
        row += '\'';
      }
      else if (field.minimum < 0)
      {
        //Keep minus and decimal keys available; setupValid enforces the numeric range.
        row += F("inputmode='text' autocapitalize='none' spellcheck='false'");
      }
      else
      {
        row += field.kind == SETUP_FLOAT ? F("inputmode='decimal' ") : F("inputmode='numeric' ");
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
    row += FPSTR(field.help);
    row += F("</p></form>");
    server.sendContent(row);
  }
  if (management)
    server.sendContent(F("<section class='card'><h2>Maintenance</h2><p>Restart to apply a saved hostname or MQTT server change.</p>"
                         "<form method='post' action='/setup/restart'><button type='submit'>Restart device</button></form>"
                         "<a href='/update'>Update firmware</a> &middot; <a href='/setup/config'>Read management settings as JSON</a></section>"));
  if (message.length())
  {
    String feedbackTarget = management ? "configuration" : "dome-controls";
    if (server.hasArg("field"))
    {
      for (const auto &field : setupFields)
        if (field.management == management && server.arg("field") == FPSTR(field.key))
          feedbackTarget = String("setting-") + FPSTR(field.key);
    }
    else if (server.uri().endsWith("/sync") || server.arg("synced") == "1")
      feedbackTarget = "bearing-sync";
    else if (server.arg("action") == "open" || server.arg("action") == "close" || server.arg("action") == "altitude" || server.arg("section") == "shutter")
      feedbackTarget = "shutter-controls";
    server.sendContent(String(F("<script>(function(){var n=document.getElementById('feedback'),t=document.getElementById('")) +
                       feedbackTarget + F("');if(n&&t){t.parentNode.insertBefore(n,t);n.focus();n.scrollIntoView({block:'start'});}})();</script>"));
  }
  server.sendContent(F("</main></body></html>"));
  server.sendContent("");
}

bool setupBusy()
{
  return domeStatus == DOME_SLEWING || domeStatus == DOME_ABORT ||
         shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_CLOSING || shutterStatus == SHUTTER_ABORTING ||
         (domeCmdList && domeCmdList->size()) || (shutterCmdList && shutterCmdList->size());
}

//Unlike getBearing(), this must never substitute a cached reading after failure.
bool readDomeSyncBearing(float &raw)
{
#if defined USE_REMOTE_ENCODER_FOR_DOME_ROTATION || defined USE_REMOTE_COMPASS_FOR_DOME_ROTATION
  if (!sensorHostname || !sensorHostname[0])
    return false;
  String response;
  const String path = F("/encoder/bearing");
  if (restQuery(sensorHostname, path, "", response, HTTP_GET) != HTTP_CODE_OK)
    return false;
  JsonDocument doc;
  if (deserializeJson(doc, response) || !doc["bearing"].is<double>())
    return false;
  raw = doc["bearing"].as<float>();
#else
  raw = bearing;
#endif
  return isfinite(raw) && raw >= 0.0F && raw <= 360.0F;
}

void handleDomeUiSync()
{
  if (server.method() != HTTP_POST)
  {
    server.sendHeader("Allow", "POST");
    server.send(405, "text/plain", "Use POST with bearing to synchronise the dome.");
    return;
  }
  const SetupField field = {setup_bearingKey, setup_bearingLabel, setup_bearingHelp, SETUP_FLOAT, false, 0, 360};
  const String input = server.arg("bearing");
  if (server.args() != 1 || !server.hasArg("bearing") || !setupValid(field, input))
  {
    sendSetupPage(false, 400, F("Enter a known bearing from 0 to 360 degrees. Fractional degrees are accepted."));
    return;
  }
  if (setupBusy() || domeStatus != DOME_IDLE)
  {
    sendSetupPage(false, 409, F("Wait until the dome is idle and all movement and queued commands have finished before synchronising."));
    return;
  }
  float raw = 0, offset = 0, corrected = 0;
  if (!readDomeSyncBearing(raw) || !calculateDomeBearingSync(raw, input.toFloat(), offset, corrected))
  {
    sendSetupPage(false, 503, F("No valid fresh bearing was received from the sensor. Calibration is unchanged."));
    return;
  }
  const float previousBearing = bearing;
  const float previousOffset = azimuthSyncOffset;
  const float previousAzimuth = currentAzimuth;
  const float previousTarget = targetAzimuth;
  bearing = raw;
  azimuthSyncOffset = offset;
  currentAzimuth = corrected;
  //Keep the idle target in the calibrated coordinate system, including future jogs.
  targetAzimuth = corrected;
  if (!saveToEeprom())
  {
    bearing = previousBearing;
    azimuthSyncOffset = previousOffset;
    currentAzimuth = previousAzimuth;
    targetAzimuth = previousTarget;
    sendSetupPage(false, 500, F("Could not save calibration. Previous running bearing and offset restored."));
    return;
  }
  debugI("Web bearing sync: sensor=%f, offset=%f, current=%f", bearing, azimuthSyncOffset, currentAzimuth);
  server.sendHeader("Location", "/setup/v1/dome/1/setup?synced=1#feedback");
  server.send(303, "text/plain", "Bearing synchronised and offset saved. No movement commanded.");
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
        doc[FPSTR(field.key)] = value == "true";
      else if (field.kind == SETUP_INT)
        doc[FPSTR(field.key)] = value.toInt();
      else if (field.kind == SETUP_FLOAT)
        doc[FPSTR(field.key)] = value.toFloat();
      else
        doc[FPSTR(field.key)] = value;
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
      doc["uniqueID"] = FPSTR(GUID);
      doc["driverVersion"] = FPSTR(DriverVersion);
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
    if (key != FPSTR(field.key) || field.management != management)
      continue;
    if (!setupValid(field, value))
    {
      sendSetupPage(management, 400, String(F("Invalid value for ")) + FPSTR(field.label) + F(". Check the limits below."));
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
  entry["DeviceType"] = FPSTR(DriverType);
  entry["DeviceNumber"] = instanceNumber;
  entry["UniqueID"] = FPSTR(GUID);
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
  server.on("/setup/v1/dome/1/sync", HTTP_ANY, handleDomeUiSync);
  server.on("/setup/v1/dome/1/control", HTTP_ANY, handleDomeUiControl);
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
              sendSetupPage(false, 200, server.arg("synced") == "1" ? F("Bearing synchronised and calibration saved. The dome has not been moved.") : server.arg("queued") == "1" ? F("Movement command queued. Refresh this page to see updated status.")
                                                                                                                                                                                  : F(""));
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
