#!/usr/bin/env python3
"""Bounded read-only HTTP load with two clients and heap/uptime stop conditions."""
import argparse
import collections
import concurrent.futures
import json
import time
import urllib.request

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('url')
parser.add_argument('--rounds', type=int, default=50)
parser.add_argument('--version', required=True)
args = parser.parse_args()
base = args.url.rstrip('/')
paths = ['/', '/dashboard.html', '/api/info', '/api/vars',
         '/api/sensors?detailed=1', '/api/settings', '/api/debug']


def read(path):
    with urllib.request.urlopen(base + path, timeout=8) as response:
        raw = response.read()
        data = json.loads(raw) if path.startswith('/api/') else None
        return response.status, data


counts = collections.Counter()
errors = []
start = time.monotonic()
previous_uptime = None
last = None
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
    for index in range(args.rounds):
        futures = [pool.submit(read, paths[(2 * index + j) % len(paths)]) for j in range(2)]
        for future in futures:
            try:
                status, _ = future.result()
                counts[str(status)] += 1
            except Exception as error:
                errors.append(str(error))
        try:
            _, last = read('/api/info')
            counts['healthChecks'] += 1
            uptime = last['system']['uptime']
            if (last['build']['version'] != args.version
                    or (previous_uptime is not None and uptime < previous_uptime)
                    or last['heap']['free'] < 12000
                    or last['heap']['maxFreeBlock'] < 4096):
                errors.append('Health/version guard reached')
            previous_uptime = uptime
        except Exception as error:
            errors.append(str(error))
        if errors:
            break
        time.sleep(.15)
result = {'requests': dict(counts), 'errors': errors, 'seconds': round(time.monotonic() - start, 2)}
if last:
    result.update(finalUptime=last['system']['uptime'], finalHeap=last['heap'])
print(json.dumps(result, indent=2))
raise SystemExit(bool(errors))
