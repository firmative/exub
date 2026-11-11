/*
 * C99 Data Race & Race Condition Demonstration
 * Standard: ISO C99 (using <pthread.h>)
 * Compile: 
 */

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

// 1. Shared state: Non-atomic boolean
// Accessing this concurrently without synchronization is UNDEFINED BEHAVIOR.
static bool flag;

// Initialize the flag
void init_flag(void) {
    flag = false;
}

// Problem 1 (Data Race): Two threads concurrently write/write or write/read
//                      without synchronization -> Undefined Behavior.
// Problem 2 (Race Condition): Read-Modify-Write sequence (Load -> Invert -> Store)
//                             is not atomic. Concurrent toggles clobber each other!
void toggle_flag(void) {
    flag = !flag;  // UB: Data race! Also non-atomic RMW: lost updates!
}

bool get_flag(void) {
    return flag;   // UB: Concurrent read while another thread writes
}

// Thread setup parameters
#define NUM_WORKERS 4
#define TOGGLES_PER_WORKER 250000

// Worker thread routine: repeatedly toggles the shared flag
void *worker_thread(void *arg) {
    int id = *(int *)arg;
    (void)id; // Suppress unused variable warning
    for (int i = 0; i < TOGGLES_PER_WORKER; i++) {
        toggle_flag();
    }
    return NULL;
}

// Watcher thread routine: concurrently samples get_flag()
void *watcher_thread(void *arg) {
    (void)arg;
    int read_true = 0;
    int read_false = 0;
    for (int i = 0; i < 100000; i++) {
        if (get_flag()) {
            read_true++;
        } else {
            read_false++;
        }
    }
    printf("[Watcher Thread] Sampled %d 'true' and %d 'false' states.\n", read_true, read_false);
    return NULL;
}

int main(void) {
    printf("=== C99 Concurrent Data Race Demonstration ===\n");
    init_flag();

    const int total_expected_toggles = NUM_WORKERS * TOGGLES_PER_WORKER;
    printf("Initial flag: %s\n", flag ? "true" : "false");
    printf("Spawning %d worker threads, each performing %d toggles.\n",
           NUM_WORKERS, TOGGLES_PER_WORKER);
    printf("Total toggles scheduled: %d (EVEN number -> final flag should be FALSE!)\n\n",
           total_expected_toggles);

    pthread_t workers[NUM_WORKERS];
    int worker_ids[NUM_WORKERS];
    pthread_t watcher;

    // 1. Launch concurrent watcher thread (reads 'flag')
    int result = pthread_create(&watcher, NULL, watcher_thread, NULL);
    if (result != 0) {
      fprintf(stderr, "pthread_create failed: %s\n", strerror(result));
      return result;
    }

    // 2. Launch concurrent worker threads (write 'flag')
    for (int i = 0; i < NUM_WORKERS; i++) {
        worker_ids[i] = i + 1;
        result = pthread_create(&workers[i], NULL, worker_thread, &worker_ids[i]);
        if (result != 0) {
            fprintf(stderr, "Failed to create worker thread %d\n", i);
            return result;
        }
    }

    // 3. Wait for all threads to finish (join)
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(workers[i], NULL);
    }
    pthread_join(watcher, NULL);

    // 4. Inspect final outcome
    printf("\nFinal flag value: %s\n", flag ? "true" : "false");
    printf("Expected value  : false (since %d toggles is even)\n", total_expected_toggles);
    
    if (flag != false) {
        printf(">>> RACE CONDITION DETECTED! Value is TRUE due to lost updates! <<<\n");
    } else {
        printf("Note: Even if it happened to land on false by coincidence,\n"
               "ThreadSanitizer flags the UB data race immediately.\n");
    }

    return 0;
}
