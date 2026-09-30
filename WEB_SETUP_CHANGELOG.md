# Dome web configuration: plan and implementation

## Scope and decisions

The reference is `../ESP8266_AscomSwitch/ESP8266_relayhandler.h`, especially
`setupFormBuilderDeviceStrings`, `setupFormBuilderDriverHeader` and the chunked
`sendDeviceSetup`/driver response pattern. The dome's existing HTML actually lives
in `ASCOM_DomeSetup.h`; `ASCOM_DomeHandler.h` implements the Alpaca device API.

Use a shared device management page and a separate page for the installed dome
driver. This hardware exposes exactly one driver: Dome, device number 1. There is
no driver-count control, runtime driver selector or editable device number because
the firmware routes are fixed. Additional firmware implementations can reuse the
layout/field pattern and register their own driver page.

The UDP setting is interpreted as the discovery **listening** port, matching the
Switch reference and existing `udpPort` variable. It defaults to 32227. Discovery
replies still go to the requester's source port, and advertise the existing HTTP
server on port 80. The user was asked to confirm this interpretation; advertised
HTTP port configuration is not implemented.

Configuration continues to use EEPROM emulation. The earlier LittleFS and FRAM
recovery discussions were plans, not storage migrations implemented here.

## Planned steps and implemented changes

1. **Feature selection:** add `DOME_MODERN_SETUP`, default 1, in
   `ESP8266_AscomDome.h`. Setting it to 0 compiles and registers the original HTML
   handlers. The replacement is `ASCOM_DomeSetupModern.h`. Shared status and
   not-found helpers remain available in either mode.
2. **Pages:** `/` and `/setup` show device management;
   `/setup/v1/dome/1/setup` shows dome settings. Both use a common responsive layout,
   navigation, labelled inputs, per-setting Save buttons, help and status messages.
   CSS is local and the pages require neither JavaScript nor a CDN. Responses are
   streamed in sections to avoid constructing the entire page in RAM.
3. **Read/write handling:** GET JSON endpoints and POST/PUT form-encoded updates
   share the same field definitions. Each update changes one setting. Validation
   checks lengths, numeric syntax/ranges, hostnames, boolean values, missing fields
   and wrong-page fields before applying data. HTML values are escaped. GET never
   changes configuration. Other methods receive 405. Settings updates and restart
   receive 409 while motion or queued commands are active.
4. **Persistence:** keep existing EEPROM field offsets, extend its RAM buffer to
   512 bytes, and reserve bytes 320 onwards for a 48-byte versioned, CRC-checked
   record (with MAX_NAME_LENGTH=40). This contains location, UDP port and the two
   disconnect options. Missing/invalid extension data leaves defaults in place.
   Both UI modes retain this extension so switching modes preserves settings.
   `saveToEeprom()` now returns the actual `EEPROM.commit()` result; the modern
   handler reports failure and restores its prior running value when saving fails.
5. **Runtime application:** discovery port saves rebind the UDP socket, with an
   attempt to restore the old port if binding/saving fails. Hostname changes apply
   after restart; MQTT server changes apply on the next reconnect or restart
   (PubSubClient retains a pointer to the hostname buffer). Other settings apply immediately. Offset changes
   update reported azimuth without issuing a motion command. Restart is an explicit
   POST on `/setup/restart`; firmware update links to the existing `/update` page.
6. **Management integration:** modern-mode configured-device and Alpaca Name
   responses use the editable `ascomName`. The existing management description
   handler reads the backed `Location` buffer. Shared libraries outside this
   project are unchanged.
7. **Verification:** build modern and legacy modes, check JSON integration and
   provide HTTP tests for the running firmware. Record results below.

## Settings inventory

| Page | Form field | Variable | Application |
|---|---|---|---|
| Management | hostname | myHostname | Restart (also regenerates MQTT thisID) |
| Management | location | Location / deviceLocation | Immediate |
| Management | udpport | udpPort | Immediate rebind; range 1–65535 |
| Management | mqttserver | MQTTServerName | Next MQTT reconnect or restart |
| Dome | ascomname | ascomName | Immediate |
| Dome | shutterhostname | shutterHostname | Next remote request |
| Dome | sensorhostname | sensorHostname | Next remote request; remote-sensor builds only |
| Dome | homeposition | homePosition | Immediate, integer 0–359 |
| Dome | parkposition | parkPosition | Immediate, integer 0–359 |
| Dome | syncoffset | azimuthSyncOffset | Immediate, -360 through 360, fractional degrees |
| Dome | parkondisconnect | parkDomeOnDisconnect | Next client disconnect |
| Dome | closeondisconnect | closeShutterOnDisconnect | Next client disconnect |

Strings accept at most MAX_NAME_LENGTH-1 bytes (39 in this build). Location can be
empty; other text fields must be nonempty. Remote controllers are hosts/IPs, not
URLs: the existing networking code constructs its endpoint paths separately.
The new offset control explicitly edits the offset, whereas the old Sync form
accepted an absolute bearing and derived an offset.

