#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
command -v arduino-cli >/dev/null || { echo "Arduino CLI is required; see docs/TESTING.md." >&2; exit 1; }
for sketch in \
  firmware/SmartDispenser_RTC \
  experiments/SmartDispenser_NoRTC \
  diagnostics/Dispenser_Hardware_Test \
  diagnostics/SmartDispenser_Button_Test \
  diagnostics/DS3231_RTC_Test; do
  arduino-cli compile --fqbn arduino:avr:mega:cpu=atmega2560 --warnings all "$sketch"
done
