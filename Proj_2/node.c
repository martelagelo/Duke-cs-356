#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>


#define MAX_NUM_ROUTING_ENTRIES 64

typedef struct port {
    uint32_t dest_addr;
    uint32_t next_hop;
    int interface_id;
    int cost;
    int state;
} port_t;

typedef struct forwarding_table {
    int num_entries;
    port_t forwarding_entries[MAX_NUM_ROUTING_ENTRIES];
} for_table_t;

typedef struct rip_packet {
	uint16_t command;
	uint16_t num_entries;

	struct {
		uint32_t cost;
		uint32_t address;
	} entries[MAX_NUM_ROUTING_ENTRIES];
} rip_packet_t;

/* Notes:
    rip algorithm cases: http://en.wikipedia.org/wiki/Routing_Information_Protocol
    for listening on multiple fd/sockets's simultaneously use fd_set and select() 

*/

void choose_command(char * command) {
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

    fd_set active_fd_set, read_set;
    fd_set *active_set_ptr;

    active_set_ptr = & active_fd_set;

    FD_ZERO (&active_fd_set);
    FD_SET (1, &active_fd_set);

    char command_line[50];

    while (1) {
    	// check for user input
    		// handle
    	// check for recieved packet
    		// handle

        read_set = active_fd_set;

        if (select (FD_SETSIZE, &read_set, NULL, NULL, NULL) < 0){ 
            perror ("select error");
            exit (EXIT_FAILURE);
        }

        if(FD_ISSET(1, &read_set)) {
            scanf("%s", command_line);
            //printf( "\n%s", commandLine);
            choose_command(command_line);
        }

    }
}