#!/usr/bin/env python3
"""
next-id.py — Sequential ID generator for pipeline artifacts.

Usage:
  python3 docs/project-management/next-id.py ticket   → {"id": "psc-0001"}
  python3 docs/project-management/next-id.py adr      → {"id": "psc-adr-0001"}

Reads existing artifacts from docs/project-management/passports/ and docs/adr/
to determine the next available sequence number.
"""

import json
import os
import re
import sys

DOCS_ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
PASSPORTS_DIR = os.path.join(DOCS_ROOT, "project-management", "passports")
ADR_DIR = os.path.join(DOCS_ROOT, "adr")


def next_ticket_id():
    max_id = 0
    if os.path.isdir(PASSPORTS_DIR):
        for f in os.listdir(PASSPORTS_DIR):
            m = re.match(r"^psc-(\d+)-passport\.md$", f)
            if m:
                max_id = max(max_id, int(m.group(1)))
    next_num = str(max_id + 1).zfill(4)
    return f"psc-{next_num}"


def next_adr_id():
    max_id = 0
    if os.path.isdir(ADR_DIR):
        for f in os.listdir(ADR_DIR):
            m = re.match(r"^psc-adr-(\d+)\.md$", f)
            if m:
                max_id = max(max_id, int(m.group(1)))
    next_num = str(max_id + 1).zfill(4)
    return f"psc-adr-{next_num}"


if __name__ == "__main__":
    if len(sys.argv) != 2 or sys.argv[1] not in ("ticket", "adr"):
        print("Usage: python3 next-id.py <ticket|adr>", file=sys.stderr)
        sys.exit(1)

    if sys.argv[1] == "ticket":
        print(json.dumps({"id": next_ticket_id()}))
    else:
        print(json.dumps({"id": next_adr_id()}))