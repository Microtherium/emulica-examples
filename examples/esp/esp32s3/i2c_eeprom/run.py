from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


PROJECT = "esp/esp32s3/i2c_eeprom/firmware/src"


def find_client_dir(start: Path) -> Path:
    for path in (start, *start.parents):
        if (path / "project_runner.py").is_file() and (path / "examples").is_dir():
            return path
    raise RuntimeError("Could not find Emulica src/client directory")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the ESP32-S3 Emulica example")
    parser.add_argument(
        "--pin",
        action="append",
        default=[],
        metavar="PIN=VALUE",
        help="External MCU pin input, e.g. GPIO4=1. Can be repeated.",
    )
    args, forwarded = parser.parse_known_args()
    for pin_assignment in args.pin:
        if "=" not in pin_assignment:
            parser.error(f"--pin must be PIN=VALUE, got {pin_assignment!r}")
        raw_value = pin_assignment.split("=", 1)[1].strip()
        try:
            int(raw_value, 0)
        except ValueError:
            parser.error(f"--pin value must be an integer, got {raw_value!r}")

    client_dir = find_client_dir(Path(__file__).resolve().parent)
    env = os.environ.copy()
    env.setdefault("EMULICA_API_KEY", "emulica-dev-key-2026")
    for pin_assignment in args.pin:
        forwarded.extend(["--pin", pin_assignment])

    command = [
        sys.executable,
        str(client_dir / "project_runner.py"),
        "simulate",
        "--project",
        PROJECT,
        *forwarded,
    ]
    return subprocess.run(command, cwd=client_dir, env=env).returncode


if __name__ == "__main__":
    raise SystemExit(main())
