# 🌪️ Project VAYU: Evolutionary History (v1.0 - v6.0)

This document chronicles the architectural evolution of Project VAYU, detailing the paradigm shifts, critical flaws discovered in previous versions, and the engineering leaps taken to resolve them.

---

### 🔹 VAYU 1.0: The Proof of Concept (Python)
**What was developed:**
* A lightweight Python script connecting to the Hyprland UNIX socket (`.socket2.sock`).
* Used standard `subprocess` to call `hyprctl activewindow` to extract the Process ID (PID).
* Executed shell commands (`taskset`) to pin the active window to Core 0, and background windows to Cores 2-3.

**The Flaw / Reason for Upgrade:**
* **Overhead:** Python and spawning shell sub-processes (`taskset`, `hyprctl`) for every window focus change introduced unacceptable latency and CPU overhead.
* **The Process Tree Illusion:** Targeting only the primary PID meant multi-process applications (like Chromium browsers) still had dozens of hidden rendering threads stealing CPU cycles on Core 0.

---

### 🔹 VAYU 2.0: Deep Thread Traversal (Pure C++)
**What was developed:**
* Completely rewritten in **C++** compiled with `-O3 -march=native`.
* Replaced shell commands with direct kernel API calls (`<sched.h>` -> `sched_setaffinity`).
* **Deep Process Traversal:** Implemented a recursive filesystem scanner that traverses `/proc/<pid>/task` to discover and affinitize *every single child process and thread* linked to the active window.

**The Flaw / Reason for Upgrade:**
* **Memory Leaks (PID Recycling):** When an app was closed, its PID remained in VAYU's memory array. If Linux reused that PID for a new process, VAYU would maliciously restrict it to background cores.
* **System Starvation:** Heavy system tasks (like audio servers) were being treated as "background GUI apps" and restricted, causing audio stuttering.

---

### 🔹 VAYU 3.0: Garbage Collection & Whitelist Engine
**What was developed:**
* **Garbage Collector:** Implemented a `kill(pid, 0)` heartbeat monitor. If a process returned `ESRCH` (No Such Process), VAYU safely erased it from memory, eliminating PID-reuse bugs.
* **God-Tier Whitelisting:** Added `~/.config/vayu/whitelist.conf` to dynamically protect critical daemons (`pipewire`, `obs`, `docker`) from core restriction.
* **Software Priority Injection:** Added `setpriority()` (renice) calls to assign `-10` software priority to active windows, and `+5` to background windows.

**The Flaw / Reason for Upgrade:**
* **The "Fake" God Mode:** A security audit revealed that the `-10` software priority injection was silently failing in the background. Standard Linux users are forbidden by the kernel from assigning negative nice values, throwing an `EACCES` error.

---

### 🔹 VAYU 4.0: Capability Injection (True God Mode)
**What was developed:**
* **CAP_SYS_NICE:** Restructured the deployment architecture. Injected the `CAP_SYS_NICE` Linux Kernel capability directly into the C++ binary's metadata using `setcap`.
* **Result:** VAYU gained the absolute authority to override the Linux CFS Scheduler without running as a dangerous global `root` daemon. 
* Fixed a false-positive bug where `kill()` returning `EPERM` for root processes triggered accidental deletion from the management array.

**The Flaw / Reason for Upgrade:**
* Even with maximum user-space authority, VAYU was still constrained to the "User Space". It had to wait for the kernel to obey its affinity requests, creating microscopic contention delays.

---

### 🔹 VAYU 5.0: The eBPF User-Kernel Bridge
**What was developed:**
* VAYU became the first desktop compositor tool to natively bridge User-Space and Kernel-Space.
* **eBPF Hash Maps:** The C++ daemon utilized the `sys_bpf` syscall to allocate a shared Hash Map directly inside Linux Kernel Memory.
* Instead of just pushing software priorities, VAYU began writing the active/background PID trees directly into the kernel's high-speed memory in real-time. Injected `CAP_BPF` and `CAP_SYS_ADMIN`.

**The Flaw / Reason for Upgrade:**
* The eBPF Map existed in the kernel and was updated perfectly by User Space, but there was no native Kernel BPF program compiled to *read* this map and forcefully block thread wakeups at the silicon tracepoint level.

---

### 🔹 VAYU 6.0: Raw eBPF Kernel Module
**What was developed:**
* **The Kernel Hook (`vayu_kernel.bpf.c`):** Wrote a raw C eBPF kernel program designed to hook into `tp/sched/sched_wakeup` (or `scx_ops` for `sched_ext`). 
* This program actively queries the eBPF Map populated by VAYU 5.0. If a background process attempts to wake up on a restricted core, the kernel drops or delays it *before* it ever leaves the CPU queue.
* **LLVM/Clang Build System:** Built an enterprise-grade `Makefile` to compile the C code into BPF Bytecode (`.o`).
* Kept entirely secure in a private repository to serve as the ultimate, pristine architectural blueprint.