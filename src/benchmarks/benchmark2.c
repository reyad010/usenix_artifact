#define BENCH_NAME benchmark2

#include <stdio.h>
#include "benchmark.h"
#include "socket_memory.h"
#include "util.h"

#define CONCAT(a, b) a##b
#define STRINGIFY(x) #x
#define EXPAND_AND_STRINGIFY(x) STRINGIFY(x)

// Access S3 memory: S1 read -> S2 read -> S0 read+check
// difference from benchmark1, targets specific CHA (CHA 13)

#define cha 13

void CONCAT(BENCH_NAME, _init)(void* addr_list,
                               int* primary_cores,
                               int* secondary_cores,
                               int* orchestrator_cores) {
  printf("%s: Initialization\n", EXPAND_AND_STRINGIFY(BENCH_NAME));

  void*(*address_list)[NUM_CHA][MAX_ADDRESSES] = addr_list;

  for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
    void* target = address_list[3][cha][addr];
    flush(target);
    mfence();
  }

  set_process_affinity(primary_cores[1]);
  for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
    void* target = address_list[3][cha][addr];
    maccess(target);
    mfence();
  }
  set_process_affinity(primary_cores[2]);
  for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
    void* target = address_list[3][cha][addr];
    maccess(target);
    mfence();
  }
  set_process_affinity(primary_cores[0]);
  for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
    void* target = address_list[3][cha][addr];
    mfence();
  }
}

void CONCAT(BENCH_NAME, _roi)(void* addr_list,
                              int* primary_cores,
                              int* secondary_cores,
                              int* orchestrator_cores) {
  printf("%s: Running Region of Interest\n", EXPAND_AND_STRINGIFY(BENCH_NAME));

  void*(*address_list)[NUM_CHA][MAX_ADDRESSES] = addr_list;

  // set_process_affinity(primary_cores[3]);
  for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
    void* target = address_list[3][cha][addr];
    maccess(target);
    mfence();
  }
}

void CONCAT(BENCH_NAME, _cleanup)(void* addr_list,
                                  int* primary_cores,
                                  int* secondary_cores,
                                  int* orchestrator_cores) {
  printf("%s: Cleanup\n", EXPAND_AND_STRINGIFY(BENCH_NAME));

  void*(*address_list)[NUM_CHA][MAX_ADDRESSES] = addr_list;

  for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
    void* target = address_list[3][cha][addr];
    flush(target);
    mfence();
  }
}

Benchmark benchmark = {EXPAND_AND_STRINGIFY(BENCH_NAME),
                       CONCAT(BENCH_NAME, _init), CONCAT(BENCH_NAME, _roi),
                       CONCAT(BENCH_NAME, _cleanup)};
