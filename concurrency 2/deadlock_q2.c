#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <signal.h>

#include <unistd.h>

#include <pthread.h>

#include "lock.h"
#include "thread.h"

#define N_TIMES 100

// Create two locks
lock_t l0, l1, meta;

// A global counter
uint64_t counter = 0ul;


/*
 *
 *  NOTE: PLEASE DO NOT EDIT lines that run usleep().
 *  i.e., do not change the number, order, etc.
 *
 */

void
thread_func_0(struct thread_args *args) {
    /* Do not change the following line */
    usleep(10000);

    for (int i=0; i<N_TIMES; ++i) {
        /* Do not change the following line */
        usleep(10);

        /* QUIZ 2: Change from here */
        // DO NOT CHANGE the lock acquiring order.
        xchg_lock(&meta);
        xchg_lock(&l0);
        printf("Thread 0 acquired l0\n");
        xchg_lock(&l1);
        printf("Thread 0 acquired l1\n");
        /* to here, if required */

        /* Do not change the following line */
        counter += args->args[0];

        /* Change from here */
        xchg_unlock(&meta);
        xchg_unlock(&l1);
        printf("Thread 0 released l1\n");
        xchg_unlock(&l0);
        printf("Thread 0 released l0\n");
        /* to here, if required.. */
    }
}

void
thread_func_1(struct thread_args *args) {
    /* Do not change the following line */
    usleep(10000);

    for (int i=0; i<N_TIMES; ++i) {
        /* Do not change the following line */
        usleep(10);

        /* QUIZ 2: Change from here */
        // DO NOT CHANGE the lock acquiring order.
        xchg_lock(&meta);
        xchg_lock(&l0);
        printf("Thread 1 acquired l0\n");
        xchg_lock(&l1);
        printf("Thread 1 acquired l1\n");
        /* to here, if required */

        /* Do not change the following line */
        counter += args->args[0];

        /* Change from here */
        xchg_unlock(&meta);
        xchg_unlock(&l1);
        printf("Thread 1 released l1\n");
        xchg_unlock(&l0);
        printf("Thread 1 released l0\n");
        /* to here, if required */
    }
}


/* DO NOT CHANGE THE FOLLOWING CODE */

void
sigalrm_handler (int signum) {
    printf("DEADLOCK, no points!\n");
    exit(-1);
}

int
main() {
    puts("\t#####################################");
    puts("\t## Welcome  to  Deadlock  Quiz  2  ##");
    puts("\t#####################################");

    puts("This program spawns two threads, and both threads");
    puts("will try acquire two locks competitively.");
    puts("However, a novice programmer (possibly Prof. Jang)");
    puts("wrote a poor code, so the program encounters 'deadlock'.");
    puts("Can you resolve the deadlock in the program by");
    puts("creating a meta lock to make acquiring all locks");
    puts("run atomically?");


    // set signal handler for SIGALRM.
    // i.e., kill the process if the execution does not finish within 5 sec.
    struct sigaction action;
    action.sa_handler = sigalrm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGALRM, &action, NULL);
    alarm(5);

    pthread_t *thread_array[2];
    struct thread_args args0, args1;

    args0.args[0] = 1ul;
    args1.args[0] = 2ul;


    thread_array[0] = fork_a_thread((void*) thread_func_0, &args0);
    thread_array[1] = fork_a_thread((void*) thread_func_1, &args1);

    join_a_thread(thread_array[0]);
    join_a_thread(thread_array[1]);

    printf("Result %lu\n", counter);

    if (counter != 300ul) {
        puts("Inconsistent result!");
        assert(0);
    }

    puts("Great job! No deadlock!");

}
