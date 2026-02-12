# a bash file for compiling mutiple programs
# the option -lrt (use real time library) is needed for mqueue
gcc -Wall -o worker worker.c -lrt
gcc -Wall -o ipc ipc.c -lrt

#to avoid manual compiling every time
# ./compile.sh to run
#if you need to change permissions do:  chmod+777 compile.sh
