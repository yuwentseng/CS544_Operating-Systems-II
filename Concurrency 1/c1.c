#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <unistd.h>
#include <pthread.h>

#include "lock.h"

/*
 * CS 444/544 Concurrency Lab 1: Implementing Locks
 *
 * In this lab, we will learn how we can synchronize multiple threads
 * by implementing multiple lock primitives.
 *
 * Starting from bad_lock, which does not guarantee any consistency,
 * you will be asked to implement:
 * bad_lock() : a lock that does not use any of hardware atomic operation.
 * xchg_lock(): a lock that uses xchg (atomic swap) as an atomic operation.
 * cmpxchg_lock(): a lock that uses cmpxchg (compare and swap) as an
 *                 atomic operation.
 * tts_xchg_lock(): implement test and swap by using xchg.
 * backoff_cmpxchg_lock(): insert exp. backoff to tts_cmpxchg_lock()
 *
 */


/*
 * On implementing the following functions, you may regard lock_t as
 * an unsigned integer (uint32_t), and we will use the following semantics
 * to our lock variable:
 *      *lock == 1:
 *          The lock is holden by a thread, so you may not acquire the lock.
 *      *lock == 0:
 *          The lock is free. You may acquire the lock (set the lock to 1)
 *
 *  So 'lock' means to set lock to 1 (*lock = 1) if the value of lock was 0.
 *  If the value of lock is 1, you need to wait until the value changes to 0.
 *  This is so-called as acquiring lock.
 *
 *  Inversely, 'unlock' means to set lock to 0, and doing this will allow
 *  other threads to see the value 0, and we hope the first thread that
 *  sees the value 0 will acquire the lock (set lock to 1).
 *
 */

/*
 * Let's start with implementing a very naive function, bad_lock/unlock.
 *
 * bad_lock(lock_t *lock); software test-and-set
 * bad_unlock(lock_t *lock)
 *
 * Your task is to implement a 'bad' lock that does not use any of
 * atomic operations provided by CPU. The reason for doing this is
 * to see the implementation without using hardware atomic operation
 * will never guarantee the consistency of jointly updating values
 * by multiple threads.
 *
 * After implementing this, please run 'make' to build your code
 * and run './test-bad'.
 * The result when all threads were correctly synchronized is 2499997500000,
 * and you will not see the result because this is a bad lock.
 *
 */

/* Please implement the following functions */
void
bad_lock(lock_t *lock) {    // software test-and-set
    // 1. wait until the value of lock becomes 0 (test)
    // Your code here:

    // 2. set the value of lock to 1 (set)
    // Your code here:
    if(*lock==1){
        *lock = 1;
    }
}

void
bad_unlock(lock_t *lock) {
    // 1. set the value of lock to 0
    // Your code here:
    *lock = 0;
}


/*
 * By implementing bad_lock, we have seen that we can't implement a good lock
 * because softwaretest-and-set will never be atomic.
 *
 * Then, let's use an atomic test-and-set instruction, 'xchg',
 * provided by Intel CPU.
 *
 * You may take a look at the description of the instruction at here:
 * https://c9x.me/x86/html/file_module_x86_id_328.html
 *
 * The description is written in Intel syntax, but we will write
 * inline assembly in AT&T/GNU syntax as we will use GNU compiler (g++)
 * to build our code.
 *
 * In AT&T/GNU syntax, the instruction could be used as:
 *
 *      "xchg [memory], [register];"
 *
 * Then, it will swap the value in register to the value in the memory.
 * You can think it as:
 *
 *      temp = *memory;
 *      *memory = register;
 *      register = temp;
 *
 *  And this swap runs atomically, which means no other threads can
 *  intervene during its execution.
 *
 *  In the following, please first implement xchg(), the function that
 *  runs atomic swap, and then implement xchg_lock() and xchg_unlock()
 *  using the xchg() function.
 *
 *
 * After implementing those functions, please run 'make' to build
 * your code, and then run './test-xchg' to see the result.
 *
 * A correct implementation must yield the result 2499997500000
 * (otherwise your implementation is incorrect), and it may take
 * few seconds.
 *
 * Please also run the following command to see how many L1-dcache-load-misses
 * does this implementation generates:
 *  ./perf-l1-cache-monitor.sh ./test-xchg
 *
 *
 * Finally, please compare the result with the implementation of
 * pthread_mutex:
 *  ./test-mutex
 *
 *
 * Seems your implementation is slow, but it's OK. We will have a
 * better implementation at the end.
 *
 */


