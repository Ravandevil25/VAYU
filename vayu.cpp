#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sched.h>
#include <dirent.h>
#include <csignal>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <linux/bpf.h>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <memory>

using namespace std;

string his_env;
string xdg_env;
set<int> managed_pids;
vector<int> current_active_tree;
vector<string> whitelist = {"pipewire", "wireplumber", "obs", "docker"};

// eBPF Map File Descriptor
int bpf_map_fd = -1;

// Wrapper for the bpf() system call
int bpf_syscall(int cmd, union bpf_attr *attr, unsigned int size) {
    return syscall(__NR_bpf, cmd, attr, size);
}

// Initialize the eBPF Hash Map in the Kernel
void init_ebpf_map() {
    union bpf_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.map_type = BPF_MAP_TYPE_HASH;
    attr.key_size = sizeof(int);   // Key: PID
    attr.value_size = sizeof(int); // Value: Status (0 = Active, 1 = Background)
    attr.max_entries = 4096;

    bpf_map_fd = bpf_syscall(BPF_MAP_CREATE, &attr, sizeof(attr));
    if (bpf_map_fd < 0) {
        cerr << "VAYU WARNING: Failed to create eBPF Map (Requires Root/CAP_SYS_ADMIN). Running in standard syscall mode." << endl;
    } else {
        cout << "VAYU SUCCESS: eBPF Kernel Map created! Bridge established." << endl;
    }
}

// Update the eBPF Map for a specific PID
void update_ebpf_map(int pid, int status) {
    if (bpf_map_fd < 0) return;
    
    union bpf_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.map_fd = bpf_map_fd;
    attr.key = reinterpret_cast<__u64>(&pid);
    attr.value = reinterpret_cast<__u64>(&status);
    attr.flags = BPF_ANY;
    
    bpf_syscall(BPF_MAP_UPDATE_ELEM, &attr, sizeof(attr));
}

// Remove dead PIDs from the eBPF Map
void delete_ebpf_map(int pid) {
    if (bpf_map_fd < 0) return;
    
    union bpf_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.map_fd = bpf_map_fd;
    attr.key = reinterpret_cast<__u64>(&pid);
    
    bpf_syscall(BPF_MAP_DELETE_ELEM, &attr, sizeof(attr));
}

void load_whitelist() {
    string path = string(getenv("HOME")) + "/.config/vayu/whitelist.conf";
    ifstream file(path);
    string line;
    if(file.is_open()) {
        whitelist.clear();
        while(getline(file, line)) {
            if(line.length() > 0 && line[0] != '#') whitelist.push_back(line);
        }
        file.close();
    }
}

bool is_whitelisted(int pid) {
    char comm_path[256];
    snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);
    ifstream comm_file(comm_path);
    string proc_name;
    if(comm_file >> proc_name) {
        for (const string& w : whitelist) {
            if (proc_name.find(w) != string::npos) return true;
        }
    }
    return false;
}

void cleanup_dead_pids() {
    for (auto it = managed_pids.begin(); it != managed_pids.end(); ) {
        if (kill(*it, 0) == -1 && errno == ESRCH) {
            delete_ebpf_map(*it); // Clean eBPF Map
            it = managed_pids.erase(it);
        } else {
            ++it;
        }
    }
}

int get_active_pid() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    string socket_path = xdg_env + "/hypr/" + his_env + "/.socket.sock";
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sock);
        return -1;
    }

    const char* cmd = "j/activewindow";
    write(sock, cmd, strlen(cmd));

    char buffer[4096];
    string result = "";
    while (true) {
        ssize_t bytes = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        result += buffer;
    }
    close(sock);

    size_t pos = result.find("\"pid\":");
    if (pos != string::npos) {
        return stoi(result.substr(pos + 6));
    }
    return -1;
}

void get_children(int ppid, vector<int>& children) {
    DIR* proc = opendir("/proc");
    if (!proc) return;
    struct dirent* ent;
    while ((ent = readdir(proc))) {
        if (!isdigit(ent->d_name[0])) continue;
        int pid = atoi(ent->d_name);
        char stat_path[256];
        snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
        FILE* f = fopen(stat_path, "r");
        if (f) {
            int file_pid; char comm[256]; char state; int file_ppid;
            if (fscanf(f, "%d %s %c %d", &file_pid, comm, &state, &file_ppid) == 4) {
                if (file_ppid == ppid) {
                    children.push_back(pid);
                    get_children(pid, children); 
                }
            }
            fclose(f);
        }
    }
    closedir(proc);
}

void apply_affinity_and_nice(int pid, bool is_background) {
    if (is_whitelisted(pid)) return;

    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (is_background) {
        CPU_SET(2, &mask);
        CPU_SET(3, &mask);
        setpriority(PRIO_PROCESS, pid, 5); 
        update_ebpf_map(pid, 1); // Inform eBPF Kernel: Background
    } else {
        CPU_SET(0, &mask);
        CPU_SET(1, &mask);
        CPU_SET(2, &mask);
        CPU_SET(3, &mask);
        setpriority(PRIO_PROCESS, pid, -10); 
        update_ebpf_map(pid, 0); // Inform eBPF Kernel: Active
    }

    char task_path[256];
    snprintf(task_path, sizeof(task_path), "/proc/%d/task", pid);
    DIR* task_dir = opendir(task_path);
    if (task_dir) {
        struct dirent* ent;
        while ((ent = readdir(task_dir))) {
            if (!isdigit(ent->d_name[0])) continue;
            int tid = atoi(ent->d_name);
            sched_setaffinity(tid, sizeof(cpu_set_t), &mask);
        }
        closedir(task_dir);
    }
}

void vayu_core_logic(int new_pid) {
    cleanup_dead_pids();
    
    vector<int> new_tree = {new_pid};
    get_children(new_pid, new_tree);

    for (int old_pid : managed_pids) {
        if (find(new_tree.begin(), new_tree.end(), old_pid) == new_tree.end()) {
            apply_affinity_and_nice(old_pid, true);
        }
    }

    for (int pid : new_tree) {
        apply_affinity_and_nice(pid, false);
        managed_pids.insert(pid);
    }
    
    current_active_tree = new_tree;
}

int main() {
    if (getenv("HYPRLAND_INSTANCE_SIGNATURE")) his_env = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (getenv("XDG_RUNTIME_DIR")) xdg_env = getenv("XDG_RUNTIME_DIR");
    
    if (his_env.empty() || xdg_env.empty()) {
        cerr << "VAYU FATAL: Wayland Environment missing." << endl;
        return 1;
    }

    load_whitelist();
    init_ebpf_map(); // Boot up the eBPF Kernel Bridge

    string socket_path = xdg_env + "/hypr/" + his_env + "/.socket2.sock";
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        cerr << "VAYU FATAL: Failed to connect to IPC." << endl;
        return 1;
    }

    cout << "VAYU 5.0 (eBPF Edition) Core Online." << endl;

    char buffer[1024];
    while (true) {
        ssize_t bytes = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        string data(buffer);
        
        if (data.find("activewindow>>") != string::npos) {
            int pid = get_active_pid();
            if (pid > 0 && (current_active_tree.empty() || pid != current_active_tree[0])) {
                vayu_core_logic(pid);
            }
        }
    }
    close(sock);
    return 0;
}
