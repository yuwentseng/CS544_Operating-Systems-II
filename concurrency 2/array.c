#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <signal.h>

#include <unistd.h>

#include <pthread.h>

#include "lock.h"
#include "thread.h"

#define N_TIMES 1000000


struct Array {
    int64_t size;       // size
    uint64_t *buffer;   // buffer
    lock_t lock;        // a lock variable for this array

    Array() {
        // allocate!
        lock = 0;       // 0 for lock
        size = 0;       // start with 0 size
        buffer = new uint64_t[N_TIMES]; // buffer
    }

    ~Array() {
        delete[] buffer;
    }

    void clear() {
        size = 0;
    }

    void insert(uint64_t item) {
        // TODO: make this thread-safe..
        // HINT: xchg_lock(&this->lock);
        xchg_lock(&this->lock);
        buffer[size] = item;
        size += 1;
        xchg_unlock(&this->lock);
    }

    uint64_t remove_last() {
        // TODO: make this thread-safe
        xchg_lock(&this->lock);
        size -= 1;
        assert(size >= 0);
        xchg_unlock(&this->lock);
        return buffer[size + 1];
    }

    uint64_t get(int64_t idx) {
        assert(idx < size);
        return buffer[idx];
    }

    void cut_in_half(Array *half) {
        // TODO: make this thread-safe.
        // XXX: please avoid deadlock
        // HINT: create an insert function that does not use lock...
        half->clear();
        
        xchg_lock(&half->lock);
        xchg_lock(&this->lock);
        
        for (int i=0; i<this->size/2; ++i) {
            half->buffer[half->size] = this->get(i);
            half->size++;
            //half->insert(this->get(i));
        }
        xchg_unlock(&this->lock);
        xchg_unlock(&half->lock);
    }

    void concatenation(Array *other, Array *concat) {
        // TODO: make this thread-safe
        // XXX: please avoid deadlock
        concat->clear();

        xchg_lock(&concat->lock);
        xchg_lock(&this->lock);
        xchg_lock(&other->lock);
        
        for (int i=0; i<this->size; ++i) {
            concat->buffer[concat->size] = this->get(i);
            concat->size ++;
        }
        xchg_unlock(&this->lock);
        
        for (int i=0; i<other->size; ++i) {
            concat->buffer[concat->size] = other->get(i);
            concat->size ++;
        }
        xchg_unlock(&other->lock);
        xchg_unlock(&concat->lock);
    }

};











/*
 *
 *  NOTE: PLEASE DO NOT EDIT lines below..
 *
 */

void
thread_func_0(struct thread_args *args) {
    Array *a = (Array *) args->args[0];
    for (int i=0; i<300000; ++i) {
        a->insert(i);
    }
}

void
thread_func_1(struct thread_args *args) {
    usleep(3000);
    Array *a = (Array *) args->args[0];
    for (int i=0; i<300000; ++i) {
        a->remove_last();
    }
}

void
thread_func_2(struct thread_args *args) {
    Array *full = (Array *) args->args[0];
    Array *half = (Array *) args->args[1];
    full->cut_in_half(half);
}

void
thread_func_3(struct thread_args *args) {
    usleep(100);
    Array *a = (Array *) args->args[0];
    for (int i=0; i<300000; ++i) {
        a->insert(i+1000000);
    }
}

void
thread_func_4(struct thread_args *args) {
    Array *a = (Array *) args->args[0];
    Array *b = (Array *) args->args[1];
    Array *concat = (Array *) args->args[2];
    a->concatenation(b, concat);
}



/* DO NOT CHANGE THE FOLLOWING CODE */

void
sigalrm_handler (int signum) {
    printf("DEADLOCK, no points!\n");
    exit(-1);
}



/* insert test */
void
test1() {
    pthread_t *thread_array[2];
    struct thread_args args0;

    Array *a = new Array();

    args0.args[0] = (uint64_t) a;

    thread_array[0] = fork_a_thread((void*) thread_func_0, &args0);
    thread_array[1] = fork_a_thread((void*) thread_func_0, &args0);

    join_a_thread(thread_array[0]);
    join_a_thread(thread_array[1]);

    printf("Test1 Size: %d (must be 600,000)\n", a->size);
}

/* remove test */
void
test2() {
    pthread_t *thread_array[2];
    struct thread_args args0;

    Array *a = new Array();

    args0.args[0] = (uint64_t) a;

    thread_array[0] = fork_a_thread((void*) thread_func_0, &args0);
    thread_array[1] = fork_a_thread((void*) thread_func_1, &args0);

    join_a_thread(thread_array[0]);
    join_a_thread(thread_array[1]);

    printf("Test2 Size: %d (must be 0)\n", a->size);
}

/* cut half test */
void
test3() {
    pthread_t *thread_array[2];
    struct thread_args args0;

    Array *a = new Array();
    Array *b = new Array();

    args0.args[0] = (uint64_t) a;
    args0.args[1] = (uint64_t) b;

    for (int i=0; i<300000; ++i) {
        a->insert(i);
    }

    thread_array[0] = fork_a_thread((void*) thread_func_2, &args0);
    thread_array[1] = fork_a_thread((void*) thread_func_1, &args0);

    join_a_thread(thread_array[0]);
    join_a_thread(thread_array[1]);

    assert(b->size == 150000);

    for (int i=0; i<150000; ++i) {
        assert(b->get(i) == i);
    }

    printf("Test3 Size: %d (must be 150000)\n", b->size);
}

/* concat test */
void
test4() {
    pthread_t *thread_array[4];
    struct thread_args args0, args1, args2, args3;

    Array *a = new Array();
    Array *b = new Array();
    Array *c = new Array();

    args0.args[0] = (uint64_t) a;
    args0.args[1] = (uint64_t) b;
    args0.args[2] = (uint64_t) c;
    args1.args[0] = (uint64_t) a;
    args2.args[0] = (uint64_t) b;
    args3.args[0] = (uint64_t) c;

    for (int i=0; i<300000; ++i) {
        a->insert(i);
    }
    for (int i=0; i<300000; ++i) {
        b->insert(i + 300000);
    }

    thread_array[0] = fork_a_thread((void*) thread_func_4, &args0);
    thread_array[1] = fork_a_thread((void*) thread_func_1, &args1);
    thread_array[2] = fork_a_thread((void*) thread_func_1, &args2);
    thread_array[3] = fork_a_thread((void*) thread_func_3, &args3);

    join_a_thread(thread_array[0]);
    join_a_thread(thread_array[1]);
    join_a_thread(thread_array[2]);
    join_a_thread(thread_array[3]);

    assert(a->size == 0);
    assert(b->size == 0);
    assert(c->size == 900000);

    for (int i=0; i<600000; ++i) {
        assert(c->get(i) == i);
    }

    for (int i=600000; i<900000; ++i) {
        assert(c->get(i) == i + 1000000 - 600000);
    }

    printf("Test4 Size: %d, %d, %d (must be 0, 0, 900000)\n",
            a->size, b->size, c->size);
}

int
main() {
    puts("\t#####################################");
    puts("\t## Welcome  to Thread-safe Array   ##");
    puts("\t#####################################");

    puts("Can you implement a thread-safe array?");
    puts("Please place locks/unlocks correctly to avoid");
    puts("getting inconsistent result, deadlock, etc.");
    puts("Please feel free to insert any function to struct Array..");

    test1();
    test2();
    test3();
    test4();
}
