# VAYU

Focus-aware dynamic CPU affinity manager for Wayland compositors.

## Overview

VAYU reduces CFS contention on desktop Linux by migrating thread trees based on window focus events. On each `activewindow` event from the compositor IPC socket, the daemon resolves the foreground PID, enumerates its full process and thread tree via `procfs`, and applies `sched_setaffinity` masks: the active tree runs unrestricted while previously active trees are restricted to background cores. This improves cache locality for the foreground application.

## Architecture

```text
Window focus -> Wayland IPC socket -> VAYU daemon -> sched_setaffinity
                                                   Active tree: unrestricted mask
                                                   Background trees: restricted mask
```

- Blocking I/O on the compositor event socket; no polling while idle
- Recursive thread discovery via `/proc/<pid>/task` and `/proc/*/stat` (covers multi-process applications such as Chromium)
- PID lifecycle tracking via `kill(pid, 0)` with `ESRCH`/`EPERM` handling to tolerate PID reuse
- Configurable process whitelist (`~/.config/vayu/whitelist.conf`, see `whitelist.example`) to exempt audio and system daemons
- Optional eBPF map (`BPF_MAP_TYPE_HASH`, PID -> status) populated from userspace; kernel-side enforcement via `vayu_kernel.bpf.o` where loaded

## Requirements

- Linux with Wayland compositor exposing an IPC event socket (Hyprland supported)
- `CAP_SYS_NICE` for negative nice values and affinity override; `CAP_BPF` and `CAP_SYS_ADMIN` for eBPF map creation

## Build and install

```bash
make all
sudo make install
sudo setcap 'cap_sys_nice,cap_sys_admin,cap_bpf+ep' /usr/bin/vayu-daemon
systemctl --user daemon-reload
systemctl --user enable --now vayu.service
```

`make install` respects `DESTDIR` and `PREFIX` (default `/usr`). See `Makefile` for installed paths.

Manual setup is also available via `install.sh`.

## Configuration

Whitelist file: `~/.config/vayu/whitelist.conf` (one process name or substring per line, `#` for comments). See `whitelist.example`.

## Limitations

- CPU masks are currently fixed for a 4-core topology (active: cores 0-3, background: cores 2-3). Other topologies require source adjustment.
- Hyprland IPC only. Other compositors are not supported.
- eBPF map creation requires elevated capabilities; without them the daemon falls back to syscall-only mode.

## License

MIT. See `LICENSE`.
