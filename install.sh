#!/bin/bash
set -e
echo "Compiling VAYU Daemon..."
make all

echo "Installing (DESTDIR aware, PREFIX=/usr)..."
make install

echo "Injecting CAP_SYS_NICE capability (requires sudo)..."
sudo setcap cap_sys_nice,cap_sys_admin,cap_bpf+ep /usr/bin/vayu-daemon

echo "Setting up systemd user service..."
systemctl --user daemon-reload
systemctl --user enable --now vayu.service
echo "VAYU Deployed and Active!"
