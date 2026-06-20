# VAYU: Dynamic Hardware-Level Core Isolation via Compositor Events
## Draft for Patent Application & System Architecture

### 1. The Problem in Modern Computing
Modern operating systems (Windows, macOS, Linux) rely on CPU schedulers (like CFS or EEVDF) that use "Time Slicing." When multiple applications are open, the CPU divides its time into milliseconds, giving a tiny slice to every app. 
**The Flaw:** When a user is interacting with an application (e.g., typing in a code editor), background applications (e.g., a browser tab running heavy JavaScript) can still steal CPU cycles and L1 cache, causing micro-stutters and input lag, especially on low-core processors.

### 2. The VAYU Innovation (The Patentable Concept)
VAYU introduces **"Gaze-Based Hardware Partitioning."** 
Instead of letting the kernel guess what is important, VAYU intercepts raw event streams directly from the Desktop Compositor (Wayland/Hyprland). 

**The Algorithm:**
1. **Event Interception:** VAYU listens to the UNIX Domain Socket of the graphical interface. 
2. **Sub-Millisecond Trigger:** The exact microsecond a user focuses on a window, an event is fired.
3. **Hardware Affinitization:** VAYU instantly executes system calls (`sched_setaffinity` or `cgroups v2`) to physically restrict all background applications to a specific subset of CPU threads.
4. **Absolute Priority:** The focused application is granted 100% exclusive access to the primary physical CPU core and its L1/L2 cache.

### 3. Why This is Novel
Current solutions (like `nice`, `cpulimit`, or `ananicy`) only adjust *software scheduling priority*. VAYU alters *physical hardware core assignment* dynamically in real-time based on graphical user focus. It turns a multi-tasking processor into a hyper-focused single-tasking processor dynamically.

### 4. Technical Stack
- **Language:** Python (Prototype) -> C++ (Production)
- **Interface:** Hyprland IPC (Inter-Process Communication)
- **Kernel API:** Linux Control Groups (cgroups v2), CPU Affinity (`taskset`)
