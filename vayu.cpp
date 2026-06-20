#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sched.h>
#include <memory>
#include <array>
#include <vector>

using namespace std;

int last_pid = -1;

// Ultra-fast query to Hyprland for active PID without loading heavy JSON libs
int get_active_pid() {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen("hyprctl activewindow -j 2>/dev/null", "r"), pclose);
    if (!pipe) return -1;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    size_t pos = result.find("\"pid\":");
    if (pos != string::npos) {
        return stoi(result.substr(pos + 6));
    }
    return -1;
}

// Hardware-level Kernel call for Core Affinity
void set_affinity(int pid, bool is_background) {
    if (pid <= 0) return;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    
    if (is_background) {
        // Starve the background app: lock to secondary cores (2,3)
        CPU_SET(2, &mask);
        CPU_SET(3, &mask);
    } else {
        // God Mode for active app: Full access to all cores (0,1,2,3)
        CPU_SET(0, &mask);
        CPU_SET(1, &mask);
        CPU_SET(2, &mask);
        CPU_SET(3, &mask);
    }
    
    // Direct system call, bypassing software schedulers
    sched_setaffinity(pid, sizeof(cpu_set_t), &mask);
}

void vayu_core_logic(int new_pid) {
    if (new_pid == last_pid) return;
    
    // 1. Instantly boost the new window
    set_affinity(new_pid, false);
    
    // 2. Choke the previous window
    if (last_pid > 0) {
        set_affinity(last_pid, true);
    }
    
    last_pid = new_pid;
}

int main() {
    const char* his = getenv("HYPRLAND_INSTANCE_SIGNATURE");
    const char* xdg = getenv("XDG_RUNTIME_DIR");
    
    if (!his || !xdg) {
        cerr << "VAYU FATAL: Wayland/Hyprland Environment variables missing." << endl;
        return 1;
    }

    string socket_path = string(xdg) + "/hypr/" + string(his) + "/.socket2.sock";

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        cerr << "VAYU FATAL: Failed to connect to Hyprland IPC." << endl;
        return 1;
    }

    cout << "VAYU C++ Core Online. Listening to Wayland IPC..." << endl;

    char buffer[1024];
    while (true) {
        ssize_t bytes = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        string data(buffer);
        
        // Zero-latency event trigger
        if (data.find("activewindow>>") != string::npos) {
            int pid = get_active_pid();
            if (pid > 0) {
                vayu_core_logic(pid);
            }
        }
    }
    close(sock);
    return 0;
}
