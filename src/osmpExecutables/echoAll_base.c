//
// Created by knrd on 02.04.25.
//
#include <stdio.h>
#include <unistd.h>

/*
 * echoAll.c
 * Echoes all command line arguments
 * and prints the process ID and parent process ID.
 */
int main(int argc, char *argv[]) {


    fprintf(stdout,"Programmname: %s\n", argv[0]);

    for (int i = 1; i < argc ; i++) {
        fprintf(stdout,"Argument %d: %s\n", i, argv[i]);
        //sleep(1);
    }

    fprintf(stdout,"PID: %d \nPPID: %d \n", getpid(), getppid());
    return 0;
}
