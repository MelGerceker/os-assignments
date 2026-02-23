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
#include "request.h"

static void rsleep (int t);


int main (int argc, char * argv[])
{
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the message queue (whose name is provided in the
    //    arguments)
    //  * repeatingly:
    //      - get the next job request 
    //      - send the request to the Req message queue
    //    until there are no more requests to send
    //  * close the message queue

    // for each job request:

    MQ_REQUEST_MESSAGE  req;
    req.jobID = 0;
    req.data = 0;
    req.serviceID = 0;
    // shouldnt be zero but should be filled from
    //getNextRequest()

    // mq_send (mq_fd_request, (char *) &req, sizeof (req), 0);
    // validate that send did not have perror()
    
    return (0);
}

// copied and modified from interprocess_basics.c
// not sure if i have to copy this?
typedef struct
{
    // a data structure with 3 members
    int                     jobID;
    int                     data;
    char                    serviceID;
} MQ_REQUEST_MESSAGE;

// to open req queue in read only, name will be sent from
// router dealer.
// mq_fd_request = mq_open (mq_name1, O_RDONLY);

// to push to req queueu:
// mq_send (mq_fd_request, (char *) &req, sizeof (req), 0);
