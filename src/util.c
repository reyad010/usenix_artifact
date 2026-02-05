#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>

int primary_cores[MAX_SOCKETS] = {0};  
int secondary_cores[MAX_SOCKETS] = {0};
int orchestrator_cores[MAX_SOCKETS] = {0};

uint64_t rdtsc() {
    uint64_t a, d;
    asm volatile("mfence");
    asm volatile("rdtscp": "=a"(a), "=d"(d)::"rcx");
    return (d << 32) | a;
}

// Usage: set_process_affinity(primary_cores[socket_id] or secondary_cores[socket_id]);
void set_process_affinity(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("sched_setaffinity");
        // Optionally, you can handle the error more gracefully, like exiting or logging.
    }
}

// Find two cores in each socket and store them as primary and secondary.
void find_primary_secondary_cores_per_socket() {
    memset(primary_cores, -1, sizeof(primary_cores));
    memset(secondary_cores, -1, sizeof(secondary_cores));
    memset(orchestrator_cores, -1, sizeof(orchestrator_cores));
    
    int *socket_cores[MAX_SOCKETS];
    int socket_core_count[MAX_SOCKETS] = {0};
    
    for (int i = 0; i < MAX_SOCKETS; i++) {
        socket_cores[i] = malloc(256 * sizeof(int));  // Assume max 256 cores per socket
    }

    DIR *dir = opendir("/sys/devices/system/cpu");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "cpu", 3) == 0) {
            int cpu_id;
            if (sscanf(entry->d_name + 3, "%d", &cpu_id) != 1) {
                continue;
            }

            char path[128];
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", cpu_id);
            FILE *file = fopen(path, "r");
            if (!file) {
                continue;
            }

            int socket_id;
            if (fscanf(file, "%d", &socket_id) != 1) {
                fclose(file);
                continue;
            }
            fclose(file);

            if (socket_id >= 0 && socket_id < MAX_SOCKETS) {
                if (primary_cores[socket_id] == -1) {
                    primary_cores[socket_id] = cpu_id;
                } else if (secondary_cores[socket_id] == -1) {
                    secondary_cores[socket_id] = cpu_id;
                }
                
                // Store cores for orchestrator selection temporarily
                socket_cores[socket_id][socket_core_count[socket_id]++] = cpu_id;
            }
        }
    }
    closedir(dir);

    srand(time(NULL));
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (socket_core_count[i] > 2) {
            int random_index;
            do {
                random_index = rand() % socket_core_count[i];
            } while (socket_cores[i][random_index] == primary_cores[i] || socket_cores[i][random_index] == secondary_cores[i]);
            
            orchestrator_cores[i] = socket_cores[i][random_index];
        }
    }

    // Print cores per socket as a table
    printf("+--------+---------------+----------------+-------------------+\n");
    printf("| Socket | Primary Core  | Secondary Core | Orchestrator Core |\n");
    printf("+--------+---------------+----------------+-------------------+\n");
    for (int i = 0; i < MAX_SOCKETS; i++) {
        printf("| %-6d | %-13d | %-14d | %-17d |\n", i, primary_cores[i], secondary_cores[i], orchestrator_cores[i]);
    }
    printf("+--------+---------------+----------------+-------------------+\n");

    // Free allocated memory
    for (int i = 0; i < MAX_SOCKETS; i++) {
        free(socket_cores[i]);
    }
}

void execute_on_socket_core(int socket_id, int use_secondary, void (*func)(void *), void *arg, int old_core_id) {
    if (socket_id < 0 || socket_id >= MAX_SOCKETS) {
        fprintf(stderr, "Invalid socket ID %d\n", socket_id);
        return;
    }

    int core_id = use_secondary ? secondary_cores[socket_id] : primary_cores[socket_id];
    if (core_id == -1) {
        fprintf(stderr, "No suitable core found for socket %d\n", socket_id);
        return;
    }

    // Set CPU affinity to the selected core
    set_process_affinity(core_id);

    // Execute the function in the same thread
    func(arg);

    // Restore affinity to all available cores after execution
    set_process_affinity(old_core_id);
}

// Function to display a progress bar with animation
void display_progress(const char *label, int current, int total) {
    static int prevPercent = -1;  // Avoid redundant prints
    if (total == 0) return;  // Prevent division by zero

    int percent = (current * 100) / total;
    if (percent == prevPercent) return;  // Skip redundant updates
    prevPercent = percent;

    int barWidth = 50;  // 50 segments for better visualization
    int filled = (percent * barWidth) / 100;

    // Create progress bar
    char bar[barWidth + 3]; // +3 for brackets and null terminator
    bar[0] = '[';
    for (int i = 1; i <= barWidth; i++) {
        if (i < filled) {
            bar[i] = '=';
        } else if (i == filled) {
            bar[i] = '>';
        } else {
            bar[i] = ' ';
        }
    }
    bar[barWidth + 1] = ']';
    bar[barWidth + 2] = '\0';

    // ASCII animation
    const char *animation[] = {"|", "/", "-", "\\"};
    const char *animChar = animation[current % 4];

    // Print progress
    printf("\r%s %s %3d%% %s", label, bar, percent, animChar);
    fflush(stdout);
}

