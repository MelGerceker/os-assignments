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
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h> // for execlp
#include <mqueue.h> // for mq

#include "settings.h"
#include "messages.h"

char client2dealer_name[30];
char dealer2worker1_name[30];
char dealer2worker2_name[30];
char worker2dealer_name[30];

pid_t worker1_pids[N_SERV1];
pid_t worker2_pids[N_SERV2];
pid_t client_pid;

// Helpers:
void init_w1_processes()
{
  // Create worker processes for service 1
  for (int i = 0; i < N_SERV1; ++i)
  {
    // Create a child process N_SERV1 times
    pid_t processID = fork();

    // Error handling for fork
    if (processID < 0)
    {
      perror("worker1 fork() failed");
      exit(1);
    }

    if (processID == 0)
    {
      // Child process: execute worker for service 1
      execlp("./worker_s1", "./worker_s1", NULL);
      // Child process should never reach this point, if execlp is successful
      perror("execlp failed");
      exit(1);
    }
    else
    {
      // Parent process: store the PID of the worker
      worker1_pids[i] = processID;
    }
  }
}

void init_w2_processes()
{
  // Create worker processes for service 2
  for (int i = 0; i < N_SERV2; ++i)
  {
    // Create a child process N_SERV2 times
    pid_t processID = fork();

    // Error handling for fork
    if (processID < 0)
    {
      perror("worker2 fork() failed");
      exit(1);
    }

    if (processID == 0)
    {
      // Child process: execute worker for service 2
      execlp("./worker_s2", "./worker_s2", NULL);
      // Child process should never reach this point, if execlp is successful
      perror("execlp failed");
      exit(1);
    }
    else
    {
      // Parent process: store the PID of the worker
      worker2_pids[i] = processID;
    }
  }
}

void init_client_process()
{
  // Client process
  client_pid = fork();
  if (client_pid < 0)
  {
    perror("Client fork() failed");
    exit(1);
  }
  if (client_pid == 0)
  {
    // Child process: execute client
    execlp("./client", "./client", NULL);
    // Child process should never reach this point, if execlp is successful
    perror("execlp failed");
    exit(1);
  }
}

struct mq_attr get_mq_attr(mqd_t mq_fd)
{
  struct mq_attr attr;
  if (mq_getattr(mq_fd, &attr) == -1)
  {
    perror("mq_getattr failed");
    exit(1);
  }
  return attr;
}

int main(int argc, char *argv[])
{
  if (argc != 1)
  {
    fprintf(stderr, "%s: invalid arguments\n", argv[0]);
  }

  // TODO:
  //  * create the message queues (see message_queue_test() in
  //    interprocess_basic.c)

  MQ_REQUEST_MESSAGE req_msg;
  MQ_RESPONSE_MESSAGE rsp_msg;
  MQ_WORKER_MESSAGE worker_msg;

  mqd_t mq_fd_request, mq_fd_response, mq_fd_worker1, mq_fd_worker2;

  char GROUP_NUMBER[] = "73";

  sprintf(client2dealer_name, "/Req_queue_%s_%d", GROUP_NUMBER, getpid());
  sprintf(worker2dealer_name, "/mq_response_%s_%d", GROUP_NUMBER, getpid());
  sprintf(dealer2worker1_name, "/mq_dealer_worker1_%s_%d", GROUP_NUMBER, getpid());
  sprintf(dealer2worker2_name, "/mq_dealer_worker2_%s_%d", GROUP_NUMBER, getpid());

  struct mq_attr attr;
  attr.mq_maxmsg = MQ_MAX_MESSAGES;
  attr.mq_msgsize = sizeof(MQ_REQUEST_MESSAGE);
  mq_fd_request = mq_open(client2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);

  attr.mq_msgsize = sizeof(MQ_RESPONSE_MESSAGE);
  mq_fd_response = mq_open(worker2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);

  attr.mq_msgsize = sizeof(MQ_WORKER_MESSAGE);
  mq_fd_worker1 = mq_open(dealer2worker1_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);
  mq_fd_worker2 = mq_open(dealer2worker2_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);

  //  * create the child processes (see process_test() and
  //    message_queue_test())

  init_w1_processes();
  init_w2_processes();
  init_client_process();

  //  * read requests from the Req queue and transfer them to the workers
  //    with the Sx queues
  int status;
  pid_t terminated_client_pid = 0;
  pid_t terminated_worker_pid = 0;
  //To track how many responses we

  // Loop until the client process has terminated (no more requests can be added)
  // and there are no more requests in the queue
  while (terminated_client_pid != client_pid && get_mq_attr(mq_fd_request).mq_curmsgs > 0)
  {
    // Repeatedly check if client process has terminated
    if (terminated_client_pid != client_pid)
    {
      terminated_client_pid = waitpid(client_pid, &status, WNOHANG);
    }

    // Check if there are requests in the queue
    if (get_mq_attr(mq_fd_request).mq_curmsgs > 0)
    {
      // Read the next request from the Req queue
      mq_receive(mq_fd_request, (char *)&req_msg, sizeof(req_msg), NULL);
      // create worker message
      worker_msg.jobID = req_msg.jobID;
      worker_msg.data = req_msg.data;
      // send worker message to specified queue with respect to its service
      if (req_msg.serviceID == 1)
      {
        mq_send(mq_fd_worker1, (char *)&worker_msg, sizeof(worker_msg), 0);
      }
      else if (req_msg.serviceID == 2)
      {
        mq_send(mq_fd_worker2, (char *)&worker_msg, sizeof(worker_msg), 0);
      }

      // extract the responses if there are any and output them
      if (get_mq_attr(mq_fd_response).mq_curmsgs > 0)
      {
        mq_receive(mq_fd_response, (char *)&rsp_msg, sizeof(rsp_msg), NULL);
        printf("%d -> %d\n", rsp_msg.jobID, rsp_msg.result);
      }
    }

    //

    //  * read answers from workers in the Rep queue and print them
    //  * wait until the client has been stopped (see process_test())
    //  * clean up the message queues (see message_queue_test())

    // Important notice: make sure that the names of the message queues
    // contain your goup number (to ensure uniqueness during testing)

    return (0);
  }
