#!/usr/bin/env python3
"""Bounded read-only heap/uptime monitor; does not change boiler settings."""
import argparse
import datetime
import json
import time
import urllib.request

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('url')
parser.add_argument('--seconds', type=int, default=900)
parser.add_argument('--interval', type=float, default=10)
args = parser.parse_args()
end = time.monotonic() + args.seconds
previous_uptime = None
while time.monotonic() < end:
    row = {'time': datetime.datetime.now(datetime.timezone.utc).isoformat()}
    try:
        with urllib.request.urlopen(args.url.rstrip('/') + '/api/info', timeout=8) as response:
            info = json.load(response)
        uptime = info['system']['uptime']
        row.update(version=info['build']['version'], uptime=uptime,
                   resetReason=info['system']['resetReason'], heap=info['heap'])
        if previous_uptime is not None and uptime < previous_uptime:
            row['rebootDetected'] = True
        previous_uptime = uptime
    except Exception as error:
        row['error'] = str(error)
    print(json.dumps(row), flush=True)
    time.sleep(min(args.interval, max(0, end-time.monotonic())))
