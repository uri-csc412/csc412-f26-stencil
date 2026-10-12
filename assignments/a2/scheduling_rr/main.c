#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

#define LOG_FOLDER "logFolder/"
#define MAX_THREADS_LOG 4
#define LOG_FOLDER_PERMISSIONS 0755
#define LOG_FILE_PERMISSIONS   0755

typedef struct {
    int threads, q;          // number of threads to schedule, time quantum (in "units")
    int at[10], bt[10];      // arrival time & burst time (in "units")
} Input;

typedef struct {
    // immutable from worker perspective (set before start)
    int id;                    // 0..threads-1
    // shared state
    int *rem;                  // remaining burst per thread
    int *finish;               // finish time per thread (units), -1 if not finished
    int *bt;                   // original burst
    int *at;                   // arrival

    // scheduler <-> worker sync
    pthread_mutex_t *mtx;
    pthread_cond_t  *cv_turn;
    pthread_cond_t  *cv_done;

    int *turn_id;              // whose turn it is, or -1 if none
    int *grant;                // quantum granted for the current turn (units)
    int *consumed;             // set by worker: how many units actually consumed
    bool *slice_done;          // worker signals slice completion
    bool *all_started;         // barrier – scheduler flips true when all workers created
    bool *stop;                // set true by scheduler when all finished
} WorkerCtx;


static void create_log_folder(void) {
    if (mkdir(LOG_FOLDER, LOG_FOLDER_PERMISSIONS) != 0) {
        if (errno != EEXIST) {
            perror("mkdir logFolder");
            exit(1);
        }
    }
    if (chmod(LOG_FOLDER, LOG_FOLDER_PERMISSIONS) != 0) {
        perror("chmod logFolder");
        exit(1);
    }
}

// logging the thread's info
static void* log_worker(void* arg) {
    long idx = (long)arg;             // 1..MAX_THREADS_LOG
    char path[64];
    snprintf(path, sizeof(path), "%sthread%ld.txt", LOG_FOLDER, idx);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, LOG_FILE_PERMISSIONS);
    if (fd < 0) { perror("open log file"); return NULL; }

    char buf[128];
    int len = snprintf(buf, sizeof(buf),
                       "Thread %ld (pid %d) wrote this log.\n", idx, getpid());

    ssize_t written = write(fd, buf, (size_t)len);
    if (written < 0) {
        perror("Error writing ot file");
    }
    close(fd);

    if (chmod(path, LOG_FILE_PERMISSIONS) != 0) {
        perror("chmod log file");
    }
    return NULL;
}

// this is running the thread runs only when granted a quantum 
static void* rr_thread(void* arg) {
    WorkerCtx *w = (WorkerCtx*)arg;

    // wait until scheduler tells us scheduling has started
    pthread_mutex_lock(w->mtx);
    while (!*(w->all_started)) pthread_cond_wait(w->cv_turn, w->mtx);

    for (;;) {
        // wait for our turn or stop
        while (!*(w->stop) && *(w->turn_id) != w->id) {
            pthread_cond_wait(w->cv_turn, w->mtx);
        }
        if (*(w->stop)) { pthread_mutex_unlock(w->mtx); break; }
        if (w->rem[w->id] <= 0) {
            // shouldn't be scheduled, but handle gracefully
            *(w->consumed) = 0;
            *(w->turn_id) = -1;
            *(w->slice_done) = true;
            pthread_cond_signal(w->cv_done);
            continue;
        }

        int slice = *(w->grant);
        if (slice <= 0) slice = 1;
        if (slice > w->rem[w->id]) slice = w->rem[w->id];

        // drop lock while we "do work" to simulate CPU usage
        pthread_mutex_unlock(w->mtx);

        // Simulate work: one unit = 5ms; run for 'slice' units
        // (replace with real work if desired)
        for (int u = 0; u < slice; u++) {
            // tiny compute + sleep to emulate CPU time without burning 100%
            volatile unsigned long s = 0;
            for (unsigned long k = 0; k < 50000UL; k++) s += k;
            (void)s;
            usleep(5000); // ~5ms
        }

        pthread_mutex_lock(w->mtx);
        w->rem[w->id] -= slice;
        *(w->consumed) = slice;
        *(w->turn_id)  = -1;
        *(w->slice_done) = true;
        pthread_cond_signal(w->cv_done);
        // loop again, waiting for next turn or stop
    }

    return NULL;
}

