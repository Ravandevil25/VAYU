# VAYU changelog

## v6.0: eBPF kernel module

- Added `vayu_kernel.bpf.c`: eBPF program hooking `tp/sched/sched_wakeup` (or `scx_ops` for `sched_ext`) that consults the PID status map populated by the daemon.
- Added Clang-based BPF build target in `Makefile` producing `vayu_kernel.bpf.o`.

## v5.0: eBPF userspace bridge

- Daemon creates a `BPF_MAP_TYPE_HASH` map (key: PID, value: status) via `bpf(BPF_MAP_CREATE)` and updates it on every affinity migration.
- Added `CAP_BPF` and `CAP_SYS_ADMIN` to the required capability set.
- Limitation: without a kernel-side consumer the map is write-only telemetry.

## v4.0: Linux capabilities

- Deployment switched to file capabilities (`setcap cap_sys_nice,...`) instead of running as root, enabling negative nice values and scheduler override.
- Fixed `kill(pid, 0)` handling: `EPERM` (process exists, no permission) no longer triggers removal from the managed set.

## v3.0: PID lifecycle and whitelist

- Added `kill(pid, 0)` heartbeat with `ESRCH` handling to drop dead PIDs and tolerate PID reuse.
- Added `~/.config/vayu/whitelist.conf` to exempt critical daemons (pipewire, wireplumber, obs, docker).
- Added `setpriority()` calls (`-10` active, `+5` background). Note: negative values require `CAP_SYS_NICE`, otherwise the call fails with `EACCES`.

## v2.0: C++ rewrite

- Rewrote the daemon in C++ with direct `sched_setaffinity` calls (no `taskset`/`hyprctl` subprocesses).
- Added recursive traversal of `/proc/<pid>/task` to cover all child processes and threads.
- Builds with generic optimization flags (`-O2`); no `-march=native`.

## v1.0: Proof of concept (Python)

- Python script on the Hyprland event socket (`.socket2.sock`) using `hyprctl activewindow` and `taskset` to pin the active window to core 0 and background windows to cores 2-3.
- Superseded due to subprocess overhead per focus event and single-PID targeting.
