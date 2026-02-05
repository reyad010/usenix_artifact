#define BENCH_NAME benchmark4

#include <stdio.h>
#include "socket_memory.h"
#include "benchmark.h"
#include "util.h"

#define CONCAT(a, b) a##b
#define STRINGIFY(x) #x
#define EXPAND_AND_STRINGIFY(x) STRINGIFY(x)

// Access S3 memory: S1 read -> S2 read -> S0 read+check

void CONCAT(BENCH_NAME, _init)(void* addr_list, int* primary_cores, int* secondary_cores, int* orchestrator_cores) {

    void* (*address_list)[NUM_CHA][MAX_ADDRESSES] = addr_list;

    for (int cha = 0; cha < NUM_CHA; cha++) {
        for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
            void* target = address_list[3][cha][addr];
            flush(target);
            mfence();
        }
    }

    for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
        set_process_affinity(primary_cores[1]);
        for (int cha = 0; cha < NUM_CHA; cha++) {
            void* target = address_list[3][cha][addr];
            maccess(target);
            mfence();
        }

        set_process_affinity(primary_cores[2]);
        for (int cha = 0; cha < NUM_CHA; cha++) {
            void* target = address_list[3][cha][addr];
            maccess(target);
            mfence();
        }
    }

    set_process_affinity(primary_cores[0]);
    for (int cha = 0; cha < NUM_CHA; cha++) {
        for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
            void* target = address_list[3][cha][addr];
            mfence();
        }
    }
}

void CONCAT(BENCH_NAME, _roi)(void* addr_list, int* primary_cores, int* secondary_cores, int* orchestrator_cores) {

    void* (*address_list)[NUM_CHA][MAX_ADDRESSES] = addr_list;

    // set_process_affinity(primary_cores[3]);
    for (int cha = 0; cha < NUM_CHA; cha++) {
        for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
            void* target = address_list[3][cha][addr];
            maccess(target);
            mfence();
        }
    }
}

void CONCAT(BENCH_NAME, _cleanup)(void* addr_list, int* primary_cores, int* secondary_cores, int* orchestrator_cores) {

    void* (*address_list)[NUM_CHA][MAX_ADDRESSES] = addr_list;

    for (int cha = 0; cha < NUM_CHA; cha++) {
        for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
            void* target = address_list[3][cha][addr];
            flush(target);
            mfence();
        }
    }
}

Benchmark benchmark = {
    EXPAND_AND_STRINGIFY(BENCH_NAME),
    CONCAT(BENCH_NAME, _init),
    CONCAT(BENCH_NAME, _roi),
    CONCAT(BENCH_NAME, _cleanup)
};
