/******************************************************************************
* FILE: osmpExecutable_SendIRecv.c
* DESCRIPTION:
* OSMP program with a simple pair of OSMP_Send/OSMP_Irecv calls
*
* LAST MODIFICATION: Darius Malysiak, March 21, 2023
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../osmpLibrary/OSMP.h"
#include "../osmpLibrary/osmpLib.h"


int main(int argc, char *argv[])
{
    int rv, size, rank, source, len, SIZE = 12;
    char *bufin, *bufout;
    OSMP_Request myrequest = NULL;
    rv = OSMP_Init( &argc, &argv );
    rv = OSMP_Size( &size );
    rv = OSMP_Rank( &rank );
    if( size != 2 ){
        printf("This program requires exactly 2 processes\n");
        rv = OSMP_Finalize();
        return OSMP_FAILURE;
    }
    if( rank == 0 ) {
        // OSMP process 0
        bufin = calloc(1, (size_t) SIZE); // check for != NULL
        len = 12; // length
        memcpy(bufin, "Hello World", (size_t) len);
        rv = OSMP_Send( bufin, len, OSMP_BYTE, 1 );
    }
    else {
        bufout = calloc(1, (size_t) SIZE); // check for != NULL
        rv = OSMP_CreateRequest( &myrequest );
        rv = OSMP_IRecv( bufout, SIZE, OSMP_BYTE, &source, &len, myrequest );
        rv = OSMP_Wait( myrequest );

        printf("Should be Hello World: %s\n", bufout);

        rv = OSMP_RemoveRequest( &myrequest );
    }
    rv = OSMP_Finalize();
    UNUSED(rv);
    return 0;
}