Device type/number, identity, versions and capabilities are read-only metadata.
Compile-time pins, credentials, motor tuning constants and firmware capabilities
are not exposed as editable settings. Live azimuth, altitude, movement and client
connection state remain available through `/status` and Alpaca; they are not
configuration controls. Manual movement controls are described below. Legacy custom
movement URLs such as `/Goto` are only registered in legacy mode.

## Build selection

Default:

```cpp
#define DOME_MODERN_SETUP 1
```

To select the original pages, either change that default to 0 or add this to the
existing PlatformIO `build_flags` list:

```ini
    -D DOME_MODERN_SETUP=0
```

This selects the complete page/route implementation, not only the appearance.
EEPROM extension support remains in both builds. Reverting to an older firmware
revision predating the extension may erase the new settings when it next commits
its smaller EEPROM buffer; existing fields retain their original layout.

## HTTP examples

```powershell
curl.exe http://espdom01/setup/config
curl.exe http://espdom01/setup/v1/dome/1/config
curl.exe -X POST --data-urlencode "field=location" --data-urlencode "value=Garden observatory" http://espdom01/setup/config
curl.exe -X PUT --data "field=parkposition&value=356" http://espdom01/setup/v1/dome/1/config
```

GET returns typed JSON. POST/PUT return the relevant HTML page with success or
failure information and meaningful HTTP status (200, 400, 409, 500, 503).
The custom setup endpoints do not require Alpaca ClientID parameters. They accept
exactly `field` and `value`. Alpaca endpoints retain their existing ID validation.

The web configuration uses the existing unauthenticated local HTTP server.
There is no change to authentication or network transport in this implementation.
EEPROM commits retain the existing single-flash-sector power-loss limitations;
the extension CRC detects invalid data but is not an atomic backup scheme.

## Validation and remaining hardware checks

Completed checks:

- Modern build (`DOME_MODERN_SETUP=1`): PASS; static RAM 45,280 / 81,920 bytes,
  firmware 431,161 / 1,044,464 bytes.
- Legacy build (`DOME_MODERN_SETUP=0`): PASS; static RAM 43,756 bytes,
  firmware 427,773 bytes. Tested using `build/legacy-setup.ini`; the project default
  remains modern and the final firmware.bin is the modern build.
- `python tests/check-discovery-script.py`: PASS for discovery/management and
  disconnection after an injected property failure, including downloading both
  setup pages. These mock checks verify the client script, not firmware HTTP behavior.
- New HTTP test script syntax and command-line help: PASS. Live HTTP tests have
  not been run and firmware has not been uploaded.
- Diff whitespace check for the modified setup, EEPROM and readme files: PASS.

Build logs: `build/modern-setup-build.log`, `build/legacy-setup-build.log`.

Run against a flashed, stationary device:

```powershell
python tests/check-setup-http.py http://espdom01
python tests/check-setup-http.py http://espdom01 --write-test
```

The default test checks pages, JSON, invalid requests, non-mutating GET and
management/name consistency. The optional write test temporarily changes location,
checks escaped HTML and management readback, then restores the original location
in a finally block. It sends no motion or restart requests.

Hardware acceptance still needs: save each setting and restart to check persistence;
change and restore the UDP port while running discovery on each port; verify busy
responses during operation; check reboot/firmware-update links; measure free heap
and largest allocation while repeatedly loading both pages with normal polling.
Build sizes do not establish peak runtime heap use.

## Manual movement controls

The modern dome page now includes a manual movement section at
`/setup/v1/dome/1/setup`, under the same `DOME_MODERN_SETUP` switch:

| Control | Request to POST /setup/v1/dome/1/control | Existing queue utility |
|---|---|---|
| Slew to target | action=slew&value=180 | addDomeCmd, CMD_DOME_SLEW |
| Jog east +5 degrees | action=east | normaliseInt, addDomeCmd |
| Jog west -5 degrees | action=west | normaliseInt, addDomeCmd |
| Open shutter | action=open | addShutterCmd, CMD_SHUTTER_OPEN |
| Close shutter | action=close | addShutterCmd, CMD_SHUTTER_CLOSE |
| Move to shutter altitude | action=altitude&value=45 | addShutterCmd, CMD_SHUTTERVAR_SET, name altitude |

Absolute azimuth accepts integer 0–360, with 360 normalized to 0. Jog size is
`domeUiJogDegrees` (5) in `ASCOM_DomeControl.h`. Jogs use the active target or last
queued movement target, round fractional active targets to whole degrees, and wrap
at north. Queued moves do not change an active slew's target; `onDomeIdle()` applies
them in order. Jogs wait if configuration/abort commands precede them in the queue.

Altitude is interpreted as **move directly to an angle**, using the existing
`shutterAltitude()` path, rather than saving separate opening/closing limits. The
current limits are 0–110 degrees, inclusive; the maximum is not wrapped to zero.
`targetAltitude` updates after the remote shutter accepts the command. This local
manual control does not change the driver's advertised `canSetAltitude` capability.
Open/close retain the remote shutter's existing endpoint behavior and wait behind
any active shutter movement. The user was asked to clarify whether persistent
opening/closing limits were intended; those are not implemented here.

