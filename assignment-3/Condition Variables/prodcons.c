/*
 * Operating Systems  (2INC0)  Practical Assignment.
 * Condition Variables Application.
 *
 * Rovshan Ayyubov (2157470)
 * Yagmur Yilmaz (2107724)
 * Melisa Gerceker (2134160)
 * 
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8.
 * Extra steps can lead to higher marks because we want students to take the initiative.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "prodcons.h"

static ITEM buffer[BUFFER_SIZE];

static void rsleep(int t);		 // already implemented (see below)
static ITEM get_next_item(void); // already implemented (see below)

// To track the next item to be inserted into the buffer
static ITEM expected_item = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t correct_order = PTHREAD_COND_INITIALIZER;

//counters for stderr
static int signal_count = 0;
static int broadcast_count = 0;
static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

//buffer variables:
//write index:
static int in = 0;
//read index:
static int out = 0;
//new information count:
static int count = 0;

//to track producer thread terminations:
static int producers_done = 0;

void my_signal(pthread_cond_t *cond) {
    pthread_mutex_lock(&count_mutex);
    signal_count++;
    pthread_mutex_unlock(&count_mutex);

    pthread_cond_signal(cond);
}

void my_broadcast(pthread_cond_t *cond) {
    pthread_mutex_lock(&count_mutex);
    broadcast_count++;
    pthread_mutex_unlock(&count_mutex);

    pthread_cond_broadcast(cond);
}

/* producer thread */
static void *
producer(void *arg)
{
	while (true /* TODO: not all items produced */)
	{
		// TODO:
		// * get the new item
		ITEM curr_item = get_next_item();

		if (curr_item == NROF_ITEMS)
		{ // We are done.
            pthread_mutex_lock(&mutex);
            producers_done++;
            // wake consumer in case it is waiting on an empty buffer 
            my_signal(&not_empty);
            pthread_mutex_unlock(&mutex);
			break;
		}

		rsleep(100); // simulating all kind of activities...

		// TODO:
		// * put the item into buffer[]
		//
		// follow this pseudocode (according to the ConditionSynchronization lecture):
		//      mutex-lock;
		pthread_mutex_lock(&mutex);
		//      while not condition-for-this-producer
		//condition = 
		while (count == BUFFER_SIZE){
    		pthread_cond_wait(&not_full, &mutex);
		}
		while (curr_item != expected_item){
			pthread_cond_wait(&correct_order, &mutex);
		}
		// critical section: insert item into FIFO buffer 
        buffer[in] = curr_item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        expected_item++;
		
		//      possible-cv-signals;
		my_signal(&not_empty);
		my_broadcast(&correct_order);

		//      mutex-unlock;
		pthread_mutex_unlock(&mutex);
		// (see condition_test() in condition_basics.c how to use condition variables)
	}
	return (NULL);
}

/* consumer thread */
static void *
consumer(void * arg)
{
	while (true)
    {
		// TODO: 
		// * get the next item from buffer[] 
		// * print the number to stdout 
		// follow this pseudocode (according to the ConditionSynchronization lecture): 
		// mutex-lock; 
		// while not condition-for-this-consumer 
		// wait-cv; 
		// critical-section; 
		// possible-cv-signals; 
		// mutex-unlock;
        pthread_mutex_lock(&mutex);

		//if there is no new information in buffer, wait
        while (count == 0)
        {
			//if all producers are terminated while waiting:
            if (producers_done == NROF_PRODUCERS)
            {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            pthread_cond_wait(&not_empty, &mutex);
        }

		//else, process the new information:
        ITEM item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;

		my_signal(&not_full);

        pthread_mutex_unlock(&mutex);

        printf("%d\n", item);

        rsleep(100); // simulating all kind of activities...
	}
	return (NULL);
}

int main(void)
{
	pthread_t producer_threads[NROF_PRODUCERS];
	pthread_t consumer_thread;

	// Create producer threads
	for (int k = 0; k < NROF_PRODUCERS; k++)
	{
		pthread_create(&producer_threads[k], NULL, producer, NULL);
	}


    // startup consumer thread 
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    // wait until all producer threads are finished 
    for (int k = 0; k < NROF_PRODUCERS; k++)
    {
    	pthread_join(producer_threads[k], NULL);
 
    }

    // wait until consumer thread is finished
    pthread_join(consumer_thread, NULL);

	fprintf(stderr, "Signals: %d\n", signal_count);
	fprintf(stderr, "Broadcasts: %d\n", broadcast_count);

	return (0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void
rsleep(int t)
{
	static bool first_call = true;

	if (first_call == true)
	{
		srandom(time(NULL));
		first_call = false;
	}
	usleep(random() % t);
}

/*
 * get_next_item()
 *
 * description:
 *	thread-safe function to get a next job to be executed
 *	subsequent calls of get_next_item() yields the values 0..NROF_ITEMS-1
 *	in arbitrary order
 *	return value NROF_ITEMS indicates that all jobs have already been given
 *
 * parameters:
 *	none
 *
 * return value:
 *	0..NROF_ITEMS-1: job number to be executed
 *	NROF_ITEMS:	 ready
 */
static ITEM
get_next_item(void)
{
	static pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
	static bool jobs[NROF_ITEMS + 1] = {false}; // keep track of issued jobs
	static int counter = 0;						// seq.nr. of job to be handled
	ITEM found;									// item to be returned

	/* avoid deadlock: when all producers are busy but none has the next expected item for the consumer
	 * so requirement for get_next_item: when giving the (i+n)'th item, make sure that item (i) is going to be handled (with n=nrof-producers)
	 */
	pthread_mutex_lock(&job_mutex);

	counter++;
	if (counter > NROF_ITEMS)
	{
		// we're ready
		found = NROF_ITEMS;
	}
	else
	{
		if (counter < NROF_PRODUCERS)
		{
			// for the first n-1 items: any job can be given
			// e.g. "random() % NROF_ITEMS", but here we bias the lower items
			found = (random() % (2 * NROF_PRODUCERS)) % NROF_ITEMS;
		}
		else
		{
			// deadlock-avoidance: item 'counter - NROF_PRODUCERS' must be given now
			found = counter - NROF_PRODUCERS;
			if (jobs[found] == true)
			{
				// already handled, find a random one, with a bias for lower items
				found = (counter + (random() % NROF_PRODUCERS)) % NROF_ITEMS;
			}
		}

		// check if 'found' is really an unhandled item;
		// if not: find another one
		if (jobs[found] == true)
		{
			// already handled, do linear search for the oldest
			found = 0;
			while (jobs[found] == true)
			{
				found++;
			}
		}
	}
	jobs[found] = true;

	pthread_mutex_unlock(&job_mutex);
	return (found);
}
