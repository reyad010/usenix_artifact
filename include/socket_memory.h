#ifndef SOCKET_MEMORY_H
#define SOCKET_MEMORY_H

#include <stdlib.h>
#include <stdio.h>
#include <numa.h>
#include <numaif.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "msr_defs.h"
#include "util.h"

#define MATCH_THRESHOLD 10  // Number of offsets to compare for reuse

#define BUFFER_SIZE (1L * 1024 * 1024 * 1024) // 1GB per socket
// #define PAGE_SIZE (2 * 1024 * 1024)  // 2MB HugePage size
#define ALIGNMENT (2L * 1024 * 1024) // 2MB alignment

extern void* address_list[MAX_SOCKETS][NUM_CHA][MAX_ADDRESSES];
extern uint8_t *socket_buffers[MAX_SOCKETS];

void allocate_memory_per_socket();
void free_memory_per_socket();
void *get_socket_buffer(int socket_id);
void access_flush_socket_memory_one(int socket_id);
void access_socket_memory_hitmealloc(int socket_id);
int load_stored_offsets(int stored_offsets[NUM_CHA][MAX_ADDRESSES], int* valid_entries);
int find_cha_mapped_offset(void* address, int* msr_fds, int num_sockets, cha_event_t* events, int num_events);
void generate_cha_mapped_offsets(int* msr_fds, int num_sockets, cha_event_t* events, int num_events);

#endif // SOCKET_MEMORY_H