Accepted requests redirect with HTTP 303 to a GET page, preventing refresh from
reissuing a movement. The page shows current/active dome position, controller states
and a refresh link. Invalid requests return 400, non-POST methods 405, unavailable
controllers/disabled builds/allocation failures 503, and full queues or halted/
aborting controllers 409. Manual submissions are rejected when the relevant queue
already has eight pending entries (Alpaca queue behavior is unchanged).
Local commands use client/transaction IDs 0 and do not modify Alpaca ownership.
They use the existing local UI access model, so can be issued while an Alpaca client
is connected. They are not written to EEPROM.

The existing `_ENABLE_DOME` and `_ENABLE_SHUTTER` switches were commented out when
this change was made. They are left unchanged: the page reports disabled motion
and handlers reject requests until the corresponding loop is enabled. Also ensure
the normal bearing acquisition is enabled/configured when using actual dome motion.

Supporting fixes: extend `shutterCmdNames` to match enum indices 0/4/5/6 (previously
valid open/close/altitude commands indexed beyond the array during logging); check
queue and command/string allocations in the existing enqueue helpers.

Host regression tests exercise the actual control handler with fake HTTP and queue
objects in `tests/test-dome-controls.cpp`. Build from a Visual Studio Native Tools
prompt, using `/DTEST_DISABLED` for the disabled-control variant:

```bat
cl /nologo /EHsc /std:c++14 /W4 /Fo:build\test-dome-controls.obj /Fe:build\test-dome-controls.exe tests\test-dome-controls.cpp
build\test-dome-controls.exe
```

Cases cover validation, non-POST rejection, absolute slews, both wrap directions,
accumulating jogs, preserving an in-progress target, shutter open/close, 0/45/110
degree altitudes, full queues, missing motor, allocation failures and disabled
control loops. Hardware motion/browser checks require a flashed controller and
were not performed; no firmware was uploaded.

Validation completed: enabled and disabled native control-handler tests PASS;
final `pio run -e esp12e` PASS (`build/manual-control-build.log`); formatting checks
PASS. The final firmware retains the project's existing disabled movement-loop
defines.

## Bearing synchronisation control

The modern dome setup page now provides **Synchronise current bearing** under
`DOME_MODERN_SETUP`. Enter the independently measured physical azimuth in degrees
(0-360, including fractional degrees). This does not move the dome.

New functions:
- `calculateDomeBearingSync()` computes the shortest signed offset from the raw
  sensor bearing to the supplied bearing, normalising 360 to zero.
- `readDomeSyncBearing()` fetches and validates a fresh remote sensor reading;
  failed requests never fall back to the cached remote value. Local sensor builds
  use their current raw bearing variable.
- `handleDomeUiSync()` validates the request, requires idle status and empty command
  queues, updates `bearing`, `azimuthSyncOffset`, `currentAzimuth` and the idle
  `targetAzimuth`, then saves calibration using the existing EEPROM function.
  A save failure restores the previous runtime values.

The raw sensor value is retained in `bearing`; the offset is applied to it during
subsequent normal azimuth updates. No motion command or client ownership change
is made. The endpoint accepts POST only and redirects after success:

```sh
curl -i -X POST http://DOME/setup/v1/dome/1/sync --data "bearing=123.5"
```

Invalid input returns 400, busy state 409, sensor failure 503 and persistence
failure 500. The management page links to the dome setup page where the control
is located. The legacy UI remains selected when `DOME_MODERN_SETUP=0`.

Native regression tests in `tests/test-bearing-sync.cpp` pass for wraparound in
both directions, fractional readings, endpoints, unchanged calibration and invalid
inputs. Hardware/browser verification requires a flashed device; no firmware was
uploaded and no hardware movement was performed.

Final validation: esp12e PlatformIO build PASS (build/bearing-sync-build.log); native bearing calculation tests PASS; clang-format verification PASS.

## Mobile and tablet layout

Implemented the high/medium priority review items for the modern management and
configuration/control pages:
- Navigation links and buttons have at least 44px touch height.
- Reduced phone padding and removed duplicate card/form borders; long values wrap.
- Jog and shutter button pairs use flexible rows that wrap on narrow screens.
- Numeric and decimal keyboard hints are provided; signed offsets use a text
  keyboard to keep minus available, retaining server-side numeric validation.
- Hostname inputs disable automatic capitalisation and spellchecking.
- Invalid configuration submissions retain the entered value for correction.
- Feedback is moved beside the relevant setting/control and focused with a small
  inline script. Without JavaScript, it remains visible at the top via #feedback.
- Movement redirects retain the relevant dome/shutter context. No command or
  persistence semantics were changed.

Browser/device rendering and mobile keyboard checks remain to be performed on
Android Chrome and iOS/iPadOS Safari at 320/375/390/768/1024px, in both orientations
and with enlarged text. No firmware was uploaded or physical commands issued.
