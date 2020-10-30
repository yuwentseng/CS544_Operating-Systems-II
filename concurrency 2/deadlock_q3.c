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
lock_t l0, l1;

// A global counter
uint64_t counter = 0ul;

uint64_t thread_0_fail_counter = 0ul;
uint64_t thread_1_fail_counter = 0ul;

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
        usleep(10000);

        /* QUIZ 3: Change from here */
        // DO NOT CHANGE the lock acquiring order.
        // You may use _xchg as a 'trylock' function.
        // i.e., _xchg(&lock, 1) == 0 then the thread has acquired the lock!
top:
        xchg_lock(&l0);
        printf("Thread 0 acquired l0\n");
        if(_xchg(&l1,1)==1){
            xchg_unlock(&l0);
            goto top;
        }
        printf("Thread 0 acquired l1\n");
        /* to here, if required */

        /* Do not change the following line */
        counter += args->args[0];

        /* Change from here */
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
        usleep(10000);

        /* QUIZ 3: Change from here */
        // DO NOT CHANGE the lock acquiring order.
        // You may use _xchg as a 'trylock' function.
        // i.e., _xchg(&lock, 1) == 0 then the thread has acquired the lock!
top:
        xchg_lock(&l1);
        printf("Thread 1 acquired l1\n");
        if(_xchg(&l0,1)==1){
            xchg_unlock(&l1);
            goto top;
        }
        printf("Thread 1 acquired l0\n");
        /* to here, if required */

        /* Do not change the following line */
        counter += args->args[0];

        /* Change from here */
        xchg_unlock(&l0);
        printf("Thread 1 released l0\n");
        xchg_unlock(&l1);
        printf("Thread 1 released l1\n");
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
    puts("\t## Welcome  to  Deadlock  Quiz  3  ##");
    puts("\t#####################################");

    puts("This program spawns two threads, and both threads");
    puts("will try acquire two locks competitively.");
    puts("However, a novice programmer (possibly Prof. Jang)");
    puts("wrote a poor code, so the program encounters 'deadlock'.");
    puts("Can you resolve the deadlock in the program by");
    puts("removing no-preemption?");
    puts("In this case, you cannot change the order of lock,");
    puts("i.e., Thread 0 acquires l0 and then l1,");
    puts("Thread 1 acquires l1 and then l0,");
    puts("and you should implement trylock/unlock,");
    puts("which you can see at the page 42 of the slide,");
    puts("https://os.unexploitable.systems/l/W7L2.pdf");


    // set signal handler for SIGALRM.
    // i.e., kill the process if the execution does not finish within 10 sec.
    struct sigaction action;
    action.sa_handler = sigalrm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGALRM, &action, NULL);
    alarm(10);

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
    printf("Thread 0 failed acquiring locks %lu times\n", thread_0_fail_counter);
    printf("Thread 1 failed acquiring locks %lu times\n", thread_1_fail_counter);
    puts("Great job! No deadlock!");

}
