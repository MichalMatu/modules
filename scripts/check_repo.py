#!/usr/bin/env python3
"""Repository regression checks for the ESP32 module baseline."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIRS = ("include", "src")
SERIAL_ALLOWLIST = {Path("src/AppLog.cpp")}
REQUIRED_FILES = (
    Path("include/HardwareProfile.h"),
    Path("include/MemoryDiagnostics.h"),
    Path("src/MemoryDiagnostics.cpp"),
)


def relative(path: Path) -> Path:
    return path.relative_to(ROOT)


def source_files() -> list[Path]:
    files: list[Path] = []
    for folder in SOURCE_DIRS:
        files.extend(
            path
            for path in (ROOT / folder).rglob("*")
            if path.suffix in {".h", ".hpp", ".c", ".cpp"}
        )
    return sorted(files)


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def check_no_arduino_string(errors: list[str]) -> None:
    pattern = re.compile(r"\bString\b")
    for path in source_files():
        text = path.read_text(encoding="utf-8")
        if pattern.search(text):
            fail(errors, f"{relative(path)} uses Arduino String")


def check_serial_print_policy(errors: list[str]) -> None:
    pattern = re.compile(r"\bSerial\.(?:print|println|printf)\b")
    for path in source_files():
        rel = relative(path)
        if rel in SERIAL_ALLOWLIST:
            continue
        text = path.read_text(encoding="utf-8")
        if pattern.search(text):
                fail(errors, f"{rel} writes directly to Serial; use AppLog")


def check_no_blocking_delay(errors: list[str]) -> None:
    pattern = re.compile(r"\bdelay\s*\(")
    for path in source_files():
        text = path.read_text(encoding="utf-8")
        if pattern.search(text):
            fail(errors, f"{relative(path)} uses delay(); use vTaskDelay")


def check_generic_heap_policy(errors: list[str]) -> None:
    pattern = re.compile(
        r"\b(?:malloc|calloc|realloc|free)\s*\("
        r"|\bnew\s+(?!\()"
        r"|\bdelete\s+"
    )
    for path in source_files():
        text = path.read_text(encoding="utf-8")
        if pattern.search(text):
            fail(errors, f"{relative(path)} uses generic heap allocation")


def check_hardware_profile_boundary(errors: list[str]) -> None:
    app_config = (ROOT / "include/AppConfig.h").read_text(encoding="utf-8")
    forbidden = ("OledSdaPin", "OledSclPin", "GPIO")
    for token in forbidden:
        if token in app_config:
            fail(errors, f"include/AppConfig.h contains board-specific token {token}")

    for required in REQUIRED_FILES:
        if not (ROOT / required).exists():
            fail(errors, f"{required} is missing")


def check_platformio_local_libraries(errors: list[str]) -> None:
    platformio = (ROOT / "platformio.ini").read_text(encoding="utf-8")
    if re.search(r"(?m)^\s*lib_deps\s*=", platformio):
        fail(errors, "platformio.ini uses lib_deps; vendor libraries into lib/")
    if "-std=gnu++17" not in platformio:
        fail(errors, "platformio.ini must build with -std=gnu++17")
    if "APP_LOG_LEVEL" not in platformio:
        fail(errors, "platformio.ini must define APP_LOG_LEVEL")
    if "CORE_DEBUG_LEVEL=0" not in platformio:
        fail(errors, "platformio.ini must disable Arduino core debug logs on main")
    if "-fno-exceptions" not in platformio or "-fno-rtti" not in platformio:
        fail(errors, "platformio.ini must disable C++ exceptions and RTTI")


def main() -> int:
    errors: list[str] = []
    check_no_arduino_string(errors)
    check_serial_print_policy(errors)
    check_no_blocking_delay(errors)
    check_generic_heap_policy(errors)
    check_hardware_profile_boundary(errors)
    check_platformio_local_libraries(errors)

    if errors:
        print("Repository regression checks failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print("Repository regression checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
