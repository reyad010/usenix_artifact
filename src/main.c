#include <numa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>	
#include <errno.h>				// errno support
#include "benchmark.h"
#include "msr_defs.h"
#include "socket_memory.h"
#include "util.h"

uint64_t new_counts[NUM_RUNS][MAX_SOCKETS][NUM_CHA][MAX_MONITOR_EVENTS] = {0};

int load_monitor_counters(char*** event_name_list, int* num_events_to_monitor);

int main(int argc, char* argv[]) {
  // print MAX_SOCKETS
  printf("MAX_SOCKETS: %d\n", MAX_SOCKETS);
  // Load all benchmarks dynamically
  load_benchmarks();

  if (argc < 2) {
    printf("Usage: %s <benchmark_name>\n", argv[0]);
    list_available_benchmarks();
    return EXIT_FAILURE;
  }

  // Get the selected benchmark
  Benchmark* benchmark = get_benchmark_by_name(argv[1]);
  if (!benchmark) {
    printf("Error: Benchmark '%s' not found!\n", argv[1]);
    list_available_benchmarks();
    return EXIT_FAILURE;
  }

  int interactive = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--interactive") == 0) {
      interactive = 1;
      break;
    }
  }

  int socket_map[MAX_SOCKETS] = {0};
  int msr_fds[MAX_SOCKETS];
  int num_sockets = find_cpu_sockets(socket_map, MAX_SOCKETS);
  if (num_sockets <= 0) {
    fprintf(stderr, "Error: Could not determine CPU sockets.\n");
    return EXIT_FAILURE;
  }

  if (open_msr_fds(socket_map, num_sockets, msr_fds) != 0) {
    fprintf(stderr, "Error: Failed to open MSR file descriptors.\n");
    return EXIT_FAILURE;
  }

  // Parse CHA events from JSON file.
  cha_event_t* events = NULL;
  int num_events = 0;
  if (parse_cha_events(JSON_FILE_PATH, &events, &num_events) != 0) {
    fprintf(stderr, "Error: Failed to parse CHA events.\n");
    return EXIT_FAILURE;
  }

  // Determine which events to program based on interactive flag.
  char** event_name_list = NULL;
  int num_total_events = 0;

  if (!interactive) {
    // Load monitoring counters from "monitor" file
    if (load_monitor_counters(&event_name_list, &num_total_events) != 0) {
      fprintf(stderr, "Error: Failed to load monitoring counters.\n");
      return EXIT_FAILURE;
    }
  } else {
    select_cha_events(events, num_events, &event_name_list, &num_total_events);
  }

  // Process events in batches of 4
  int num_batches = (num_total_events + NUM_CTR_PER_CHA - 1) / NUM_CTR_PER_CHA;
  int event_index = 0;  // Tracks the index for storing results

  disable_prefetch(msr_fds, num_sockets);

  // Memory allocation and initialization for monitoring
  find_primary_secondary_cores_per_socket();
  allocate_memory_per_socket();
  set_process_affinity(orchestrator_cores[0]);
  generate_cha_mapped_offsets(msr_fds, num_sockets, events, num_events);

  DEBUG_PRINT("Monitoring %d events in %d batches", num_total_events, num_batches);

  // Set the global values before running the benchmark
  set_global_values((void*)address_list, primary_cores, secondary_cores,
                    orchestrator_cores);

  for (int batch = 0; batch < num_batches; batch++) {
    // ------------------------------------------------------------------ 
    // Since we can only monitor 4 events per CHA at a time, we need to
    // program the counters in batches of 4. This loop will iterate over
    // all events and program them in groups of 4.
    int start_idx = batch * NUM_CTR_PER_CHA;
    int num_events_to_program = (start_idx + NUM_CTR_PER_CHA > num_total_events)
                                    ? num_total_events - start_idx
                                    : NUM_CTR_PER_CHA;

    char* event_group[NUM_CTR_PER_CHA];
    for (int i = 0; i < num_events_to_program; i++) {
      event_group[i] = event_name_list[start_idx + i];
    }

    printf("Monitoring session %d/%d: ", batch + 1, num_batches);
    for (int i = 0; i < num_events_to_program; i++) {
      printf("%s ", event_group[i]);
    }
    printf("\n");

    // ------------------------------------------------------------------
    // Set up a PMU monitoring session following documentation:
    //
    // Step (a): Freeze all uncore counters globally.
    // Step (d): Reset counters in each box (done in configure_cha_counters).
    // Step (b) & (c): Program event control registers and enable each monitor.
    // Step (f): Unfreeze counters to begin counting.
    // ------------------------------------------------------------------

    // Step (a): Freeze counters globally before configuration.
    freeze_counters_global(msr_fds, num_sockets);

    // Monitoring session: perform measurements over NUM_RUNS iterations.
    for (int run_idx = 0; run_idx < NUM_RUNS; run_idx++) {
      // Steps (d), (b), (c): Reset counters and program event control
      // registers. This call resets the counters in each CHA (by writing 0x3 to
      // unit control registers) and then programs the control registers with
      // enable (.en), event selection (.ev_sel) and umask bits for each
      // requested event. Note: Currently U_MSR_PMON_UNIT_CTL_rst_both is
      // working. if not, use delta calculation
      configure_cha_counters(msr_fds, num_sockets, events, num_events,
                             event_group, num_events_to_program);

      // Bench: Preconfigure the benchmark here
      benchmark->init((void*)address_list, primary_cores, secondary_cores,
                      orchestrator_cores);

      // Step (f): Unfreeze global counters to start counting.
      unfreeze_counters_global(msr_fds, num_sockets);

      // Bench: Run the benchmark here
      benchmark->roi((void*)address_list, primary_cores, secondary_cores,
                     orchestrator_cores);

      // Freeze counters to stop counting at the end of the monitoring interval.
      freeze_counters_global(msr_fds, num_sockets);

      // Run cleanup (if available)
      if (benchmark->cleanup) {
        benchmark->cleanup((void*)address_list, primary_cores, secondary_cores,
                           orchestrator_cores);
      }

      // Read new counter values after measurement interval.
      read_cha_counters(msr_fds, num_sockets, events, num_events, event_group,
                        num_events_to_program, run_idx, new_counts,
                        event_index);
    }

    event_index += num_events_to_program;  // Move index forward
  }

  // Write event counts to output file.
  write_event_counts(new_counts, num_total_events, num_sockets, event_name_list,
                     benchmark->name);

  // Cleanup resources.
  free_cha_events(events, num_events);
  close_msr_fds(msr_fds, num_sockets);

  // Free dynamically allocated event names in interactive mode.
  if (interactive) {
    for (int i = 0; i < num_total_events; i++) {
      free(event_name_list[i]);
    }
    free(event_name_list);
  }

  return EXIT_SUCCESS;
}

// Function to load monitoring counters from a file
int load_monitor_counters(char*** event_name_list, int* num_events_to_monitor) {
  FILE* file = fopen("monitor", "r");
  if (!file) {
    perror("Failed to open monitor file");
    return -1;
  }

  char** events = malloc(MAX_MONITOR_EVENTS * sizeof(char*));
  char line[128];
  int count = 0;

  while (fgets(line, sizeof(line), file) && count < MAX_MONITOR_EVENTS) {
    line[strcspn(line, "\n")] = '\0';  // Remove newline
    events[count] = strdup(line);
    count++;
  }

  fclose(file);
  *event_name_list = events;
  *num_events_to_monitor = count;
  return 0;
}