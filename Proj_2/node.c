#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct rip_packet {
	uint16_t command;
	uint16_t num_entries;
	struct {
		uint32_t cost;
		uint32_t address;
	} entries[num_entries];
}

/* Notes:
    rip algorithm cases: http://en.wikipedia.org/wiki/Routing_Information_Protocol
    for listening on multiple fd/sockets's simultaneously use fd_set and select() 

*/

void chooseCommand(char * command) {
    if(strcmp("ifconfig", command) == 0) {
        //do dis 
        printf("ifconfig\n");
    }
    else if (strcmp("route", command) == 0) {
        //do this other thing
        printf("route\n");
    }
    else if (strcmp("up", command) == 0) {
        //do this other thing
        printf("up\n");
    }
    else if (strcmp("down", command) == 0) {
        //do this other thing
        printf("down\n");
    }
    else if (strcmp("yo", command) == 0) { //send
        printf("send\n");
    }
    else {
        printf("\nCommand not found. Please enter a different command.\n");
    }
}


int main(int argc, char ** argv) {
    // Initialize based on input file
    // setup non-blocking UDP recieve socket
    // setup non-blocking UDP send socket
    // initialize routing information

    fd_set active_fd_set, readSet;
    fd_set *active_set_ptr;

    active_set_ptr = & active_fd_set;

    FD_ZERO (&active_fd_set);
    FD_SET (0, &active_fd_set);

    char commandLine[50];

    while (1) {
    	// check for user input
    		// handle
    	// check for recieved packet
    		// handle

        readSet = active_fd_set;

        if (select (FD_SETSIZE, &readSet, NULL, NULL, NULL) < 0){ 
            perror ("select error");
            exit (EXIT_FAILURE);
        }

        if(FD_ISSET(0, &readSet)) {
            scanf("%s", commandLine);
            //printf( "\n%s", commandLine);
            chooseCommand(commandLine);
        }
    }
}