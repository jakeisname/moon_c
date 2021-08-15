
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <pthread.h>
#include <stdatomic.h>

#define MAX_CNT 100000000UL

pthread_t th1, th2;

int flag;
unsigned long price;

unsigned long pro_mask = 1;
unsigned long con_mask = 2;

void *consumer(void *ptr)
{
        register unsigned long cnt = 1;
        register unsigned long price2;
        int rc;

        printf("consumer thread started\n");

        rc = pthread_setaffinity_np(th2, sizeof(con_mask), (cpu_set_t *)&con_mask);
        if (rc < 0)
                perror("pthread_setaffinity_np failed\n");

        do {
                if (__atomic_load_n(&flag, __ATOMIC_ACQUIRE)) {
                        price2 = price;

                        if (cnt != price2)
                                printf("price=%lu\n", price2);

                        cnt++;
			flag = 0;
                }
        } while (cnt <= MAX_CNT);

        printf("consumer thread finished\n");

        return NULL;
}

void *provider(void *ptr)
{
        unsigned long mask = 0b1; /* processor 1 */
        register unsigned long cnt = 0;
        int rc;
	int flag2 = 1;

        printf("provider thread started\n");

        rc = pthread_setaffinity_np(th1, sizeof(pro_mask), (cpu_set_t *)&pro_mask);
        if (rc < 0)
                perror("pthread_setaffinity_np failed\n");

        do {
                if (!flag) {
                        cnt++;
                        price = cnt;
                        __atomic_store(&flag, &flag2, __ATOMIC_RELEASE);
                }
        } while (cnt < MAX_CNT);

        printf("provider thread finished\n");

        return NULL;
}

void main(int argc, char **argv)
{
        int rc;

        /* test 0 1 -> cpu#0 and cpu#1 */
        if (argc > 1)
                pro_mask = 1 << atol(argv[1]);

        if (argc > 2)
                con_mask = 1 << atol(argv[2]);

        printf("started. barrier=atomic_load & atomic_store\n");

        rc = pthread_create(&th1, NULL, *provider, NULL);
        if (rc < 0) {
                perror("create provider thread failed.\n");
                exit(0);
        }

        rc = pthread_create(&th2, NULL, *consumer, NULL);
        if (rc < 0) {
                perror("create consumer thread failed.\n");
                exit(0);
        }

	pthread_join(th1, NULL);
	pthread_join(th2, NULL);

        printf("finished. cnt=%lu\n", MAX_CNT);
}

