#!/usr/bin/env python3
import os
import socket
import json
import subprocess

def get_hyprland_socket():
    his = os.environ.get("HYPRLAND_INSTANCE_SIGNATURE")
    if not his:
        print("error: HYPRLAND_INSTANCE_SIGNATURE is not set (Hyprland not running).")
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
    # Proof-of-concept simulation of the affinity migration (no taskset executed).
    print(f"active window: {title} (pid {pid})")
    print(f"would pin pid {pid} to core 0")
    print("would restrict previous window tree to cores 1-2")

def listen_to_ipc():
    sock_path = get_hyprland_socket()
    print(f"listening on {sock_path}")
    
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
