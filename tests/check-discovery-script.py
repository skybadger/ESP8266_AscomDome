"""Local integration checks: python tests/check-discovery-script.py"""
import json
from pathlib import Path
import socket
import subprocess
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlsplit

SCRIPT = Path(__file__).resolve().parents[1] / "test-discovery.ps1"


def check(fail_property=False):
    state = {"connected": False, "puts": [], "paths": [], "ids": [], "packets": 0}

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args):
            pass

        def do_GET(self):
            self.respond()

        def do_PUT(self):
            self.respond()

        def respond(self):
            url = urlsplit(self.path)
            path = url.path
            state["paths"].append(path)
            fields = parse_qs(url.query)
            if self.command == "PUT":
                fields = parse_qs(self.rfile.read(int(self.headers["Content-Length"])).decode())
                state["puts"].append((path, fields["Connected"][0]))
                state["connected"] = fields["Connected"][0] == "true"
            state["ids"].append((fields["ClientID"][0], int(fields["ClientTransactionID"][0])))
            value = True
            if path == "/management/apiversions":
                value = [1]
            elif path == "/management/v1/configureddevices":
                value = [{"DeviceType": "Dome", "DeviceName": "Mock dome", "DeviceNumber": 7},
                         {"DeviceType": "Telescope", "DeviceName": "Other", "DeviceNumber": 0}]
            elif path.endswith("/connected"):
                value = state["connected"]
            error = 1024 if fail_property and path.endswith("/azimuth") else 0
            body = json.dumps({"Value": value, "ErrorNumber": error,
                               "ErrorMessage": "Injected failure" if error else ""}).encode()
            if path == "/setup":
                body = b'<html><input name="homePosition" value="90"></html>'
            self.send_response(200)
            self.end_headers()
            self.wfile.write(body)

    http = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.bind(("127.0.0.1", 0))
    udp.settimeout(0.2)
    stop = threading.Event()

    def discover():
        while not stop.is_set():
            try:
                packet, address = udp.recvfrom(1024)
            except socket.timeout:
                continue
            assert packet == b"alpacadiscovery1", packet
            state["packets"] += 1
            udp.sendto(json.dumps({"AlpacaPort": http.server_port}).encode(), address)

    threads = [threading.Thread(target=http.serve_forever), threading.Thread(target=discover)]
    for thread in threads:
        thread.start()
    try:
        with tempfile.TemporaryDirectory() as output:
            result = subprocess.run([
                "powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(SCRIPT),
                "-DiscoveryAddress", "127.0.0.1", "-DiscoveryPort", str(udp.getsockname()[1]),
                "-DiscoverySeconds", "1", "-OutputDirectory", output,
            ], capture_output=True, text=True, timeout=45)
            assert (result.returncode != 0) == fail_property, result.stdout + result.stderr
            assert state["packets"] == 3, state
            assert state["puts"] == [("/api/v1/dome/7/connected", "true"),
                                     ("/api/v1/dome/7/connected", "false")], state
            assert not state["connected"]
            assert "/api/v1/dome/7/slewing" in state["paths"]
            assert len(set(client for client, _ in state["ids"])) == 1
            assert [tid for _, tid in state["ids"]] == list(range(1, len(state["ids"]) + 1))
            assert list(Path(output).glob("*-GET-setup.txt"))
            print("PASS:", "property error still disconnects and returns failure" if fail_property
                  else "UDP discovery, management device 7, properties, setup, disconnect")
    finally:
        stop.set()
        http.shutdown()
        for thread in threads:
            thread.join()
        udp.close()
        http.server_close()


if __name__ == "__main__":
    check()
    check(fail_property=True)
