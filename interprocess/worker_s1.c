/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
 *
 * Grading:
 * Your work will be evaluated based on the following criteria:
 * - Satisfaction of all the specifications
 * - Correctness of the program
 * - Coding style
 * - Report quality
 * - Deadlock analysis
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>      // for perror()
#include <unistd.h>     // for getpid()
#include <mqueue.h>     // for mq-stuff
#include <time.h>       // for time()

#include "messages.h"
#include "service1.h"

static void rsleep (int t);


int main (int argc, char * argv[])
{
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the two message queues (whose names are provided in the
    //    arguments)
    //  * repeatedly:
    //      - read from the S1 message queue the new job to do
    //      - wait a random amount of time (e.g. rsleep(10000);)
    //      - do the job 
    //      - write the results to the Rsp message queue
    //    until there are no more tasks to do
    //  * close the message queues

    if (argc != 3) {
        fprintf(stderr, "usage: %s <Rsp_queue_name> <S1_queue_name>\n", argv[0]);
        return 1;
    }

    const char *rsp_name = argv[1];
    const char *s1_name = argv[2];

    //open S1 queue for reading
    mqd_t s1_mq = mq_open(s1_name, O_RDONLY);
    if (s1_mq == (mqd_t)-1) {
        perror("worker_s1: mq_open S1");
        return 1;
    }
    
    //open Rsp queue for writing
    mqd_t rsp_mq = mq_open(rsp_name, O_WRONLY);
    if (rsp_mq == (mqd_t)-1) {
        perror("worker_s1: mq_open Rsp");
        mq_close(s1_mq);
        return 1;
    }

    //repeatedly: receive job -> do service1 -> rsleep -> send result
    while(1) {
        MQ_WORKER_MESSAGE job;
        ssize_t n = mq_receive(s1_mq, (char *)&job, sizeof(job), NULL);
        if (n < 0) {
            if (errno == EINTR) continue; //retry if interrupted
            perror("worker_s1: mq_receive");
            break;
        }

        if((size_t)n != sizeof(MQ_WORKER_MESSAGE)) {
            fprintf(stderr, "worker_s1: wrong message size %zd (expected %zu)\n", n, sizeof(MQ_WORKER_MESSAGE));
            continue;
        }

        if (job.jobID < 0) {
            break;
        }

        int res = service(job.data);

        //required random sleep between receiving and sending
        rsleep(10000);

        MQ_RESPONSE_MESSAGE resp;
        resp.jobID = job.jobID;
        resp.result = res;

        if(mq_send(rsp_mq, (const char *)&resp, sizeof(resp), 0) == -1) {
            if (errno == EINTR) continue; 
            perror("worker_s1: mq_send");
            break;   
        }
    }

    mq_close(s1_mq);
    mq_close(rsp_mq);
    return(0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time (NULL) % getpid ());
        first_call = false;
    }
    usleep (random() % t);
}
