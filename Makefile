CLANG ?= clang
ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/i386/x86/')
BPF_TARGET := bpf

all: vayu_kernel.bpf.o vayu-daemon

vayu_kernel.bpf.o: vayu_kernel.bpf.c
	$(CLANG) -g -O2 -target $(BPF_TARGET) -D__TARGET_ARCH_$(ARCH) -I/usr/include/bpf -c $< -o $@

vayu-daemon: vayu.cpp
	g++ -O3 -march=native vayu.cpp -o vayu-daemon

clean:
	rm -f *.o vayu-daemon
