<div align="center">

# 🌪️ Project VAYU
**Focus-Aware Dynamic CPU Affinity Manager for Wayland**

[![Written In](https://img.shields.io/badge/Written%20In-C++-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Wayland-orange.svg)]()
[![Status](https://img.shields.io/badge/Status-Stable%20Release-green.svg)]()

</div>

## 🚀 Overview
VAYU is a highly optimized C++ systems daemon designed to mitigate CFS (Completely Fair Scheduler) contention on desktop Linux. Unlike traditional optimizers that rely on modifying static software `nice` values, VAYU performs **Real-Time Dynamic Affinity Migration**.

By synchronizing with Wayland compositor IPC events, VAYU identifies foreground graphical applications in sub-milliseconds, enumerates their entire process tree via `procfs`, and executes `sched_setaffinity` system calls. This isolates background workloads to specific CPU cores and reduces L1/L2 cache thrashing for the user's active application.

## 🧠 System Architecture

```text
 ┌─────────────────┐       ┌─────────────────┐       ┌──────────────────┐
 │ User Focus      │ ───►  │ Wayland IPC     │ ───►  │ VAYU C++ Daemon  │
 └─────────────────┘       └─────────────────┘       └────────┬─────────┘
                                                              │
                            ┌─────────────────────────────────┴─────────┐
                            ▼                                           ▼
                 [ sched_setaffinity ]                       [ sched_setaffinity ]
                 Target: Active PID Tree                     Target: Background PID Trees
                 Mask: Unrestricted (0, 1, 2, 3)             Mask: Restricted (2, 3)
                            │                                           │
                            ▼                                           ▼
             ┌─────────────────────────────┐             ┌─────────────────────────────┐
             │ Unrestricted Execution      │             │ Segregated Execution        │
             │ (High Cache Locality)       │             │ (High Contention Zone)      │
             └─────────────────────────────┘             └─────────────────────────────┘
```

## ⚡ Key Features
1. **eBPF Kernel Bridge (VAYU 5.0):** VAYU pioneers the User-Space to Kernel-Space eBPF pipeline for desktop compositors. The daemon natively creates an eBPF Hash Map in the Linux Kernel using `sys_bpf`. Real-time active and background PID trees are written directly into kernel memory. Future integration with `sched_ext` (SCX) allows the kernel to natively read this map and block context switches at the silicon level.
2. **Recursive Thread Discovery:** Parses `/proc/<pid>/task` to dynamically identify and migrate every child process and execution thread of multi-process applications (e.g., Chromium).
3. **Zero-Polling IPC Hook:** Utilizes blocking I/O on UNIX domain sockets (`.socket.sock` & `.socket2.sock`) for 0.00% idle CPU overhead.
4. **Orphan Protection:** Actively tracks PID lifecycles utilizing `kill(pid, 0)` with robust `EPERM` vs `ESRCH` handling to prevent memory leaks from OS PID recycling.
5. **Daemon Whitelisting:** Extensible policy engine (`~/.config/vayu/whitelist.conf`) to bypass real-time audio and critical system daemons (e.g., `pipewire`).

## 🛡️ Deployment
VAYU must be compiled natively for your target CPU architecture and requires `CAP_SYS_NICE` and `CAP_BPF` capabilities to override the kernel scheduler and interact with eBPF maps.

```bash
# Compile both the C++ Daemon and the eBPF Kernel Module
make all

# Inject Kernel Capabilities
sudo setcap 'cap_sys_nice,cap_sys_admin,cap_bpf+ep' vayu-daemon
```

## ⚖️ Legal
Concept and Architectural Draft by **Saurav Kumar**. 
