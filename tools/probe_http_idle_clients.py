#!/usr/bin/env python3
# EnvNode-owned: see repository LICENSE.
"""Read-only HTTP regression: empty TCP connections must not starve a GET."""
import argparse
import http.client
import json
import socket
import time
from urllib.parse import urlsplit


def probe(url, idle_count=0, fragmented=False):
    target = urlsplit(url)
    if target.scheme != "http" or not target.hostname or target.username or target.password:
        raise ValueError("Use an http://host[:port]/path URL without credentials")
    address = (target.hostname, target.port or 80)
    clients = []
    request = None
    response = None
    try:
        for _ in range(idle_count):
            clients.append(socket.create_connection(address, timeout=10))
        if clients:
            time.sleep(0.3)  # Let the firmware accept the deliberately empty clients.
        started = time.monotonic()
        request = socket.create_connection(address, timeout=10)
        path = target.path or "/"
        if target.query:
            path += "?" + target.query
        wire = (f"GET {path} HTTP/1.1\r\nHost: {target.netloc}\r\nConnection: close\r\n\r\n").encode("ascii")
        if fragmented:
            request.sendall(wire[:1])
            time.sleep(0.05)
            request.sendall(wire[1:])
        else:
            request.sendall(wire)
        response = http.client.HTTPResponse(request)
        response.begin()
        headers_at = time.monotonic()
        size = len(response.read())
        return dict(idle_clients=idle_count, fragmented=fragmented,
                    status=response.status, bytes=size,
                    headers_ms=round((headers_at-started)*1000, 1),
                    total_ms=round((time.monotonic()-started)*1000, 1))
    finally:
        if response:
            response.close()
        if request:
            request.close()
        for client in clients:
            client.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("url", help="Existing read-only page, e.g. http://192.168.20.63/controllers")
    parser.add_argument("--max-seconds", type=float, default=1.0,
                        help="Fail when a complete response exceeds this limit (default: 1)")
    args = parser.parse_args()
    failed = False
    for idle, fragmented in [(0, False), (1, False), (2, False), (4, False), (1, True)]:
        try:
            result = probe(args.url, idle, fragmented)
            result["passed"] = result["status"] == 200 and result["total_ms"] <= args.max_seconds*1000
        except (OSError, ValueError, http.client.HTTPException) as error:
            result = dict(idle_clients=idle, fragmented=fragmented, passed=False, error=str(error))
        failed |= not result["passed"]
        print(json.dumps(result), flush=True)
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
