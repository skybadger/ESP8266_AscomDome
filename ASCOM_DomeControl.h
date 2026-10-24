#include "DomeFunctionTrace.h"
#ifndef ASCOM_DOME_CONTROL_H
#define ASCOM_DOME_CONTROL_H

#include "AlpacaRequestValidation.h"
#include <math.h>

const int domeUiJogDegrees = 5;
const int domeUiQueueLimit = 8;

/**
 * @brief Send a 303 redirect to the relevant setup section so refreshing the browser does not repeat the movement POST.
 * @details Send a 303 redirect to the relevant setup section so refreshing the browser does not repeat the movement POST.
 * @par Arguments
 * None.
 * @return (void) Observable effects are described above.
 */
void sendDomeUiQueued()
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("state dome=%d shutter=%d azimuth=%g target=%g", (int)domeStatus, (int)shutterStatus, (double)currentAzimuth, (double)targetAzimuth);
                      });
  //A browser refresh must not repeat a movement POST.
  const String action = server.arg("action");
  const bool shutter = action == "open" || action == "close" || action == "altitude";
  server.sendHeader("Location", shutter ? "/setup/v1/dome/1/setup?queued=1&section=shutter#feedback" : "/setup/v1/dome/1/setup?queued=1#feedback");
  server.send(303, "text/plain", "Movement command queued.");
}

//Movement is queued; manual abort stops immediately regardless of client ownership.
/**
 * @brief Validate the POST action and numeric limits, enforce queue capacity and enabled hardware, then queue movement or perform immediate abort.
 * @details Validate the POST action and numeric limits, enforce queue capacity and enabled hardware, then queue movement or perform immediate abort. Ignore additional East or West requests while a WebJog slew remains queued.
 * @par Arguments
 * None.
 * @return (void) Observable effects are described above.
 */
void handleDomeUiControl()
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("state dome=%d shutter=%d azimuth=%g target=%g", (int)domeStatus, (int)shutterStatus, (double)currentAzimuth, (double)targetAzimuth);
                      });
  if (server.method() != HTTP_POST)
  {
    server.sendHeader("Allow", "POST");
    server.send(405, "text/plain", "Use POST to issue a movement command.");
    return;
  }
  const String action = server.arg("action");
  const bool abort = action == "abort";
  const bool domeMove = action == "slew" || action == "east" || action == "west";
  const bool shutterMove = action == "open" || action == "close" || action == "altitude";
  const bool needsValue = action == "slew" || action == "altitude";
  if (!server.hasArg("action") || (!abort && !domeMove && !shutterMove) ||
      server.args() != (needsValue ? 2 : 1) || (needsValue && !server.hasArg("value")))
  {
    sendSetupPage(false, 400, F("Invalid movement request. Supply an action and, where required, a value."));
    return;
  }
  if (abort)
  {
    if (abortAllMovement() != 200)
    {
      sendSetupPage(false, 503, F("Dome stopped and both command queues cleared. Shutter stop was not confirmed; check the shutter controller and retry Abort."));
      return;
    }
    server.sendHeader("Location", "/setup/v1/dome/1/setup?aborted=1#feedback");
    server.send(303, "text/plain", "Dome stopped, shutter abort acknowledged, and command queues cleared.");
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
    if (action != "slew")
    {
      for (int index = 0; index < domeCmdList->size(); ++index)
      {
        const cmdItem_t *pending = domeCmdList->get(index);
        if (pending && pending->cmd == CMD_DOME_SLEW && String(pending->cmdName) == F("WebJog"))
        {
          //The existing jog remains queued; refreshing must not repeat this POST.
          sendDomeUiQueued();
          return;
        }
      }
    }
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
    if (!addDomeCmd(0, 0, action == "slew" ? F("WebSlew") : F("WebJog"), CMD_DOME_SLEW, target))
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
  if (!shutterCmdList || !shutterHostname || !shutterHostname[0])
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
