/*
 * C23 Data Race & Race Condition Demonstration
 * Standard: ISO C23 (using standard <threads.h>)
 * Compile: gcc -std=c23 -fsanitize=thread -O2 -g main.c -o race
 */

#include <stdio.h>
#include <stdbool.h>
#include <threads.h>

// 1. Shared state: Non-atomic boolean
// In C23, 'bool', 'true', 'false' are first-class keywords.
// Accessing this concurrently without synchronization is UNDEFINED BEHAVIOR.
static bool flag;

// Initialize the flag
void init_flag(void) {
    flag = false;
}

// Problem 1 (Data Race): Two threads concurrently write/write or write/read
//                      without synchronization -> Undefined Behavior in ISO C.
// Problem 2 (Race Condition): Read-Modify-Write sequence (Load -> Invert -> Store)
//                            is not atomic. Concurrent toggles clobber each other!
void toggle_flag(void) {
    flag = !flag;  // UB: Data race! Also non-atomic RMW: lost updates!
}

bool get_flag(void) {
    return flag;   // UB: Concurrent read while another thread writes
}

// Thread setup parameters
constexpr unsigned int NUM_WORKERS = 4;
constexpr unsigned int TOGGLES_PER_WORKER = 250000;

// Worker thread routine: repeatedly toggles the shared flag
int worker_thread(void *arg) {
    int id = *(int *)arg;
    for (unsigned int i = 0; i < TOGGLES_PER_WORKER; i++) {
        toggle_flag();
    }
    return 0;
}

// Watcher thread routine: concurrently samples get_flag()
int watcher_thread(void *arg) {
    (void)arg;
    unsigned int read_true = 0;
    unsigned int read_false = 0;
    for (unsigned int i = 0; i < 100000; i++) {
        if (get_flag()) {
            read_true++;
        } else {
            read_false++;
        }
    }
    printf("[Watcher Thread] Sampled %d 'true' and %d 'false' states.", read_true, read_false);
    return 0;
}

int main(void) {
    puts("=== C23 Concurrent Data Race Demonstration ===);
    init_flag();

    constexpr unsigned int total_expected_toggles = NUM_WORKERS * TOGGLES_PER_WORKER;
    printf("Initial flag: %s\n", flag ? "true" : "false");
    printf("Spawning %d worker threads, each performing %d toggles.\n",
           NUM_WORKERS, TOGGLES_PER_WORKER);
    printf("Total toggles scheduled: %d (EVEN number -> final flag should be FALSE!)\n\n",
           total_expected_toggles);

    thrd_t workers[NUM_WORKERS];
    unsigned int worker_ids[NUM_WORKERS];
    thrd_t watcher;

    // 1. Launch concurrent watcher thread (reads 'flag')
    int status = thrd_create(&watcher, watcher_thread, NULL);
    if (status != thrd_success) {
        fputs("Failed to create read watcher thread 1", stderr);
        return status;
    }

    // 2. Launch concurrent worker threads (write 'flag')
    for (unsigned int i = 0; i < NUM_WORKERS; i++) {
        worker_ids[i] = i + 1;
        status = thrd_create(&workers[i], worker_thread, &worker_ids[i]);
        if (status != thrd_success) {
            fprintf(stderr, "Failed to create write worker thread%d\n", i);
            return status;
        }
    }

    // 3. Wait for all threads to finish (join)
    for (unsigned int i = 0; i < NUM_WORKERS; i++) {
        thrd_join(workers[i], NULL);
    }
    thrd_join(watcher, NULL);

    // 4. Inspect final outcome
    printf("\nFinal flag value: %s\n", flag ? "true" : "false");
    printf("Expected value  : false (since %d toggles is even)\n", total_expected_toggles);
    
    if (flag != false) {
        puts(">>> RACE CONDITION DETECTED! Value is TRUE due to lost updates! <<<");
    } else {
        puts("Note: Even if it happened to land on false by coincidence,\n"
               "ThreadSanitizer flags the UB data race immediately.");
    }

    return 0;
}
