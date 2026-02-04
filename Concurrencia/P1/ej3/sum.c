#include <errno.h>
#include <pthread.h>
#include <stdio.h>
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

    pthread_mutex_t *mutex_ej3; //EJERCICIO 3
    long size; //EJERCICIO 3
};

struct args {
	int thread_num;		// application defined thread #
	long iterations;	// number of operations
	struct nums *nums;	// pointer to the counters (shared with other threads)

    pthread_mutex_t *mutex_ej1;  //EJERCICIO 1
    pthread_mutex_t *mutex_ej2; //EJERCICIO 2
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

        pthread_mutex_lock(args->mutex_ej2); //EJERCICIO 2
        if(n->cnt_ej2 > 0) { //EJERCICIO 2
            n->cnt_ej2--; //EJERCICIO 2
            pthread_mutex_unlock(args->mutex_ej2); //EJERCICIO 2


            int increment_random = rand() % n->size;  //EJERCICIO 3
            int decrement_random = rand() % n->size;  //EJERCICIO 3

            pthread_mutex_lock(&n->mutex_ej3[increment_random]);  //EJERCICIO 3
            n->increase[increment_random]++;  //EJERCICIO 3
            pthread_mutex_unlock(&n->mutex_ej3[increment_random]);  //EJERCICIO 3

            pthread_mutex_lock(&n->mutex_ej3[decrement_random]);  //EJERCICIO 3
            n->decrease[decrement_random]--;  //EJERCICIO 3
            pthread_mutex_unlock(&n->mutex_ej3[decrement_random]);  //EJERCICIO 3

        }
        else { //EJERCICIO 2
            pthread_mutex_unlock(args->mutex_ej2);  //EJERCICIO 2
            break;  //EJERCICIO 2
        }
    }
    return NULL;
}

// start opt.num_threads threads running on decrease_incresase
struct thread_info *start_threads(struct options opt, struct nums *nums)
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

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    // Create num_thread threads running decrease_increase
    for (i = 0; i < opt.num_threads; i++) {
        threads[i].args = malloc(sizeof(struct args));
        threads[i].args->mutex_ej1 = mutex_ej1;  //EJERCICIO 1
        threads[i].args->mutex_ej2 = mutex_ej2;  //EJERCICIO 2
        threads[i].args->thread_num = i;
        threads[i].args->nums       = nums;
        threads[i].args->iterations = opt.iterations;

        if (0 != pthread_create(&threads[i].id, NULL, decrease_increase, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }

    return threads;
}

void print_totals(struct nums *nums) //EJERCICIO 3
{
    long incrAcc = 0;
    long decrAcc = 0;
    for (int i = 0; i < nums->size; ++i) {
        incrAcc += nums->increase[i];
        decrAcc += nums->decrease[i];
    }
    long diff = (nums->total * nums->size) - (incrAcc + decrAcc);

    printf("Final: increasing %ld decreasing %ld suma %ld =? sol %ld        diff: %ld\n",
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
    pthread_mutex_destroy(threads->args->mutex_ej1);  //EJERCICIO 1
    pthread_mutex_destroy(threads->args->mutex_ej2);  //EJERCICIO 2

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

    nums.total = opt.iterations * opt.num_threads;
    nums.increase = malloc(opt.size * sizeof(long)); //EJERCICIO 3
    nums.decrease = malloc(opt.size * sizeof(long)); //EJERCICIO 3
    nums.mutex_ej3 = malloc(opt.size * sizeof(pthread_mutex_t)); //EJERCICIO 3
    nums.size = opt.size;  //EJERCICIO 3
    nums.diff = 0;
    nums.cnt_ej2 = opt.iterations; //EJERCICIO 2

    for (int i = 0; i < opt.size; ++i) {  //EJERCICIO 3
        nums.increase[i] = 0;
        nums.decrease[i] = nums.total;
        pthread_mutex_init(&nums.mutex_ej3[i], NULL);
    }
    thrs = start_threads(opt, &nums);
    wait(opt, &nums, thrs);

    return 0;
}
