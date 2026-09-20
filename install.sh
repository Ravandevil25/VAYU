#!/bin/bash
set -e
echo "Building vayu..."
make all

echo "Installing to /usr (DESTDIR and PREFIX supported)..."
sudo make install

echo "Assigning capabilities (requires sudo)..."
sudo setcap cap_sys_nice,cap_sys_admin,cap_bpf+ep /usr/bin/vayu-daemon

echo "Enabling user service..."
systemctl --user daemon-reload
systemctl --user enable --now vayu.service
echo "vayu installed and enabled."
