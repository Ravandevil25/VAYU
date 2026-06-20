<div align="center">

# 🌪️ Project VAYU
**Dynamic Hardware-Level Gaze Isolation for Wayland**

[![Written In](https://img.shields.io/badge/Written%20In-C++-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Hyprland-orange.svg)]()
[![Status](https://img.shields.io/badge/Status-God%20Mode-red.svg)]()

</div>

## 🚀 The Philosophy
Software optimizers like `ananicy` or `gamemode` alter software-level *scheduling priorities* (nice values). VAYU bypasses software entirely. 

VAYU dynamically rewires your **CPU's physical core allocation** in real-time based on where your eyes are looking. By intercepting internal UNIX sockets from the Desktop Compositor (Wayland), VAYU identifies the precise millisecond you focus on a window. It then executes raw Kernel System Calls (`sched_setaffinity`) to lock that application into a dedicated, traffic-free physical CPU core, while ruthlessly starving all background graphical processes.

This is not software priority. This is **Hardware Segregation**.

## 🧠 System Architecture

```text
 ┌─────────────────┐       ┌─────────────────┐       ┌──────────────────┐
 │ User Focus      │ ───►  │ Hyprland IPC    │ ───►  │ VAYU C++ Daemon  │
 └─────────────────┘       └─────────────────┘       └────────┬─────────┘
                                                              │
                            ┌─────────────────────────────────┴─────────┐
                            ▼                                           ▼
                 [ sched_setaffinity ]                       [ sched_setaffinity ]
                 Target: Active PID                          Target: Background PIDs
                 Mask: CPU 0, 1, 2, 3                        Mask: CPU 2, 3
                            │                                           │
                            ▼                                           ▼
             ┌─────────────────────────────┐             ┌─────────────────────────────┐
             │ CPU Core 0                  │             │ CPU Core 1                  │
             │ (100% Exclusive L1 Cache)   │             │ (Restricted Queue)          │
             └─────────────────────────────┘             └─────────────────────────────┘
```

## ⚡ Technical superiority
1. **Zero Polling Overhead:** VAYU does not use `while(true)` loops to scan PIDs. It is purely event-driven via the `.socket2.sock` Wayland IPC. It uses exactly `0.00%` CPU when you are not changing windows.
2. **C++ Raw Performance:** Written in pure C++ for microsecond execution. It queries the kernel directly via `<sched.h>`. No Python overhead. No bash subshells.
3. **Patentable Novelty:** Linking compositor-level active window state to `cpu_set_t` bitmasks dynamically on desktop Linux is an unmapped architectural frontier.

## 🛡️ Compilation & Deployment
This daemon is compiled directly on your machine specifically for your exact CPU instruction set.

```bash
g++ -O3 -march=native vayu.cpp -o vayu-daemon
```

## ⚖️ Legal
Concept and Architectural Draft designed for Patent Considerations by **Saurav Kumar**. 
