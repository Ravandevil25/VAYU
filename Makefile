CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra
CLANG ?= clang
PREFIX ?= /usr
ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/i386/x86/')
BPF_TARGET := bpf

all: vayu_kernel.bpf.o vayu-daemon

vayu_kernel.bpf.o: vayu_kernel.bpf.c
	$(CLANG) -g -O2 -target $(BPF_TARGET) -D__TARGET_ARCH_$(ARCH) -c $< -o $@

vayu-daemon: vayu.cpp
	$(CXX) $(CXXFLAGS) vayu.cpp -o vayu-daemon

install: all
	install -Dm755 vayu-daemon "$(DESTDIR)$(PREFIX)/bin/vayu-daemon"
	install -Dm644 vayu_kernel.bpf.o "$(DESTDIR)$(PREFIX)/lib/vayu/vayu_kernel.bpf.o"
	install -Dm644 vayu.service "$(DESTDIR)$(PREFIX)/lib/systemd/user/vayu.service"
	install -Dm644 whitelist.example "$(DESTDIR)$(PREFIX)/share/doc/vayu/whitelist.example"
	install -Dm644 README.md "$(DESTDIR)$(PREFIX)/share/doc/vayu/README.md"
	install -Dm644 VAYU_Whitepaper.md "$(DESTDIR)$(PREFIX)/share/doc/vayu/VAYU_Whitepaper.md"
	install -Dm644 LICENSE "$(DESTDIR)$(PREFIX)/share/licenses/vayu/LICENSE"

clean:
	rm -f *.o vayu-daemon
