# VAYU: A System and Method for Focus-Aware Dynamic CPU Affinity Management in Wayland Compositors

## 1. Abstract
The VAYU system introduces a method for dynamically reallocating CPU affinity of graphical applications in real-time, functioning in direct response to compositor-generated focus events. By leveraging UNIX domain sockets for zero-polling event synchronization, recursive thread discovery via the `procfs` virtual filesystem, and kernel-level affinity migration (`sched_setaffinity`), the system reduces Completely Fair Scheduler (CFS) contention and improves cache locality for foreground tasks, while explicitly segregating background workloads.

## 2. Background and Prior Art
Traditional desktop Linux performance optimization relies heavily on adjusting software scheduling priorities (e.g., `nice` and `renice` commands) or static Control Groups (`cgroups`). Daemons like `ananicy` adjust `nice` values dynamically, but they do not physically restrict threads to specific CPU cores based on immediate user gaze/focus. 

While CPU affinity (`sched_setaffinity`) and process pinning have existed in server environments for decades, integrating these kernel APIs with real-time GUI focus events via a Wayland compositor IPC represents a novel architectural combination on desktop Linux.

## 3. System Architecture and Methodology
The VAYU method operates via the following continuous pipeline:
1. **Event Synchronization:** The daemon connects to the Wayland compositor's UNIX domain socket, waiting in a blocked I/O state (`sleep`) until an `activewindow` event is broadcast.
2. **Real-Time PID Discovery:** Upon receiving the focus event, the daemon extracts the Process ID (PID) of the active graphical client.
3. **Thread Tree Enumeration:** The system traverses `/proc/<pid>/task/` and `/proc/*/stat` to recursively identify all child processes and execution threads associated with the target application, ensuring multi-process architectures (e.g., Chromium-based browsers) are fully captured.
4. **Affinity Migration:** The daemon issues `sched_setaffinity` system calls to migrate the active thread tree to an unrestricted CPU mask, whilst simultaneously migrating previously active threads to a restricted background CPU mask.
5. **PID Reuse Protection & Whitelisting:** The system utilizes `kill(pid, 0)` to verify process existence, preventing memory leaks and erroneous affinity assignments caused by PID recycling. Critical system services are bypassed via a configurable policy engine.

## 4. Primary Patentable Claims
1. A method for dynamic CPU allocation comprising: receiving a window focus event from a graphical display server via inter-process communication; identifying the primary Process ID of the focused window; recursively enumerating all associated execution threads; and modifying the CPU affinity mask of said threads in real-time.
2. The method of claim 1, further comprising the simultaneous migration of all previously focused process threads to a restricted subset of CPU cores to segregate background workloads.
3. A system executing the method of claim 1, utilizing a zero-polling blocked I/O architecture on a UNIX domain socket to eliminate idle CPU overhead.
