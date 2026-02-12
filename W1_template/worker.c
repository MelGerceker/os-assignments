#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>

#include "messages.h"

// argc is the number of arguments the process has recieved
// argv pointer to the arguments

int main(int argc, char * argv[])
{
    printf("Working...\n");

    printf("number of arguments: %d\n", argc);
    printf("first param: %s\n", argv[0]);
    printf("second param: %s\n", argv[1]);

    mqd_t channel = mq_open(argv[1], O_WRONLY);
    //0_WRONLY means write only??
    if(channel == -1)
    {
         perror("Channel creation failed (worker)!");
    }

    struct message m;
    m.mid = 2;
    m.result = 77;

    sleep(3);
    
    int result = mq_send(channel, (char*)&m, sizeof(struct message), 0);
    // 0 is priortiy, can be ignored
    if(result == -1)
    {
        perror("Sending message failed (worker)!");
    }

    mq_close(channel);
    // dont need the unlink?
}