/*
 * Requirement:
 *      Use the 'xchg' instruction to atomically swap the xchg_value and
 *      the value stored in the lock variable.
 */
static uint32_t
xchg(lock_t *lock, uint32_t xchg_value) {
    uint32_t result;

    // 1. write an inline assembly line to use the 'xchg' instruction to
    // exchange *lock with xchg_value.
    asm volatile("lock; xchgl %0, %1" // your assembly here..
            : "+m"(*lock),"=a"(result)// 2. put lock and result..
            : "1"(xchg_value)// 3. put exchange value here..
            );

    return result;
}


/*
 * Requirement:
 *      Use the xchg() to implement the following.
 */
void
xchg_lock(lock_t *lock) {

    // 1. wait until xchg(lock, 1) returns 0.
    // Your code here:
    while(xchg(lock,1)==1){
        while(*lock==1)
            break;
    }

    // After finishing 1., the lock value will be 1,
    // so your thread acquired the lock!
}

/*
 * Requirement:
 *      Use the xchg() to implement the follwoing.
 */
void
xchg_unlock(lock_t *lock) {
    // 1. use xchg() to set the lock value to 0.
    // Your code here:
    xchg(lock,0);
}


/*
 * To avoid cache contention, Intel processor also provides 'cmpxchg',
 * which is an atomic test and test-and-set instruction.
 * You may take a look at the usage of the instruction at here:
 * https://www.felixcloutier.com/x86/cmpxchg
 *
 * The AT&T/GNU syntax of the usage of the instruction is:
 *      "lock cmpxchg [register_new_value], [memory];"
 *
 * The register operand will store the new value to update the value in memory,
 * and before the update, the eax register will be compared to
 * the value in memory.
 *
 * If the value in the eax register matches to the value in memory,
 * the instruction will update the value in memory to the value of the
 * operand register (not eax, the other one, specified as [register_new_value]).
 * Otherwise (if eax != *memory), the value in memory will be stored into
 * the eax register.
 *
 * The pseudocode of the instruction is as follows:
 *      if (eax == *memory) {
 *          *memory = register_new_value;
 *      }
 *      else {
 *          eax = *memory;
 *      }
 *
 * So setting eax = 0 and register_new_value = 1 will compare and exchange
 * the value in the lock variable as we expects.
 *
 * In the following, please implement cmpxchg(), and also implement
 * cmpxchg_lock by using cmpxchg().
 *
 * After implementing those functions, please run 'make' to build
 * your code, and then run './test-cmpxchg' to see the result.
 *
 * A correct implementation must yield the result 2499997500000
 * (otherwise your implementation is incorrect), and it may take
 * few seconds.
 *
 * Please also run the following command to see how many L1-dcache-load-misses
 * does this implementation generates:
 *  ./perf-l1-cache-monitor.sh ./test-cmpxchg

 */

/*
 * Requirement:
 *      Use the 'lock cmpxchg' instruction to atomically swap the xchg_value and
 *      the value stored in the lock variable if the value matches to
 *      the value of 'compare' variable.
 */
static uint32_t
cmpxchg(lock_t *lock, uint32_t compare, uint32_t xchg_value) {
    uint32_t result;

    // Your code here:
    asm volatile("lock;cmpxchgl %2,%1"
            ";"      // setup eax to store compare
            ";"      // lock cmpxchg [xchg_value] [*lock]
            :"=a"(result), "+m"(*lock)        // outputs
            :"r"(xchg_value), "0"(compare)        // inputs
            : "cc"   // the cmpxchg may change eflags.. (DO NOT CHANGE).
            );

    return result;
}

/*
 * Requirement:
 *      Use the xchg() to implement the following.
 */
void
cmpxchg_lock(lock_t *lock) {
    // 1. Wait until cmpxchg(lock, 0, 1) returns 0...
    // Your code here:
    while(cmpxchg(lock, 0, 1)==1){
        while(*lock==1){
            break;
        }
    }

    // After this, you acquired the lock because now the value in the lock
    // is 1 but it was 0.
}

