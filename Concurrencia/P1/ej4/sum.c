#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "options.h"

struct nums {
    long *increase;  //EJERCICIO 3
    long *decrease;  //EJERCICIO 3
    long total;
    long diff;

    long cnt_ej2;  //EJERCICIO 2

    pthread_mutex_t *mutex_ej2; //EJERCICIO 2
    pthread_mutex_t *mutex_ej3; //EJERCICIO 3
    long size; //EJERCICIO 3
};

struct args {
    int thread_num;		// application defined thread #
    long iterations;	// number of operations
    struct nums *nums;	// pointer to the counters (shared with other threads)

    pthread_mutex_t *mutex_ej1;  //EJERCICIO 1

};

struct thread_info {
    pthread_t    id;    // id returned by pthread_create()
    struct args *args;  // pointer to the arguments
};

// Threads run on this function
void *decrease_increase(void *ptr)
{
    struct args *args = ptr;
    struct nums *n = args->nums;

    while(args->iterations--) {

        pthread_mutex_lock(n->mutex_ej2); //EJERCICIO 2
        if(n->cnt_ej2 > 0) { //EJERCICIO 2
            n->cnt_ej2--; //EJERCICIO 2
            pthread_mutex_unlock(n->mutex_ej2); //EJERCICIO 2


            long increment_random = rand() % n->size;  //EJERCICIO 3
            long decrement_random = rand() % n->size;  //EJERCICIO 3

            pthread_mutex_lock(&n->mutex_ej3[increment_random]);  //EJERCICIO 3
            n->increase[increment_random]++;  //EJERCICIO 3
            pthread_mutex_unlock(&n->mutex_ej3[increment_random]);  //EJERCICIO 3

            pthread_mutex_lock(&n->mutex_ej3[decrement_random]);  //EJERCICIO 3
            n->decrease[decrement_random]--;  //EJERCICIO 3
            pthread_mutex_unlock(&n->mutex_ej3[decrement_random]);  //EJERCICIO 3

        }
        else { //EJERCICIO 2
            pthread_mutex_unlock(n->mutex_ej2);  //EJERCICIO 2
            break;  //EJERCICIO 2
        }
    }
    return NULL;
}
void *increment_ej4(void *ptr) {
    struct args *args = ptr;
    struct nums *n = args->nums;

    while (true) {
        pthread_mutex_lock(n->mutex_ej2);
        if(n->cnt_ej2 >= 0){
            n->cnt_ej2--;
            pthread_mutex_unlock(n->mutex_ej2);
            break;
        }


        long pos1 = rand() % n->size;
        long pos2;
        do {
            pos2 = rand() % n->size;
        } while (pos1 == pos2);

        pthread_mutex_lock(&n->mutex_ej3[pos1]);
        n->increase[pos1]++;
        pthread_mutex_unlock(&n->mutex_ej3[pos1]);

        pthread_mutex_lock(&n->mutex_ej3[pos2]);
        n->increase[pos2]--;
        pthread_mutex_unlock(&n->mutex_ej3[pos2]);

    }
    return NULL;
}

// start opt.num_threads threads running on decrease_incresase
struct thread_info *start_threads(struct options opt, struct nums *nums, struct thread_info *ej4)
{
    int i;
    struct thread_info *threads;

    pthread_mutex_t  *mutex_ej1; //EJERCICIO 1
    pthread_mutex_t  *mutex_ej2; //EJERCICIO 2

