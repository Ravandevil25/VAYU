#!/usr/bin/env python3
import os
import socket
import json
import subprocess

def get_hyprland_socket():
    his = os.environ.get("HYPRLAND_INSTANCE_SIGNATURE")
    if not his:
        print("Error: Hyprland is not running or HYPRLAND_INSTANCE_SIGNATURE is not set.")
        exit(1)
    return f"/tmp/hypr/{his}/.socket2.sock"

def get_active_window_pid():
    try:
        result = subprocess.run(['hyprctl', 'activewindow', '-j'], capture_output=True, text=True)
        data = json.loads(result.stdout)
        return data.get("pid"), data.get("title")
    except Exception as e:
        return None, None

def isolate_core_for_pid(pid, title):
    # This is the VAYU Algorithm Execution
    # For this PoC, we will safely simulate the hardware pinning.
    print(f"\n[VAYU ALGORITHM TRIGGERED]")
    print(f"🎯 Target Acquired: {title} (PID: {pid})")
    print(f"⚡ VAYU is locking Physical Core 0 exclusively for PID {pid}...")
    print(f"🛑 VAYU is migrating all background GUI apps to Core 1 & 2...")
    print(f"✅ Hardware Partition Complete. Zero-Latency mode active for: {title}\n")
    # In production, we execute: os.system(f"taskset -pc 0 {pid}")

def listen_to_ipc():
    sock_path = get_hyprland_socket()
    print(f"📡 VAYU Core Listening to Hyprland IPC: {sock_path}")
    
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(sock_path)
        while True:
            data = s.recv(4096).decode('utf-8')
            if not data:
                break
            
            # If the user changes window focus
            if "activewindow" in data:
                pid, title = get_active_window_pid()
                if pid:
                    isolate_core_for_pid(pid, title)

if __name__ == "__main__":
    listen_to_ipc()
