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

// mutexes:
pthread_mutex_t path_mutexes[7];
// where

/*
0->147
1->2479
2->258
3->369
4->46
5->56
6->57
*/

int mutexes_for_path[10][3] = { // 3 since max ever is 3

    {-1, -1, -1}, // skip path 0 so its like 1-based indexing
    {0, -1, -1},  // path 1 which only uses mutex 0 which is 147
    {1, 2, -1},   // path 2 which uses mutex 1(2479) and mutex 2(258)
    {3, -1, -1},
    {0, 1, 4},
    {2, 5, 6},
    {3, 4, 5},
    {0, 1, 6},
    {2, -1, -1},
    {1, 3, -1}

};

static bool Terminate = false;

/*
 * supply_arrivals()
 *
 * A function for supplying arrivals to the intersection
 * This should be executed by a separate thread
 */
static void *supply_arrivals()
{
  int num_curr_arrivals[4][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

  // for every arrival in the list
  for (int i = 0; i < sizeof(input_arrivals) / sizeof(Arrival); i++)
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

  return (0);
}

static int get_path(Side side, Direction direction)
{
  if (side == NORTH)
  {
    if (direction == RIGHT)
    {
      return 1;
    }
    if (direction == STRAIGHT)
    {
      return 2;
    }
  }

  if (side == EAST)
  {
    if (direction == RIGHT)
    {
      return 3;
    }
    if (direction == STRAIGHT)
    {
      return 4;
    }
    if (direction == LEFT)
    {
      return 5;
    }
  }

  if (side == SOUTH)
  {
    if (direction == STRAIGHT)
    {
      return 6;
    }
    if (direction == LEFT)
    {
      return 7;
    }
  }

  if (side == WEST)
  {
    if (direction == RIGHT)
    {
      return 8;
    }
    if (direction == LEFT)
    {
      return 9;
    }
  }
  return -1; // unused lane?
}

static void lock_path_mutexes(int path)
{
  for (int i = 0; i < 3; i++)
  {
    int m = mutexes_for_path[path][i];
    if (m == -1)
    {
      break;
    }
    pthread_mutex_lock(&path_mutexes[m]);
  }
}

static void unlock_appropriate_mutexes(int path)
{
  for (int i = 0; i < 3; i++)
  {
    int m = mutexes_for_path[path][i];
    if (m == -1)
    {
      break;
    }
    pthread_mutex_unlock(&path_mutexes[m]);
  }
}

/*
 * manage_light(void* arg)
 *
 * A function that implements the behaviour of a traffic light
 */
static void *manage_light(void *arg)
{
  // TODO:
  // while it is not END_TIME yet, repeatedly:
  //  - wait for an arrival using the semaphore for this traffic light
  //  - lock the right mutex(es)
  //  - make the traffic light turn green
  //  - sleep for CROSS_TIME seconds
  //  - make the traffic light turn red
  //  - unlock the right mutex(es)

  int *info = (int *)arg;
  Side side = info[0];
  Direction direction = info[1];
  int index = 0;

  while (get_time_passed() < END_TIME)
  {

    sem_wait(&semaphores[side][direction]);

    // Check for termination flag
    if (Terminate)
    {
      break;
    }

    Arrival arrival = curr_arrivals[side][direction][index];
    index++;

    // pthread_mutex_lock(&mutex);
    int path = get_path(side, direction);


    lock_path_mutexes(path);


    // turn light green
    printf("traffic light %d %d turns green at time %d for car %d\n", arrival.side, arrival.direction, get_time_passed(), arrival.id);

    sleep(CROSS_TIME);

    // turn light red
    printf("traffic light %d %d turns red at time %d\n", arrival.side, arrival.direction, get_time_passed());

    unlock_appropriate_mutexes(path);
  }

  return (0);
}


// void unlock_all_mutexes()
// {  
//   for (int i = 0; i < 7; i++)
//   {
//     pthread_mutex_unlock(&path_mutexes[i]);
//   }
// }

int main(int argc, char *argv[])
{
  // create semaphores to wait/signal for arrivals
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      sem_init(&semaphores[i][j], 0, 0);
    }
  }

  for (int i = 0; i < 7; i++)
  {
    pthread_mutex_init(&path_mutexes[i], NULL);
  }

  int location[12][2];

  // start the timer
  start_time();

  // TODO: create a thread per traffic light that executes manage_light

  pthread_t threads[12];
  int t = 0;
  for (int side = 0; side < 4; side++)
  {
    for (int direction = 0; direction < 3; direction++)
    {
      location[t][0] = side;
      location[t][1] = direction;

      pthread_create(&threads[t], NULL, manage_light, (void *)location[t]);
      t++;
    }
  }

  // TODO: create a thread that executes supply_arrivals
  pthread_t sup_arr_thread;
  pthread_create(&sup_arr_thread, NULL, supply_arrivals, NULL);

  // TODO: wait for all threads to finish'
  // Wait for the supply thread to finish
  pthread_join(sup_arr_thread, NULL);

  // Wait until endtime is reached and set Terminate flag
  // for threads to terminate instead of trying to fetch new information.
  sleep_until_arrival(END_TIME);
  Terminate = true;
  // release the threads by posting to each semaphore
  for (int side = 0; side < 4; side++)
  {
    for (int direction = 0; direction < 3; direction++)
    {
      sem_post(&semaphores[side][direction]);
    }
  }

  // Collect terminated threads.
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

  for (int i = 0; i < 7; i++)
  {
    pthread_mutex_destroy(&path_mutexes[i]);
  }
}
