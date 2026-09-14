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
configuration controls. Motion controls remain in the Alpaca API. Legacy custom
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
