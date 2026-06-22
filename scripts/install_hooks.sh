#!/usr/bin/env sh
set -eu

git config core.hooksPath .githooks
chmod +x .githooks/pre-commit scripts/checks.sh scripts/check_repo.py
printf 'Git hooks installed from .githooks\n'
