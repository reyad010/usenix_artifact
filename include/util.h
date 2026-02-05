#ifndef UTIL_H
#define UTIL_H

#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define DEBUG 0  // Set to 0 to disable debugging

#if DEBUG
    #define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: " fmt "\n", ##args)
#else
    #define DEBUG_PRINT(fmt, args...)
#endif

#define MAX_SOCKETS 2
#define MAX_CORES_PER_SOCKET 2

#define CACHE_LINE_SIZE 64  // 64-byte alignment
#define PAGE_SIZE 4096 // 4KB pages

extern int primary_cores[MAX_SOCKETS];
extern int secondary_cores[MAX_SOCKETS];
extern int orchestrator_cores[MAX_SOCKETS];

static inline void flush(void *p) { asm volatile("clflush 0(%0)"::"r"(p): "rax"); }
static inline void maccess(void *p) { asm volatile("movq (%0), %%rax"::"r"(p): "rax"); }
static inline void mmodify(void *p) { asm volatile("movq $0x1, (%0)"::"r"(p): "memory"); }
static inline void mfence() { asm volatile("mfence"); }

uint64_t rdtsc();

void set_process_affinity(int core_id);
void find_primary_secondary_cores_per_socket();
void execute_on_socket_core(int socket_id, int use_secondary, void (*func)(void *), void *arg, int old_core_id);

void display_progress(const char *label, int current, int total);

#endif
