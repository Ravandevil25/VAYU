#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sched.h>
#include <dirent.h>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

string his_env;
string xdg_env;
set<int> managed_pids;
vector<int> current_active_tree;

// Direct UNIX Socket call to Hyprland (Zero Forking!)
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

// Deep Process Tree Traversal
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
                    get_children(pid, children); // Recursive deep search
                }
            }
            fclose(f);
        }
    }
    closedir(proc);
}

// Apply Hardware Affinity to ALL threads of a PID
void apply_affinity(int pid, bool is_background) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (is_background) {
        CPU_SET(2, &mask);
        CPU_SET(3, &mask);
    } else {
        CPU_SET(0, &mask);
        CPU_SET(1, &mask);
        CPU_SET(2, &mask);
        CPU_SET(3, &mask);
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
    // 1. Build the full tree for the new active window
    vector<int> new_tree = {new_pid};
    get_children(new_pid, new_tree);

    // 2. Choke all previously managed PIDs that are NOT in the new tree
    for (int old_pid : managed_pids) {
        if (find(new_tree.begin(), new_tree.end(), old_pid) == new_tree.end()) {
            apply_affinity(old_pid, true);
        }
    }

    // 3. Boost all PIDs in the new tree and add to managed list
    for (int pid : new_tree) {
        apply_affinity(pid, false);
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

    cout << "VAYU 2.0 Core Online. Advanced Thread Traversal Active." << endl;

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
