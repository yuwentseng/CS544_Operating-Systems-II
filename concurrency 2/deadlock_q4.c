#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <signal.h>

#include <unistd.h>

#include <pthread.h>

#include "lock.h"
#include "thread.h"

#define N_TIMES 100000

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
        usleep(1);

        /* QUIZ 4: Change from here */
        // DO NOT USE LOCK
        uint64_t old_value = counter;
        do{
            old_value = counter;
        }while(_cmpxchg((lock_t*)&counter, old_value, old_value+args->args[0])!=old_value);
        //counter += args->args[0];
        uint64_t new_value = counter;
        /* to here, if required.. */

        /* Do not change the following line */
        printf("Thread 0 changed the value from %lu to %lu\n", old_value, new_value);
    }
}

void
thread_func_1(struct thread_args *args) {
    /* Do not change the following line */
    usleep(10000);

    for (int i=0; i<N_TIMES; ++i) {
        /* Do not change the following line */
        usleep(1);

        /* QUIZ 4: Change from here */
        // DO NOT USE LOCK
        uint64_t old_value = counter;
        do{
            old_value = counter;
        }while(_cmpxchg((lock_t*)&counter, old_value, old_value+args->args[0])!=old_value);
        //counter += args->args[0];
        uint64_t new_value = counter;
        /* to here, if required.. */

        /* Do not change the following line */
        printf("Thread 1 changed the value from %lu to %lu\n", old_value, new_value);
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
    puts("\t## Welcome  to  Deadlock  Quiz  4  ##");
    puts("\t#####################################");

    puts("This program spawns two threads, and both threads");
    puts("will try to increase the value of the global counter.");
    puts("Can you get the consistent result (sum as 300,000)");
    puts("without using lock? In such a case, we can avoid deadlock!");
    puts("Please refer to the page 35-36 of the slide:");
    puts("https://os.unexploitable.systems/l/W7L2.pdf");

    puts("Press ENTER to continue:");

    char buf[512];
    fgets(buf, 512, stdin);

    pthread_t *thread_array[2];
    struct thread_args args0, args1;

    args0.args[0] = 1ul;
    args1.args[0] = 2ul;


    thread_array[0] = fork_a_thread((void*) thread_func_0, &args0);
    thread_array[1] = fork_a_thread((void*) thread_func_1, &args1);

    join_a_thread(thread_array[0]);
    join_a_thread(thread_array[1]);

    printf("Result %lu\n", counter);

    if (counter != 300000ul) {
        puts("Inconsistent result!");
        assert(0);
    }
    printf("Thread 0 failed acquiring locks %lu times\n", thread_0_fail_counter);
    printf("Thread 1 failed acquiring locks %lu times\n", thread_1_fail_counter);
    puts("Great job! No deadlock!");

}
