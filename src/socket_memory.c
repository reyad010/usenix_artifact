#include <inttypes.h>
#include "socket_memory.h"

void* address_list[MAX_SOCKETS][NUM_CHA][MAX_ADDRESSES] = {{{NULL}}};
uint8_t *socket_buffers[MAX_SOCKETS] = {NULL};

void allocate_memory_per_socket() {
    if (numa_available() < 0) {
        fprintf(stderr, "NUMA is not available on this system.\n");
        return;
    }

    int max_nodes = numa_max_node() + 1;
    if (max_nodes > MAX_SOCKETS) {
        max_nodes = MAX_SOCKETS;
    }

    int total_pages = (BUFFER_SIZE / PAGE_SIZE) * max_nodes; // Total pages across all sockets
    int processed_pages = 0;

    for (int socket_id = 0; socket_id < max_nodes; socket_id++) {
        void *raw_mem = numa_alloc_onnode(BUFFER_SIZE + ALIGNMENT, socket_id);
        if (!raw_mem) {
            fprintf(stderr, "Memory allocation failed on socket %d\n", socket_id);
            continue;
        }

        // Ensure 4MB alignment
        uintptr_t aligned_addr = ((uintptr_t)raw_mem + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
        uint8_t *buffer = (uint8_t *)aligned_addr;

        // Access every 4KB of allocated memory and show progress
        for (size_t i = 0; i < BUFFER_SIZE; i += PAGE_SIZE) {
            buffer[i] = 0;  // Enforce allocation
            
            processed_pages++;
            display_progress("Memory Allocation:", processed_pages, total_pages);
        }

        socket_buffers[socket_id] = buffer;
    }

    // Ensure progress bar reaches 100% at the end
    display_progress("Memory Allocation:", total_pages, total_pages);
    printf("\n");
}


void free_memory_per_socket() {
    for (int socket_id = 0; socket_id < MAX_SOCKETS; socket_id++) {
        if (socket_buffers[socket_id]) {
            numa_free(socket_buffers[socket_id], BUFFER_SIZE);
            socket_buffers[socket_id] = NULL;
        }
    }
}

void *get_socket_buffer(int socket_id) {
    if (socket_id < 0 || socket_id >= MAX_SOCKETS) {
        fprintf(stderr, "Invalid socket ID: %d\n", socket_id);
        return NULL;
    }
    return socket_buffers[socket_id];
}

void access_flush_socket_memory_one(int socket_id) {
    void *buffer = get_socket_buffer(socket_id);
    if (!buffer) {
        fprintf(stderr, "Error: No allocated buffer for socket %d\n", socket_id);
        return;
    }

    void *target = buffer; // Targeting the first byte of the buffer

    for (int i = 0; i < 2000; i++) {
        maccess(target);  // Access the memory
        mfence();         // Memory fence
        flush(target);    // Flush from cache
        mfence();         // Memory fence
    }
}

void access_socket_memory_hitmealloc(int socket_id) {
    void *buffer = get_socket_buffer(socket_id);
    int p,q;
    if (!buffer) {
        fprintf(stderr, "Error: No allocated buffer for socket %d\n", socket_id);
        return;
    }
    if (socket_id == 0) {
        p = 1;
        q = 2;
    }
    else if (socket_id == 1) {
        p = 2;
        q = 3;
    }
    else if (socket_id == 2) {
        p = 3;
        q = 0;
    } else {
        p = 0;
        q = 1;
    }
    if (!buffer) {
        fprintf(stderr, "Error: No allocated buffer for socket %d\n", socket_id);
        return;
    }


    for (int cha = 0; cha < NUM_CHA; cha++) {
        for (int addr = 0; addr < MAX_ADDRESSES; addr++) {
            void* target = address_list[0][cha][addr];
            set_process_affinity(primary_cores[p]);
            maccess(target);  // Access the memory
            mfence();         // Memory fence
            set_process_affinity(primary_cores[q]);
            maccess(target);  // Access the memory
            mfence();         // Memory fence
        }
    }
}

// Function to determine which CHA an address belongs to across all sockets
int find_cha_mapped_offset(void* address, int* msr_fds, int num_sockets, cha_event_t* events, int num_events) {
    uint64_t new_counts[NUM_RUNS][MAX_SOCKETS][NUM_CHA][MAX_MONITOR_EVENTS] = {0};

    char *default_events[] = {"UNC_CHA_LLC_LOOKUP.DATA_READ_DDT"};
    char **event_name_list = default_events;
    int num_events_to_program = 1;

    // Freeze counters globally before configuration.
    freeze_counters_global(msr_fds, num_sockets);

    for (int run_idx = 0; run_idx < NUM_RUNS; run_idx++) {
        // Reset and program counters
        configure_cha_counters(msr_fds, num_sockets, events, num_events, event_name_list, num_events_to_program);
        unfreeze_counters_global(msr_fds, num_sockets);

        // Access and flush the block 2000 times
        for (int i = 0; i < 20; i++) {
            maccess(address);
            mfence();
            flush(address);
            mfence();
        }

        freeze_counters_global(msr_fds, num_sockets);
        read_cha_counters(msr_fds, num_sockets, events, num_events, event_name_list, num_events_to_program, run_idx, new_counts, 0);
        break;
    }

    // Determine CHA mapping based on counter values across all sockets
    int best_cha = -1;
    uint64_t max_value = 0;

    for (int socket = 0; socket < num_sockets; socket++) {
        for (int cha_id = 0; cha_id < NUM_CHA; cha_id++) {
            if (new_counts[0][socket][cha_id][0] > max_value) {
                max_value = new_counts[0][socket][cha_id][0];
                best_cha = cha_id;
            }
        }
    }

    return (max_value >= 18) ? best_cha : -1;  // Return the CHA ID if valid, else -1
}

void print_binary(uintptr_t value) {
    for (int i = 47; i >= 0; i--) { // Assume 48-bit addresses (typical for x86_64 virtual memory)
        if (i == 15 || i == 5) {
            printf("|"); // Separator for bits [16-6]
        }
        printf("%d", (value >> i) & 1);
    }
    printf("\n");
}

void generate_cha_mapped_offsets(int* msr_fds, int num_sockets, cha_event_t* events, int num_events) {
    FILE *log_file = fopen(OFFSET_FILE, "w");
    if (!log_file) {
        perror("Error opening log file");
        return;
    }

    for (int socket_id = 0; socket_id < num_sockets; socket_id++) {
        // if (socket_id < 0) continue;
        void* buffer = get_socket_buffer(socket_id);
        if (!buffer) {
            fprintf(stderr, "Error: No allocated buffer for socket %d\n", socket_id);
            continue;
        }

        fflush(stdout);

        int cha_offset_list[NUM_CHA][MAX_ADDRESSES] = {0};  // Store offsets for each CHA
        int cha_count[NUM_CHA] = {0};  // Keep track of found offsets per CHA

        for (int offset = 0; offset < BUFFER_SIZE;) {
            if (offset + 64 > BUFFER_SIZE) break;

            void* target = (char*)buffer + offset;
            offset += 64;


            // // Perform bitwise operation to ensure bits [16-6] are set to 0b00000011011
            // uintptr_t target_addr = (uintptr_t)target;
            // target_addr &= ~(((uintptr_t)0x3FF) << 6); // Clear bits [16-6]
            // target_addr |= ((uintptr_t)TARGET_SET_MASK) << 6;     // Set bits [16-6] to 0b00000011011
            // target = (void*)target_addr; // Assign modified address back to target
            // // Calculate the offset for next iteration
            // offset = (uintptr_t)target - (uintptr_t)buffer + 65536;
            // if (offset >= BUFFER_SIZE) break;
            
            // // Print target address in binary and highlight bits [16-6]
            // printf("Modified target address: 0x%" PRIxPTR "\n", target_addr);
            // printf("Binary representation: ");
            // print_binary(target_addr);
            
            int cha_id = find_cha_mapped_offset(target, msr_fds, num_sockets, events, num_events);

            if (cha_id != -1 && cha_count[cha_id] < MAX_ADDRESSES) {
                // Check if this cha already has max_addresses
                // if (cha_count[cha_id] >= MAX_ADDRESSES) {
                //     continue;
                // }
                cha_offset_list[cha_id][cha_count[cha_id]] = offset;
                address_list[socket_id][cha_id][cha_count[cha_id]] = target;  // Store actual address
                cha_count[cha_id]++;

                // Check if all CHA mappings are filled
                int all_filled = 1;
                for (int i = 0; i < NUM_CHA; i++) {
                    if (cha_count[i] < MAX_ADDRESSES) {
                        all_filled = 0;
                        break;
                    }
                }

                // If all CHA mappings are found, write to the file and update global list
                if (all_filled) {
                    for (int i = 0; i < NUM_CHA; i++) {
                        fprintf(log_file, "CHA %d on Socket %d:\n", i, socket_id);
                        for (int j = 0; j < MAX_ADDRESSES; j++) {
                            fprintf(log_file, "Offset: %d\n", cha_offset_list[i][j]);
                        }
                        fflush(log_file);
                    }
                    break;  // Move to the next socket
                }

                // Update progress bar
                int processed_addresses = 0;
                for (int i = 0; i < NUM_CHA; i++) processed_addresses += cha_count[i];
                int total_addresses = MAX_ADDRESSES * NUM_CHA;
                display_progress("Find CHA Mapping: ", processed_addresses, total_addresses);
                fflush(stdout);
            }
        }

        // Ensure progress bar reaches 100% for each socket
        display_progress("Find CHA Mapping: ", MAX_ADDRESSES * NUM_CHA, MAX_ADDRESSES * NUM_CHA);
        printf("\n");
        fflush(stdout);
    }

    // fclose(log_file);
    printf("\nCHA mapping completed. Results saved in %s\n", OFFSET_FILE);
    fflush(stdout);
}

void access_flush_addresses(void* address_list_1d[], int num_addresses) {
    for (int i = 0; i < num_addresses; i++) {
        if (address_list_1d[i] == NULL) {
            continue;
        }

        void* target = address_list_1d[i];

        for (int j = 0; j < 1000; j++) {
            maccess(target);  // Access the memory
            mfence();         // Ensure ordering
            flush(target);    // Flush from cache
            mfence();         // Ensure completion
        }
    }
}
