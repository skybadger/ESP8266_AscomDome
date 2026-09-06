# ArduinoJson v7 migration

## Arduino CLI build

Run this command from the `ESP8266_AscomDome` directory:

```powershell
& 'C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' compile `
  --fqbn esp8266:esp8266:nodemcuv2 `
  --libraries 'C:\Mikes\software\Skybadgers-Git-Repository\libraries' `
  --build-path '.\build\arduinojson-v7-check' `
  .
```

The explicit `--libraries` argument is required because helper files such as
`DebugSerial.h`, `ASCOMAPICommon_rest.h`, and `AlpacaManagement.h` are stored in
the repository-level `libraries` directory.

## Changes made

- Replaced ArduinoJson v5 buffers and APIs with ArduinoJson v7 `JsonDocument`,
  `deserializeJson()`, and `serializeJson()` implementations.
- Added `ArduinoJsonCompat.h` to the shared `ASCOM_rest` library. It selects v5
  or v7 object, serialization, and nested-value implementations using
  `ARDUINOJSON_VERSION_MAJOR`.
- Updated the shared `ASCOMAPICommon_rest.h` and `AlpacaManagement.h` handlers to
  use the compatibility macros, preserving support for projects using v5.
- Added `validateClientTransactionIds()` to `ASCOM_DomeHandler.h` and applied it
  to every dome handler. Invalid or missing `ClientID` or
  `ClientTransactionID` values now produce a JSON error response with HTTP 400.

The ESP8266 NodeMCU build was verified successfully with ArduinoJson 7.0.0 and
ESP8266 core 3.1.2.

## Dome interface v3: first pass

### Plan

1. Correct malformed and incorrectly registered URLs.
2. Add the `connect`, `disconnect`, `connecting`, and `devicestate` handlers.
3. Add shared controller and connection states without changing the five ASCOM
   `ShutterState` values returned over the wire.
4. Reuse a common status snapshot for the custom status page and DeviceState.
5. Poll once per second while active and once every five seconds while idle.
6. Compile with Arduino CLI, then defer the remaining behavioral conformance
   work to a second pass.

### Completed

- Corrected the device setup URL in `ESP8266_AscomDome.ino` from a string that
  contained invisible zero-width characters to `/setup/v1/dome/1/setup`.
- Changed `commandstring` from HTTP GET to the required HTTP PUT registration.
- Added PUT `connect`, PUT `disconnect`, GET `connecting`, and GET `devicestate`
  routes and handlers.
- Added a non-blocking connection state machine while retaining the legacy
  `connected` GET/PUT behavior.
- Added shared state enums and V3 handler declarations to
  `libraries/ASCOM_rest/ASCOMAPIDome_rest.h` so dome projects can include them.
- Added internal abort/halt states. Internal shutter states outside the five
  standard values are mapped to `SHUTTER_ERROR` in Alpaca responses.
- Added `appendDomeStatusFields()`. The custom `/status` handler and DeviceState
  now use the same status snapshot and the existing ISO timestamp helper.
- DeviceState returns the required known dome properties and useful internal
  controller state values as `Name`/`Value` objects.
- Expanded `Slewing` so shutter opening, closing, and aborting count as motion.
- Added adaptive status processing: 1000 ms while motion or queued commands are
  present and 5000 ms when the dome and shutter are idle.
- Updated `InterfaceVersion` to 3.
- Verified the first pass by compiling for `esp8266:esp8266:nodemcuv2` with
  ArduinoJson 7.0.0 and ESP8266 core 3.1.2.

### Deferred to the second pass

- Full AbortSlew coordination and confirmation across dome and shutter.
- Complete IDomeV3 asynchronous state and error semantics.
- Review optional Alpaca client-ID behavior and HTTP 400 response formatting.
- ConformU endpoint and behavioral testing against real hardware.
