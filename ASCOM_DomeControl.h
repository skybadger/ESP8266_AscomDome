#ifndef ASCOM_DOME_CONTROL_H
#define ASCOM_DOME_CONTROL_H

#include "AlpacaRequestValidation.h"
#include <math.h>

const int domeUiJogDegrees = 5;
const int domeUiQueueLimit = 8;

void sendDomeUiQueued()
{
  //A browser refresh must not repeat a movement POST.
  const String action = server.arg("action");
  const bool shutter = action == "open" || action == "close" || action == "altitude";
  server.sendHeader("Location", shutter ? "/setup/v1/dome/1/setup?queued=1&section=shutter#feedback" : "/setup/v1/dome/1/setup?queued=1#feedback");
  server.send(303, "text/plain", "Movement command queued.");
}

//Queue commands only; the normal control loop owns active movement and targets.
void handleDomeUiControl()
{
  if (server.method() != HTTP_POST)
  {
    server.sendHeader("Allow", "POST");
    server.send(405, "text/plain", "Use POST to issue a movement command.");
    return;
  }
  const String action = server.arg("action");
  const bool domeMove = action == "slew" || action == "east" || action == "west";
  const bool shutterMove = action == "open" || action == "close" || action == "altitude";
  const bool needsValue = action == "slew" || action == "altitude";
  if (!server.hasArg("action") || (!domeMove && !shutterMove) ||
      server.args() != (needsValue ? 2 : 1) || (needsValue && !server.hasArg("value")))
  {
    sendSetupPage(false, 400, F("Invalid movement request. Supply an action and, where required, a value."));
    return;
  }
  uint32_t value = 0;
  if (needsValue)
  {
    const String input = server.arg("value");
    if (!parseAlpacaUint32(input.c_str(), input.length(), value) ||
        (action == "slew" && value > 360) ||
        (action == "altitude" && (value < SHUTTER_MIN_ALTITUDE || value > SHUTTER_MAX_ALTITUDE)))
    {
      sendSetupPage(false, 400, F("Enter whole degrees: dome 0-360, shutter within the displayed altitude limits."));
      return;
    }
  }
  if (domeMove)
  {
#if !defined _ENABLE_DOME
    sendSetupPage(false, 503, F("Dome movement is disabled in this firmware build."));
    return;
#else
//    if (!domeCmdList ) 
//    {
//      sendSetupPage(false, 503, F("Dome commands are unavailable due to queue not available"));
//      return;
//    }
    if (domeCmdList->size() >= domeUiQueueLimit)
    {
      sendSetupPage(false, 409, F("Dome commands are unavailable due to pending actions (queue full)."));
      return;
    }
    int target = static_cast<int>(value);
    if (action != "slew") //east or west jog
    {
      target = static_cast<int>(lroundf(fmodf(targetAzimuth, 360.0F)));
      normaliseInt(target, 360);
      target += action == "east" ? domeUiJogDegrees : -domeUiJogDegrees;
    }
    //Handle as a normal slew otherwise 
    normaliseInt(target, 360);
    if (!addDomeCmd(0, 0, F("WebSlew"), CMD_DOME_SLEW, target))
    {
      sendSetupPage(false, 503, F("Unable to allocate a queued dome command."));
      return;
    }
    sendDomeUiQueued();
    return;
#endif
  }
#if !defined _ENABLE_SHUTTER
  sendSetupPage(false, 503, F("Shutter movement is disabled in this firmware build."));
  return;
#else
  //Shutter moves
  if ( !shutterCmdList || !shutterHostname || !shutterHostname[0] )
  {
    sendSetupPage(false, 503, F("Shutter command queue or controller address is unavailable."));
    return;
  }
  if (shutterStatus == SHUTTER_ABORTING || shutterStatus == SHUTTER_ABORTED || shutterStatus == SHUTTER_HALTED ||
      shutterCmdList->size() >= domeUiQueueLimit)
  {
    sendSetupPage(false, 409, F("Shutter is halted, aborting, or its queue is full."));
    return;
  }
  const shutterCmd command = action == "open" ? CMD_SHUTTER_OPEN : action == "close" ? CMD_SHUTTER_CLOSE
                                                                                     : CMD_SHUTTERVAR_SET;
  if (!addShutterCmd(0, 0, action == "altitude" ? "altitude" : "", command, static_cast<int>(value)))
  {
    sendSetupPage(false, 503, F("Unable to allocate a shutter command."));
    return;
  }
  sendDomeUiQueued();
#endif
}

#endif
