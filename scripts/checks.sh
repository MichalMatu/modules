#!/usr/bin/env sh
set -eu

python3 scripts/check_repo.py
git diff --check
pio run
