"""Exercise a flashed modern-setup device. No motion/restart requests are sent.

python tests/check-setup-http.py http://espdom01
python tests/check-setup-http.py http://espdom01 --write-test
The optional write test changes location briefly and restores it in finally.
"""
import argparse
import json
from urllib.error import HTTPError
from urllib.parse import urlencode
from urllib.request import Request, build_opener, ProxyHandler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base_url")
    parser.add_argument("--write-test", action="store_true")
    args = parser.parse_args()
    base = args.base_url.rstrip("/")
    opener = build_opener(ProxyHandler({}))

    def request(path, method="GET", data=None, expected=200):
        payload = None if data is None else urlencode(data).encode()
        req = Request(base + path, data=payload, method=method)
        try:
            response = opener.open(req, timeout=20)
        except HTTPError as error:
            response = error
        with response:
            body = response.read().decode()
            assert response.code == expected, (path, response.code, body[:500])
        return body

    management = "/setup/config"
    dome = "/setup/v1/dome/1/config"
    original_management = json.loads(request(management))
    original_dome = json.loads(request(dome))
    for page, keys in (("/setup", ["location", "udpport", "hostname", "mqttserver"]),
                       ("/setup/v1/dome/1/setup", ["syncoffset", "parkposition", "ascomname"])):
        html = request(page)
        assert "<html lang='en'>" in html and "method='post'" in html
        for key in keys:
            assert f"id='{key}'" in html
        assert "https://maxcdn" not in html

    bad = [(management, "udpport", "0"), (management, "udpport", "65536"),
           (management, "udpport", "32227junk"), (management, "hostname", "bad host"),
           (management, "location", "x" * 40), (dome, "homeposition", "360"),
           (dome, "parkposition", "1.5"), (dome, "syncoffset", "NaN"),
           (dome, "syncoffset", "1.2.3"), (dome, "closeondisconnect", "yes"),
           (dome, "udpport", "32227"), (management, "unknown", "value")]
    for endpoint, key, value in bad:
        request(endpoint, "POST", {"field": key, "value": value}, 400)
    request(management, "POST", {"field": "location"}, 400)
    request(management, "DELETE", expected=405)
    #GET arguments must never modify configuration.
    request(management + "?field=location&value=should-not-save")
    assert json.loads(request(management)) == original_management
    assert json.loads(request(dome)) == original_dome
    configured = json.loads(request("/management/v1/configureddevices"))["Value"]
    assert len(configured) == 1 and configured[0]["DeviceNumber"] == 1
    assert configured[0]["DeviceName"] == original_dome["ascomname"]
    name = json.loads(request("/api/v1/dome/1/name?ClientID=99&ClientTransactionID=1"))
    assert name["ErrorNumber"] == 0 and name["Value"] == original_dome["ascomname"]
    print("PASS: pages, JSON, validation, read-only GET, management/name consistency")
    if args.write_test:
        try:
            value = "Observatory <test> & 'quoted'"
            html = request(management, "PUT", {"field": "location", "value": value})
            assert "&lt;test&gt;" in html and "&#39;quoted&#39;" in html
            assert json.loads(request(management))["location"] == value
            description = json.loads(request("/management/v1/description"))
            assert description["Value"]["Location"] == value
        finally:
            request(management, "POST", {"field": "location", "value": original_management["location"]})
        assert json.loads(request(management))["location"] == original_management["location"]
        print("PASS: update, HTML escaping, management readback, original location restored")


if __name__ == "__main__":
    main()
