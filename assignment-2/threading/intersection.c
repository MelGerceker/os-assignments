#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "arrivals.h"
#include "intersection_time.h"
#include "input.h"

/* 
 * curr_arrivals[][][]
 *
 * A 3D array that stores the arrivals that have occurred
 * The first two indices determine the entry lane: first index is Side, second index is Direction
 * curr_arrivals[s][d] returns an array of all arrivals for the entry lane on side s for direction d,
 *   ordered in the same order as they arrived
 */
static Arrival curr_arrivals[4][3][20];

/*
 * semaphores[][]
 *
 * A 2D array that defines a semaphore for each entry lane,
 *   which are used to signal the corresponding traffic light that a car has arrived
 * The two indices determine the entry lane: first index is Side, second index is Direction
 */
static sem_t semaphores[4][3];

/*
 * supply_arrivals()
 *
 * A function for supplying arrivals to the intersection
 * This should be executed by a separate thread
 */
static void* supply_arrivals()
{
  int num_curr_arrivals[4][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

  // for every arrival in the list
  for (int i = 0; i < sizeof(input_arrivals)/sizeof(Arrival); i++)
  {
    // get the next arrival in the list
    Arrival arrival = input_arrivals[i];
    // wait until this arrival is supposed to arrive
    sleep_until_arrival(arrival.time);
    // store the new arrival in curr_arrivals
    curr_arrivals[arrival.side][arrival.direction][num_curr_arrivals[arrival.side][arrival.direction]] = arrival;
    num_curr_arrivals[arrival.side][arrival.direction] += 1;
    // increment the semaphore for the traffic light that the arrival is for
    sem_post(&semaphores[arrival.side][arrival.direction]);
  }

  return(0);
}

// made global since 1 car allowed
static pthread_mutex_t      mutex          = PTHREAD_MUTEX_INITIALIZER;


/*
 * manage_light(void* arg)
 *
 * A function that implements the behaviour of a traffic light
 */
static void* manage_light(void * arg)
{
  // TODO:
  // while it is not END_TIME yet, repeatedly:
  //  - wait for an arrival using the semaphore for this traffic light
  //  - lock the right mutex(es)
  //  - make the traffic light turn green
  //  - sleep for CROSS_TIME seconds
  //  - make the traffic light turn red
  //  - unlock the right mutex(es)


    int* info = (int*) arg;
    Side side = info[0];
    Direction direction = info[1];
    int index = 0;

  while ( get_time_passed() < END_TIME){

      // only semwait doesnt terminate?

    //sem_wait(&semaphores[side][direction]);
    //tried:
    if (sem_trywait(&semaphores[side][direction]) == -1)
{
    usleep(1000);
    continue;
}

    Arrival arrival = curr_arrivals[side][direction][index];
    index++;


    pthread_mutex_lock (&mutex);


    // turn light green
    printf("traffic light %d %d turns green at time %d for car %d\n" , arrival.side, arrival.direction, get_time_passed(), arrival.id);

    sleep(CROSS_TIME);

    //turn light red
    printf("traffic light %d %d turns red at time %d\n", arrival.side, arrival.direction, get_time_passed());

    pthread_mutex_unlock (&mutex);


  }

  return(0);
}


int main(int argc, char * argv[])
{
  // create semaphores to wait/signal for arrivals
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      sem_init(&semaphores[i][j], 0, 0);
    }
  }

  int location[12][2];

  // start the timer
  start_time();

  // TODO: create a thread per traffic light that executes manage_light
    
  pthread_t threads[12];
  int t = 0;
  for(int side = 0; side<4; side++){
    for (int direction=0; direction<3;direction++){
      location[t][0] = side;
      location[t][1] = direction;

      pthread_create(&threads[t], NULL, manage_light, (void*)location[t]); //added void here bc error???
      t++;


    }
  }


//  pthread_t threads[4];
  //for(int i = 0; i < 4; i++){
    //pthread_create(&threads[i], NULL, manage_light, (void*)&i); //added void here bc error???
 // }
  

  // TODO: create a thread that executes supply_arrivals
  pthread_t sup_arr_thread;
  pthread_create(&sup_arr_thread, NULL, supply_arrivals, NULL);
  

  // TODO: wait for all threads to finish'
  pthread_join(sup_arr_thread, NULL);
  for (int i = 0; i < 12; i++)
  { 
    pthread_join(threads[i], NULL);
  }
  

  // destroy semaphores
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      sem_destroy(&semaphores[i][j]);
    }
  }
}
