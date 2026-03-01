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
    mqd_t               mq_fd_request;


    int jobID, data, serviceID;

    const char * mq_name1 = argv[1];

    while(1){


       // to open req queue in read only, name will be sent from router dealer.
        mq_fd_request = mq_open(mq_name1, O_RDONLY);

        int nextJob = getNextRequest(&jobID, &data, &serviceID);

        if (nextJob == NO_REQ) {
        // no requests to make
        break;

        }
        else {
        // means everything is good, NO_ERR
        // fill the msg
        req.jobID = jobID;
        req.data = data;
        req.serviceID = serviceID;

        mq_send (mq_fd_request, (char *) &req, sizeof (req), 0);
        // validate that send did not have perror()

        }

    }

    // release resources
    mq_close(mq_fd_request);
    
    
    return (0);
}