int main(void) {
    Input in;

    printf("Total number of threads in the system: "); fflush(stdout);
    if (scanf("%d", &in.threads) != 1 || in.threads < 1 || in.threads > 10) {
        fprintf(stderr, "threads must be 1..10\n"); return 1;
    }
    for (int i = 0; i < in.threads; i++) {
        printf("\nEnter the Arrival and Burst time of Thread[%d]\n", i + 1);
        printf("Arrival Time: "); fflush(stdout);
        if (scanf("%d", &in.at[i]) != 1) { fprintf(stderr, "bad Arrival Time value\n"); return 1; }
        printf("Burst Time: "); fflush(stdout);
        if (scanf("%d", &in.bt[i]) != 1 || in.bt[i] <= 0) { fprintf(stderr, "bad Burst Time value\n"); return 1; }
    }
    printf("\nEnter the Time Quantum: "); fflush(stdout);
    if (scanf("%d", &in.q) != 1 || in.q <= 0) { fprintf(stderr, "bad quantum, oh darn\n"); return 1; }

    // prep logging threads
    create_log_folder();
    pthread_t log_tids[MAX_THREADS_LOG];
    for (long t = 1; t <= MAX_THREADS_LOG; t++) {
        if (pthread_create(&log_tids[t-1], NULL, log_worker, (void*)t) != 0) {
            perror("pthread_create log_worker");
            return 1;
        }
    }

    // round-robin scheduler state
    int rem[10], finish[10];
    for (int i = 0; i < in.threads; i++) { rem[i] = in.bt[i]; finish[i] = -1; }

    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  cv_turn = PTHREAD_COND_INITIALIZER;   // scheduler -> workers
    pthread_cond_t  cv_done = PTHREAD_COND_INITIALIZER;   // worker -> scheduler

    int turn_id = -1;     // whose turn it is, -1 if none
    int grant   = 0;      // quantum granted this turn
    int consumed = 0;     // set by worker on completion
    bool slice_done = false;
    bool all_started = false;
    bool stop = false;

    // build worker contexts & spawn one thread per process
    pthread_t tids[10];
    WorkerCtx ctx[10];
    for (int i = 0; i < in.threads; i++) {
        ctx[i] = (WorkerCtx){
            .id = i,
            .rem = rem,
            .finish = finish,
            .bt = in.bt,
            .at = in.at,
            .mtx = &mtx,
            .cv_turn = &cv_turn,
            .cv_done = &cv_done,
            .turn_id = &turn_id,
            .grant = &grant,
            .consumed = &consumed,
            .slice_done = &slice_done,
            .all_started = &all_started,
            .stop = &stop
        };
        if (pthread_create(&tids[i], NULL, rr_thread, &ctx[i]) != 0) {
            perror("pthread_create rr_thread");
            return 1;
        }
    }

    // start scheduling! :)
    int now = 0; 
    int finished = 0;
    pthread_mutex_lock(&mtx);
    all_started = true;
    pthread_cond_broadcast(&cv_turn); // release workers to wait for turns

    int i = 0;
    while (finished < in.threads) {
        // find next runnable (arrived and remaining > 0)
        int start_i = i;
        int picked = -1;
        do {
            if (rem[i] > 0 && in.at[i] <= now) { picked = i; break; }
            i = (i + 1) % in.threads;
        } while (i != start_i);

        if (picked == -1) {
            // no one runnable yet -> jump time to next arrival
            int next_arrival = -1;
            for (int k = 0; k < in.threads; k++) {
                if (rem[k] > 0) {
                    if (next_arrival == -1 || in.at[k] < next_arrival)
                        next_arrival = in.at[k];
                }
            }
            if (next_arrival > now) now = next_arrival;
            continue; // retry pick
        }

        // grant one quantum to picked thread
        turn_id = picked;
        grant   = (rem[picked] < in.q) ? rem[picked] : in.q;
        slice_done = false;
        pthread_cond_broadcast(&cv_turn);

        // wait for the worker to finish its slice
        while (!slice_done) pthread_cond_wait(&cv_done, &mtx);

        now += consumed;                  // advance global time
        if (rem[picked] == 0 && finish[picked] == -1) {
            finish[picked] = now;
            finished++;
        }

        // advance RR pointer to the next position
        i = (picked + 1) % in.threads;
    }

    // signal workers to stop and join
    stop = true;
    pthread_cond_broadcast(&cv_turn);
    pthread_mutex_unlock(&mtx);

    for (int t = 0; t < in.threads; t++) pthread_join(tids[t], NULL);
    for (int t = 0; t < MAX_THREADS_LOG; t++) pthread_join(log_tids[t], NULL);

    // report
    int total_wt = 0, total_tat = 0;
    printf("\n\nThread No \tBurst Time \tTurnaround Time \tWaiting Time\n");
    for (int p = 0; p < in.threads; p++) {
        int tat = finish[p] - in.at[p];
        int wt  = tat - in.bt[p];
        if (wt < 0) wt = 0;
        printf("Thread [%d] \t%d \t\t%d \t\t\t%d\n", p + 1, in.bt[p], tat, wt);
        total_wt  += wt;
        total_tat += tat;
    }
    printf("\nAverage Turnaround Time: %f", (double)total_tat / in.threads);
    printf("\nAverage Waiting Time: %f\n", (double)total_wt / in.threads);

    return 0;
}
