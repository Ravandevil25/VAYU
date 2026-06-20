#!/usr/bin/env python3
import os
import socket
import json
import subprocess
import glob

def get_hyprland_socket():
    his = os.environ.get("HYPRLAND_INSTANCE_SIGNATURE")
    xdg_runtime = os.environ.get("XDG_RUNTIME_DIR", f"/run/user/{os.getuid()}")
    base_path = f"{xdg_runtime}/hypr"
    
    if his:
        sock_path = f"{base_path}/{his}/.socket2.sock"
        if os.path.exists(sock_path):
            return sock_path
            
    # Fallback to finding the newest directory in XDG_RUNTIME_DIR/hypr
    try:
        hypr_dirs = glob.glob(f"{base_path}/*")
        if hypr_dirs:
            latest_dir = max(hypr_dirs, key=os.path.getmtime)
            return f"{latest_dir}/.socket2.sock"
    except Exception as e:
        print(e)
        
    print("Error: Could not find Hyprland socket.")
    exit(1)

def get_active_window_pid():
    try:
        result = subprocess.run(['hyprctl', 'activewindow', '-j'], capture_output=True, text=True)
        data = json.loads(result.stdout)
        return data.get("pid")
    except Exception:
        return None

last_pid = None

def apply_vayu_logic(new_pid):
    global last_pid
    if new_pid == last_pid:
        return
        
    subprocess.run(['taskset', '-pc', '0-3', str(new_pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    if last_pid:
        subprocess.run(['taskset', '-pc', '2,3', str(last_pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        
    last_pid = new_pid

def listen_to_ipc():
    sock_path = get_hyprland_socket()
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(sock_path)
        while True:
            data = s.recv(4096).decode('utf-8')
            if not data:
                break
            if "activewindow" in data:
                pid = get_active_window_pid()
                if pid:
                    apply_vayu_logic(pid)

if __name__ == "__main__":
    listen_to_ipc()
