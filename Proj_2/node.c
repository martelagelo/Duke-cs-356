#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

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

void send_packet() {

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
        printf("up\n");

    }
    else if (strcmp("down", command) == 0) {
        //do this other thing
        printf("down\n");
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


int init_listen_socket(int port, fd_set * running_fd_set){
    int listen_socket;
    struct sockaddr_in server_addr;

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if ((listen_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { //UDP socket listening for anything
        perror("Create socket error: ");
        exit(1);
    }

    if ((bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0) {
         perror("Bind error: ");
         exit(1);
    }

    FD_SET (listen_socket, running_fd_set);

    fcntl( listen_socket,  F_SETFL,  O_NONBLOCK, 1); // non-blocking interactions

    return listen_socket;
}

int main(int argc, char ** argv) {
    // Initialize based on input file
    


    // initialize routing information
    int listen_socket;
    fd_set running_set, read_set;
    fd_set *running_ptr;

    running_ptr = & running_set;


    FD_ZERO (&running_set);
    FD_SET (1, &running_set);

    listen_socket = init_listen_socket(7000, running_ptr);

    char command_line[50];

    while (1) {
    	// check for user input
    		// handle
    	// check for recieved packet
    		// handle

        read_set = running_set;

        if (select (FD_SETSIZE, &read_set, NULL, NULL, NULL) < 0){ 
            perror ("Select error: ");
            exit (EXIT_FAILURE);
        }

        if (FD_ISSET(1, &read_set)) {
            scanf("%s", command_line);
            //printf( "\n%s", commandLine);
            choose_command(command_line);
        }
    }
}