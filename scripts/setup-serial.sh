#!/usr/bin/env bash
# Give the current user access to the ESP32's USB serial port (CH340 -> /dev/ttyUSB*).
set -euo pipefail
user="${SUDO_USER:-$USER}"
sudo usermod -aG dialout "$user"
# Immediate access for this session; the group membership applies after next login.
for p in /dev/ttyUSB* /dev/ttyACM*; do
  [ -e "$p" ] && sudo chmod a+rw "$p" && echo "opened $p"
done
echo "done: $user is in dialout (log out/in once to make it permanent)"
