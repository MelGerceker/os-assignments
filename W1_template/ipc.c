#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <mqueue.h>

#include "messages.h"

int main(int argc, char * argv[])
{
    //how to create a channel:
    struct mq_attr attr;
    attr.mq_maxmsg = 4;
    attr.mq_msgsize = sizeof(struct message);
    char channelName[30] = "/theMines";
    mqd_t channel = mq_open(channelName, O_CREAT | O_RDONLY | O_EXCL |O_NONBLOCK , 0600, &attr);
    //O_nonblock is for?
    // the 0600 is the file permissions
    // channel name should always start with a /
    // you have to tell OS when you are no longer using hte channel, so close(channel) at the end

    // to verify
    if(channel == -1)
    {
        perror("Channel creation failed!\n");
    }


    int pid1 = fork();
    printf("Hello world!\n");
    
    printf("pid1: %d\n", pid1);
    
    if(pid1 == 0) // if child
    {
        sleep(3);
        printf("I'm the child!\n");

        execlp("./worker", "childWorkerName", channelName, NULL); //so execute the worker file
        // execlp can accept any number of arguments, the first one should always be the exe file
        // the last param should alwasy be NULL so that it knows the number of arguments
        // char used in worker.c main() so it assumes the param are strings

        // to pass data while process is running, use communication channels.

        printf("This part should never be reached\n");
        exit(6);
     }
    
    printf("I'm the parent!\n");


    struct message m;
    int result = mq_receive(channel, (char*)&m, sizeof(struct message), 0);
    if(result == -1)
    {
       perror("Receiving failed!");
    }

    printf("Received message with id %d and result %d\n", m.mid, m.result);


    int status;
    
    wait(NULL);
    
    waitpid(pid1, &status, 0);
    printf("status: %d\n", status);

    //WNOHANG useful for process that needs to wait for multiple child process
    // creates overhead but it is sometimes necessary

    // blocking wait for waiting on one thing?
    
    //changed the last pid to pid1?
    //while (waitpid(pid1, &status, WNOHANG) != pid1)
    //{
    //    printf("waiting...\n");
        //sleep(1);
    //}


    mq_close(channel);
    mq_unlink(channelName);
}
