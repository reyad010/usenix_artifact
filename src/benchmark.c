#include "benchmark.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>

#define BENCHMARK_DIR "bin/"
#define MAX_BENCHMARKS 100
#define MAX_PATH_LEN 512

Benchmark *benchmarks[MAX_BENCHMARKS];
int num_benchmarks = 0;
void* global_address_list = NULL;
int* global_primary_cores = NULL;
int* global_secondary_cores = NULL;
int* global_orchestrator_cores = NULL;

// Set the global values before running benchmarks
void set_global_values(void* address_list, int* primary, int* secondary, int* orchestrator) {
    global_address_list = address_list;
    global_primary_cores = primary;
    global_secondary_cores = secondary;
    global_orchestrator_cores = orchestrator;
}

// Check if a benchmark with the same name already exists
int is_duplicate(const char *name) {
    for (int i = 0; i < num_benchmarks; i++) {
        if (strcmp(benchmarks[i]->name, name) == 0) {
            return 1;  // Duplicate found
        }
    }
    return 0;
}

// Function to dynamically load benchmarks from shared object files (*.so)
void load_benchmarks() {
    DIR *dir = opendir(BENCHMARK_DIR);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strstr(entry->d_name, ".so")) continue;

        char so_path[MAX_PATH_LEN];
        snprintf(so_path, sizeof(so_path), "%s%s", BENCHMARK_DIR, entry->d_name);

        void *handle = dlopen(so_path, RTLD_NOW);
        if (!handle) {
            fprintf(stderr, "Failed to load %s: %s\n", so_path, dlerror());
            continue;
        }

        Benchmark *benchmark = (Benchmark *) dlsym(handle, "benchmark");
        if (!benchmark) {
            fprintf(stderr, "Failed to find 'benchmark' symbol in %s\n", so_path);
            dlclose(handle);
            continue;
        }

        if (is_duplicate(benchmark->name)) {
            fprintf(stderr, "Error: Duplicate benchmark name '%s' found in %s\n", benchmark->name, so_path);
            dlclose(handle);
            continue;
        }

        benchmarks[num_benchmarks++] = benchmark;

        if (num_benchmarks >= MAX_BENCHMARKS) break;
    }

    closedir(dir);
}

// Get a benchmark by name
Benchmark* get_benchmark_by_name(const char *name) {
    for (int i = 0; i < num_benchmarks; i++) {
        if (strcmp(benchmarks[i]->name, name) == 0) {
            return benchmarks[i];
        }
    }
    return NULL;
}

// List all available benchmarks
void list_available_benchmarks() {
    printf("Available Benchmarks:\n");
    for (int i = 0; i < num_benchmarks; i++) {
        printf("  - %s\n", benchmarks[i]->name);
    }
}
