"""Audit registrations locally; optionally exercise read-only endpoints on hardware.

python tests/check-alpaca-routes.py
python tests/check-alpaca-routes.py http://espdom01
"""
import json
from pathlib import Path
import re
import sys
from urllib.error import HTTPError
from urllib.parse import urlencode
from urllib.request import build_opener, ProxyHandler

ROOT = Path(__file__).resolve().parents[1]
GETS = {
    "altitude": "handleAltitudeGet", "athome": "handleAtHomeGet", "atpark": "handleAtParkGet",
    "azimuth": "handleAzimuthGet", "canfindhome": "handleCanFindHomeGet", "canpark": "handleCanParkGet",
    "cansetaltitude": "handleCanSetAltitudeGet", "cansetazimuth": "handleCanSetAzimuthGet",
    "cansetpark": "handleCanSetParkGet", "cansetshutter": "handleCanSetShutterGet",
    "canslave": "handleCanSlaveGet", "cansyncazimuth": "handleCanSyncAzimuthGet",
    "shutterstatus": "handleShutterStatusGet", "slaved": "handleSlavedGet", "slewing": "handleSlewingGet",
    "connected": "handleConnected", "description": "handleDescriptionGet", "driverinfo": "handleDriverInfoGet",
    "driverversion": "handleDriverVersionGet", "interfaceversion": "handleInterfaceVersionGet",
    "supportedactions": "handleSupportedActionsGet", "connecting": "handleConnectingGet",
    "devicestate": "handleDeviceStateGet",
}
PUTS = {
    "slaved": "handleSlavedPut", "abortslew": "handleAbortSlewPut", "closeshutter": "handleCloseShutterPut",
    "findhome": "handleFindHomePut", "openshutter": "handleOpenShutterPut", "park": "handleParkPut",
    "setpark": "handleSetParkPut", "slewtoaltitude": "handleSlewToAltitudePut",
    "slewtoazimuth": "handleSlewToAzimuthPut", "synctoazimuth": "handleSyncToAzimuthPut",
    "connect": "handleConnectPut", "disconnect": "handleDisconnectPut", "connected": "handleConnected",
    "action": "handleAction", "commandblind": "handleCommandBlind", "commandbool": "handleCommandBool",
    "commandstring": "handleCommandString",
}


def audit():
    source = (ROOT / "ESP8266_AscomDome.ino").read_text()
    routes = re.findall(r'server\.on\(F\("(/api/v1/dome/1/[^\"]+)"\),\s*HTTP_(GET|PUT),\s*(\w+)\)', source)
    expected = {(f"/api/v1/dome/1/{name}", verb, handler)
                for verb, entries in (("GET", GETS), ("PUT", PUTS)) for name, handler in entries.items()}
    #Both conditional name registrations must exist, one in each UI mode.
    expected |= {( "/api/v1/dome/1/name", "GET", handler) for handler in ("handleModernName", "handleNameGet")}
    assert len(routes) == len(set(routes)), "Duplicate registrations"
    assert set(routes) == expected, ("Missing", expected - set(routes), "Unexpected", set(routes) - expected)
    for method in ("apiversions", "description", "configureddevices"):
        path = "/management/" + ("" if method == "apiversions" else "v1/") + method
        assert f'server.on(F("{path}"), HTTP_GET,' in source
    print(f"PASS: {len(expected)-1} dome method/path pairs, both Name variants and management registrations")


def live(base):
    opener = build_opener(ProxyHandler({}))

    def get(method, fields, expected=200):
        url = base.rstrip("/") + "/api/v1/dome/1/" + method + "?" + urlencode(fields)
        try:
            response = opener.open(url, timeout=15)
        except HTTPError as error:
            response = error
        with response:
            body = json.loads(response.read())
            assert response.code == expected, (url, response.code, body)
        return body

    for method in list(GETS) + ["name"]:
        body = get(method, {"ClientID": "99", "ClientTransactionID": "99", "Mike": "nutter"})
        assert body["ErrorNumber"] == 0 and body["ClientTransactionID"] == 99, (method, body)
    for number in ("99", "999", "0", "2147483648", "4294967295"):
        body = get("canfindhome", {"clientid": number, "CLIENTTRANSACTIONID": number})
        assert body["ErrorNumber"] == 0 and body["ClientID"] == int(number)
        assert body["ClientTransactionID"] == int(number)
    for field in ("ClientID", "ClientTransactionID"):
        for invalid in ("", "-1", "99junk", "4294967296", "9.9"):
            fields = {"ClientID": "99", "ClientTransactionID": "999", field: invalid}
            assert get("canfindhome", fields, 400)["ErrorNumber"] != 0
    print("PASS: read-only routes, trace reproduction, mixed-case argument names and invalid IDs")


if __name__ == "__main__":
    audit()
    if len(sys.argv) > 1:
        live(sys.argv[1])
