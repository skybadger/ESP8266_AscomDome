# Alpaca route and ID validation fix

## Findings

- Dome registrations used mixed-case operation names, whereas ESP8266WebServer's
  default `Uri` matcher compares paths case-sensitively. All Alpaca device URL
  components must be lowercase; parameter keys are case-insensitive.
  Reference: https://ascom-standards.org/api/
- `CanSyncAazimuth` was misspelled, preventing requests for `cansyncazimuth` from
  reaching the registered handler.
- `checkUint32()` assigned an HTTP argument to a JSON variant as a string, then
  tested whether the variant contained an integer. Thus "99" and "999" failed the
  type check. The later signed `as<int>()` conversion also could not preserve the
  full uint32 range.
- The trace for CanFindHome proves that handler was selected for that particular
  request: the failure shown was subsequent validation, not handler lookup.
- Debug output supplied Arduino String objects to `%s` rather than their character
  buffers. This has been corrected as part of the validation trace.

## Changes

- Register the canonical lowercase operation paths and correct `cansyncazimuth`.
  Device number stays 1. HTTP GET/PUT assignments are unchanged and audited.
- Parse ASCII decimal digits directly in `AlpacaRequestValidation.h`, checking
  overflow before multiplication and never converting through a signed integer.
  The output is zero on failure; a valid input assigns the entire parsed value.
  Zero and leading zeros are accepted. Empty input, signs, whitespace, fractions,
  exponent notation, trailing junk and values above 4294967295 are rejected.
- Look up ClientID and ClientTransactionID case-insensitively using the existing
  `hasArgIC()` helper before passing the argument strings to the parser. Unrelated
  arguments such as `Mike=nutter` remain ignored.
- Keep the dome validator's existing requirement that both ID fields be present.
  This patch does not redesign connection ownership, missing-ID policy or the
  separate shared-library validation used by common device handlers.
- Exclude native regression tests from the embedded build source filter.

## Checks

Native C++ tests compile the same parser header used by the firmware. From a
Visual Studio Native Tools prompt:

```bat
cl /nologo /EHsc /std:c++14 /W4 /Fo:build\test-request-validation.obj /Fe:build\test-request-validation.exe tests\test-request-validation.cpp
build\test-request-validation.exe
```

Local route audit:

```powershell
python tests/check-alpaca-routes.py
```

Both passed: uint32 boundary/malformed-input cases and all 41 dome HTTP method/path
pairs, both conditional Name handlers and management registrations.

The ESP12E firmware build also passed (`build/request-validation-build.log`):
43,964 bytes static RAM and 428,249 bytes application flash. Firmware was not uploaded.

After uploading, reproduce the supplied trace using the canonical URL:

```powershell
curl.exe "http://espdom01/api/v1/dome/1/canfindhome?ClientID=99&ClientTransactionID=99&Mike=nutter"
curl.exe "http://espdom01/api/v1/dome/1/canfindhome?clientid=999&CLIENTTRANSACTIONID=999"
python tests/check-alpaca-routes.py http://espdom01
```

Expected: HTTP 200, ErrorNumber 0, and the supplied client transaction number in
the response. The live script only calls GET endpoints; it sends no motion or
connection-change commands. Live checks require the updated firmware and have not
been executed as part of this change.