/*
 * Seems using cmpxchg still slow.
 * Let's try a different approach. We may implement test and test-and-set
 * by combining 'software test' and 'hardware test-and-set'.
 * For software test, we may check if (*lock == 0), and only if it is,
 * we may run the 'hardware test-and-set', which is 'xchg',
 * to set the lock variable to 1.
 *
 * However, because such a combination of 'software test' and
 * 'hardware test-and-set' is not atomic (e.g., after running the
 * 'software test', another thread could intervene before running
 * 'hardware test-and-set', so passing if (*lock == 0) cannot guaranatee
 * 100% for the free of lock.
 *
 * So to implement this, after passing if (*lock == 0), you may also
 * want to check if your xchg() have acquired the lock (e.g., checking against
 * value 0). If it is, you may regard that your thread have acquired the lock.
 *
 * Please implement the following function:
 *
 */
void
tts_xchg_lock(lock_t *lock) {
    // Algorithm:
    // 1. Wait until *lock == 0
    //      1-1. if it is, try xchg(lock, 1).
    //      1-2. if it returns 0, you acquired the lock.
    //      1-3. otherwise, no, you did not acquire the lock. Go back to 1.
    // 2. otherwise, go back to 1.
    // Your code here:
    while(1){
        if(*lock==0&&xchg(lock, 1)==0){
            break;
        }
    }
}
/*
 * After implementing tts_xchg_lock, please check how it performs,
 * by running the program (./test-tts-xchg) and also running it with
 * perf-l1-cache-monitor.sh.
 *
 * The result must be consisent and also must be faster and having
 * less cache misses than former two implementations
 * (otherwise, your implementation is incorrect!).
 *
 * But, it still slower than pthread_mutex (./test-mutex).
 * How can we enhance this?
 *
 */




/*
 * Let's try another approach.
 *
 * The major performance bottleneck of cmpxchg is at the point that
 * many threads would like to check if *lock == 0 while
 * only one thread can hold the lock.
 *
 * Those who fail wait till the resource becomes available and then retry.
 * But if everyone were to retry at the same time, quite possibly
 * none of them will succeed. In such a case, what will be the best way
 * to resolve such a contention?
 *
 * The answer is to apply exponential backoff for those who waits for
 * the lock. Theoretically, this will gradually release the resource
 * contention by having exponential slots for waiting, and finally
 * threads will have no contention for waiting for the shared resource.
 *
 * You may read more about the exponential backoff from here:
 * https://en.wikipedia.org/wiki/Exponential_backoff
 *
 * and the following article explains how we can apply this exponential backoff
 * in implementing locks.
 * https://geidav.wordpress.com/tag/exponential-back-off/
 *
 *
 * So your task here is to apply the exponential backoff to our cmpxchg lock.
 * Please fill the condition for the while loop to finish to implement
 * cmpxchg lock with the exponential backoff.
 *
 */
void
backoff_cmpxchg_lock(lock_t *lock) {
    // start with backoff value 1.
    uint32_t backoff = 1;
    // Your code:
    // please uncomment the following and
    // implement the condition for the while loop...

    while(xchg(lock, 1)==1){
        for(int i=0; i<backoff; ++i) {
            asm volatile("pause");
        }
        // shift left by 1 means multiply backoff by 2 (exponential)!
        // 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, ...
        backoff <<= 1;
    }

}


/*
 * THIS IS THE END OF CONCURRENCY LAB 1.
 * Can you measure the performance (time to finish and cache misses) of
 * your final implementation (./test-backoff-cmpxchg)?
 *
 * You may run 'make test-all' to run all the test program with perf.
 *
 * Is your implementation better than pthread_mutex?
 * And please also fill the answers to 'answers-c1.txt',
 * and please commit and push your work by tagging the final commit as
 * 'c1-final'.
 *
 */

// Followings uses pthread_mutex.
// These will be compared to your implementation regarding lock perofrmance.
void
mutex_lock(lock_t *lock) {
    pthread_mutex_t *mutex_lock = (pthread_mutex_t *)lock;
    pthread_mutex_lock(mutex_lock);
}

void
mutex_unlock(lock_t *lock) {
    pthread_mutex_t *mutex_lock = (pthread_mutex_t *)lock;
    pthread_mutex_unlock(mutex_lock);
}
