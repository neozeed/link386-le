#!/usr/bin/env python3
from r3u_import_anchors import audit,ROOT
actual,pred=audit(ROOT/'fixtures/hi-reference.exe',ROOT/'r3n-hi-report.txt')
assert actual == {1:8,2:4},actual
assert pred == {1:12},pred
print('R3U PASS: exact Microsoft-vs-planner object mismatch reproduced')