    printf("creating %d threads\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * opt.num_threads);


    mutex_ej1 = malloc(sizeof(pthread_mutex_t));  //EJERCICIO 1
    pthread_mutex_init(mutex_ej1, NULL); //EJERCICIO 1

    mutex_ej2 = malloc(sizeof(pthread_mutex_t)); //EJERCICIO 2
    pthread_mutex_init(mutex_ej2, NULL); //EJERCICIO 2

    if (threads == NULL || ej4 == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    // Create num_thread threads running decrease_increase
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].args = malloc(sizeof(struct args));
        threads[i].args->mutex_ej1 = mutex_ej1;  //EJERCICIO 1
        threads[i].args->thread_num = i;
        threads[i].args->nums       = nums;
        threads[i].args->iterations = opt.iterations;

        if (0 != pthread_create(&threads[i].id, NULL, decrease_increase, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }
    for (i = 0; i < opt.num_threads; i++) {
        ej4->args->nums->cnt_ej2 = opt.iterations;
        ej4->args->nums->mutex_ej2 = mutex_ej2;
        ej4->args->mutex_ej1 = mutex_ej1;
        if (0 != pthread_create(&ej4[i].id, NULL, increment_ej4, ej4[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    return threads;
}


void print_totals(struct nums *nums) {
    long incrAcc = 0;
    long decrAcc = 0;
    for (int i = 0; i < nums->size; ++i) {
        incrAcc += nums->increase[i];
        decrAcc += nums->decrease[i];
    }
    long diff = (nums->total * nums->size) - (incrAcc + decrAcc);

    printf("Final: increasing %ld decreasing %ld\n"
           "Array incrementos + Array decrementos (%ld) =? Total * Tamaño Array (%ld)\n"
           "diff: %ld\n",
           incrAcc, decrAcc, incrAcc + decrAcc , nums->total * nums->size, diff);
}


// wait for all threads to finish, print totals, and free memory
void wait(struct options opt, struct nums *nums, struct thread_info *threads) {
    // Wait for the threads to finish
    for (int i = 0; i < opt.num_threads; i++)
        pthread_join(threads[i].id, NULL);

    print_totals(nums);
    for (int i = 0; i < opt.size; ++i) { //EJERCICIO 3
        pthread_mutex_destroy(&nums->mutex_ej3[i]);
    }

    pthread_mutex_destroy(nums->mutex_ej2);  //EJERCICIO 2

    for (int i = 0; i < opt.num_threads; i++)
        free(threads[i].args);

    free(threads);
    free(nums->increase); //EJERCICIO 3
    free(nums->decrease); //EJERCICIO 3
    free(nums->mutex_ej3); //EJERCICIO 3
}

int main (int argc, char **argv)
{
    struct options opt;
    struct nums nums;
    struct thread_info *thrs;

    srand(time(NULL));

    // Default values for the options
    opt.num_threads  = 4;
    opt.iterations   = 100000;
    opt.size         = 10;

    read_options(argc, argv, &opt);

    struct thread_info thrs_ej4[opt.num_threads];
    struct args args_ej4[opt.num_threads];


    struct nums_ej4_struct {//EJERCICIO 3
        long increase[1024];
        long decrease[1024];
        long total;
        long diff;

        long cnt_ej2;  //EJERCICIO 2
        pthread_mutex_t mutex_ej2;
        pthread_mutex_t mutex_ej3[1024]; //EJERCICIO 3
        long size; //EJERCICIO 3
    };

    struct nums_ej4_struct nums_ej4;

    nums.total = opt.iterations * opt.num_threads;
    nums.increase = malloc(opt.size * sizeof(long)); //EJERCICIO 3
    nums.decrease = malloc(opt.size * sizeof(long)); //EJERCICIO 3
    nums.mutex_ej2 = malloc(sizeof(pthread_mutex_t)); //EJERCICIO 3
    nums.mutex_ej3 = malloc(opt.size * sizeof(pthread_mutex_t)); //EJERCICIO 3
    nums.size = opt.size;  //EJERCICIO 3
    nums.diff = 0;
    nums.cnt_ej2 = opt.iterations; //EJERCICIO 2

    pthread_mutex_init(nums.mutex_ej2, NULL);
    for (int i = 0; i < opt.size; ++i) {  //EJERCICIO 3
        nums.increase[i] = 0;
        nums.decrease[i] = nums.total;
        pthread_mutex_init(&nums.mutex_ej3[i], NULL);
    }

    nums_ej4.total = opt.iterations * opt.num_threads;
    nums_ej4.size = opt.size;
    nums_ej4.diff = 0;
    nums_ej4.cnt_ej2 = opt.iterations;
    pthread_mutex_init(&nums_ej4.mutex_ej2, NULL);

    for (int i = 0; i < opt.size; ++i) {  //EJERCICIO 3
        nums_ej4.increase[i] = 0;
        nums_ej4.decrease[i] = nums.total;
        pthread_mutex_init(&nums_ej4.mutex_ej3[i], NULL);
    }

    for (int i = 0; i < opt.num_threads; ++i) {
        args_ej4[i].nums = (struct nums *) &nums_ej4;
        args_ej4[i].thread_num = opt.num_threads + i;
    }

    for (int i = 0; i < opt.num_threads; ++i) {
        thrs_ej4[i].args = &args_ej4[i];
    }



    thrs = start_threads(opt, &nums, thrs_ej4);
    wait(opt, &nums, thrs);

    return 0;
}
