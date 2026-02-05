#define BENCH_NAME template

#include <stdio.h>
#include "socket_memory.h"
#include "benchmark.h"

#define CONCAT(a, b) a##b
#define STRINGIFY(x) #x
#define EXPAND_AND_STRINGIFY(x) STRINGIFY(x)

void CONCAT(BENCH_NAME, _init)(void* addr_list, int* primary_cores, int* secondary_cores, int* orchestrator_cores) {
    printf("%s: Initialization\n", EXPAND_AND_STRINGIFY(BENCH_NAME));
    // void* (*address_list)[NUM_CHA][MAX_ADDRESSES] = addr_list;
}

void CONCAT(BENCH_NAME, _roi)(void* addr_list, int* primary_cores, int* secondary_cores, int* orchestrator_cores) {
    printf("%s: Running Region of Interest\n", EXPAND_AND_STRINGIFY(BENCH_NAME));
    // void* (*address_list)[NUM_CHA][MAX_ADDRESSES] = addr_list;

}

void CONCAT(BENCH_NAME, _cleanup)(void* addr_list, int* primary_cores, int* secondary_cores, int* orchestrator_cores) {
    printf("%s: Cleanup\n", EXPAND_AND_STRINGIFY(BENCH_NAME));
    // void* (*address_list)[NUM_CHA][MAX_ADDRESSES] = addr_list;
}

Benchmark benchmark = {
    EXPAND_AND_STRINGIFY(BENCH_NAME),
    CONCAT(BENCH_NAME, _init),
    CONCAT(BENCH_NAME, _roi),
    CONCAT(BENCH_NAME, _cleanup)
};
