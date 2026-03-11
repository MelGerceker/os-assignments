// definition of a struct
// we put this into a header file, so that we only need to define it once
//  but use it in both the ipc.c and worker.c
struct message
{
    int mid;
    int result;
};
