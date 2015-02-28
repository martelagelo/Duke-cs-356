#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>


#define MAX_NUM_ROUTING_ENTRIES 64

typedef struct forwarding_entry {
    uint32_t dest_addr;
    uint32_t next_hop;
    int interface_id;
    int cost;
    char *state;
} forwarding_entry_t;

typedef struct forwarding_table {
    int num_entries;
    forwarding_entry_t forwarding_entries[MAX_NUM_ROUTING_ENTRIES];
} forwarding_table_t;

typedef struct ifconfig_entry {
    int interface_id;
    char *state;
    uint32_t source_addr;
} ifconfig_entry_t;

typedef struct ifconfig_table {
    int num_entries;
    ifconfig_entry_t ifconfig_entries[MAX_NUM_ROUTING_ENTRIES];
} ifconfig_table_t;

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

forwarding_table_t FORWARDING_TABLE;
ifconfig_table_t IFCONFIG_TABLE;


void build_forwarding_table() {
    
}

void send_packet(uint32_t destination, char * msg) {
    int i;
    for(i = 0; i < FORWARDING_TABLE.num_entries; i++) {
        if(FORWARDING_TABLE.forwarding_entries[i].dest_addr == destination) {
            //send message
            return;
        }
    }
    printf("Path does not exist\n");
}

void forward_packet() {

}

void set_as_up(int ID) {
    int i;
    for(i = 0; i < IFCONFIG_TABLE.num_entries; i++) {
        if(IFCONFIG_TABLE.ifconfig_entries[i].interface_id == ID) {
            IFCONFIG_TABLE.ifconfig_entries[i].state == "up";
            printf("Interface %d is up.\n", ID);
            return;
        }
    }
    //Needs to also update forwarding table and network table
    printf("Interface %d is not found.\n", ID);
}

void set_as_down(int ID) {
    int i;
    for(i = 0; i< IFCONFIG_TABLE.num_entries; i++) {
        if(IFCONFIG_TABLE.ifconfig_entries[i].interface_id == ID) {
            IFCONFIG_TABLE.ifconfig_entries[i].state == "down";
            printf("Interface %d is down.\n", ID);
            return;
        }
    }
    //Needs to also update forwarding table and network table
    printf("Interface %d is not found.\n", ID);
}

void print_routes() {
    printf("Start finding routes....\n");
    int i;
    for (i = 0; i < FORWARDING_TABLE.num_entries; ++i) {
        forwarding_entry_t entry = FORWARDING_TABLE.forwarding_entries[i];
        printf("%d %d %d\n", entry.dest_addr, entry.cost, entry.next_hop);
    }
    printf("....end finding routes.\n");
}

void print_ifconfig() {
    printf("Start ifconfig....\n");
    int i;
    for (i = 0; i < IFCONFIG_TABLE.num_entries ; ++i) {
        ifconfig_entry_t entry = IFCONFIG_TABLE.ifconfig_entries[i];
        printf("%d %d %s\n", entry.interface_id, entry.source_addr, entry.state);
    }
    printf("....end ifconfig.\n");
}

void choose_command(char * command) {
    if(strcmp("ifconfig", command) == 0) {
        print_ifconfig();
    }
    else if (strcmp("route", command) == 0) {
        print_routes();
    }
    else if (strcmp("up", command) == 0) {
        //do this other thing
        set_as_up();

    }
    else if (strcmp("down", command) == 0) {
        //do this other thing
        set_as_down();
    }
    else if (strcmp("send", command) == 0) { 
        //send
        printf("send\n");
        send_packet();
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

    fd_set running_set, read_set;
    fd_set *running_ptr;

    running_ptr = & running_set;

    FD_ZERO (&running_set);
    FD_SET (1, &running_set);

    char command_line[50];

    while (1) {
    	// check for user input
    		// handle
    	// check for recieved packet
    		// handle

        read_set = running_set;

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