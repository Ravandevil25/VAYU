#!/bin/bash
set -e
echo "🌪️ Compiling VAYU Daemon for native architecture..."
g++ -O3 -march=native vayu.cpp -o vayu-daemon
echo "🛡️ Setting up systemd service..."
mkdir -p ~/.config/systemd/user/
cp vayu.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now vayu.service
echo "✅ VAYU Deployed and Active!"
