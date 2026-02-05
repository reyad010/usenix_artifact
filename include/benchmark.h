#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>
#include "socket_memory.h"
#include "util.h"

typedef struct {
    const char *name;
    void (*init)(void*, int*, int*, int*);    // Pass core arrays dynamically
    void (*roi)(void*, int*, int*, int*);
    void (*cleanup)(void*, int*, int*, int*);
} Benchmark;

// Extern reference to benchmarks array (populated dynamically in benchmark.c)
extern Benchmark *benchmarks[];
extern int num_benchmarks;

// Function prototypes
Benchmark* get_benchmark_by_name(const char *name);
void list_available_benchmarks();
void load_benchmarks();

#endif // BENCHMARK_H
