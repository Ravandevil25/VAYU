#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/*
 * VAYU kernel BPF program.
 * Shared map with the userspace daemon.
 * Key: PID. Value: 0 for active, 1 for background.
 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, int);
    __type(value, int);
    __uint(max_entries, 4096);
} vayu_pid_map SEC(".maps");

/*
 * sched_wakeup tracepoint.
 * Looks up the waking PID in the VAYU map. Active entries are left alone;
 * background entries are candidates for restriction under a full sched_ext
 * implementation.
 */
SEC("tp/sched/sched_wakeup")
int vayu_enforce_affinity(void *ctx) {
    int pid = bpf_get_current_pid_tgid() >> 32;
    
    int *status = bpf_map_lookup_elem(&vayu_pid_map, &pid);
    if (!status) {
        return 0;
    }

    if (*status == 0) {
        // Active process: no action.
    } else {
        // Background process: restrict under a full sched_ext implementation.
    }

    return 0;
}

char _license[] SEC("license") = "GPL";
