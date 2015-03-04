#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include "ipsum.h"

using namespace std;

#define MAX_NUM_ROUTING_ENTRIES 64
#define LOCALHOST "127.0.0.1"
#define IP_ADDR_LEN 16
#define MAX_TTL 16
#define MAX_MTU_SIZE 1400
#define MAX_RECV_SIZE (1024 * 64) // 64 KB
#define TEST_PROTOCOL_VAL 0
#define RIP_PROTOCOL_VAL 200

typedef struct interface {
    int interface_id;
    char my_ip[IP_ADDR_LEN];
    uint16_t my_port;
    //char other_ip[IP_ADDR_LEN];
    //uint16_t other_port;
    char my_vip[IP_ADDR_LEN];
    char other_vip[IP_ADDR_LEN];
    int mtu_size;
    bool is_up;
    int send_socket;
} interface_t;

typedef struct forwarding_entry {
    char entry_src_addr[IP_ADDR_LEN];
    char dest_addr[IP_ADDR_LEN];
    int interface_id;
    int cost;
    time_t last_updated;
} forwarding_entry_t;

typedef struct forwarding_table {
    int num_entries;
    forwarding_entry_t forwarding_entries[MAX_NUM_ROUTING_ENTRIES];
} forwarding_table_t;

typedef struct ifconfig_table {
    int num_entries;
    interface_t ifconfig_entries[MAX_NUM_ROUTING_ENTRIES];
} ifconfig_table_t;

typedef struct rip_packet {
    uint16_t command;
    uint16_t num_entries;

    struct {
        uint32_t cost;
        uint32_t address;
    } entries[MAX_NUM_ROUTING_ENTRIES];
} rip_packet_t;

typedef struct metadata {
    uint32_t port;
    char my_ip[IP_ADDR_LEN];
} metadata_t;

/* Notes:
    rip algorithm cases: http://en.wikipedia.org/wiki/Routing_Information_Protocol
    for listening on multiple fd/sockets's simultaneously use fd_set and select() 

*/

forwarding_table_t FORWARDING_TABLE;
ifconfig_table_t IFCONFIG_TABLE;
metadata_t SELF;

void initialize_interface(interface_t * interface) {
    if ( (interface->send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("Failed to start send socket");
        exit(1);
    }
}


//Debugging utility
void print_mem(char const *vp, size_t n)
{
    char const *p = vp;
    for (size_t i=0; i<n; i++)
        printf("%02x\n", p[i]);
    putchar('\n');
};

void send_packet_with_interface(interface_t * interface, char * data, int data_size, struct iphdr * ip_header) {
    if (!interface->is_up) return;

    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = inet_addr(interface->my_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(interface->my_port);


    ip_header->tot_len = ip_header->ihl * 4 + data_size;
    ip_header->check = ip_sum((char*)ip_header, ip_header->ihl * 4);
    char full_packet[ip_header->tot_len];

    memcpy(full_packet, ip_header, ip_header->ihl*4);
    memcpy(full_packet + ip_header->ihl*4, data, data_size);

    if (sendto(interface->send_socket, full_packet, ip_header->tot_len, 0, (struct sockaddr*) &dest_addr, sizeof(dest_addr)) < 0) {
        perror("Failed to send packet");
    }
}

interface_t* get_interface_by_id(int id) {
    interface_t * temp = IFCONFIG_TABLE.ifconfig_entries;
    int i;
    for (i = 0; i< IFCONFIG_TABLE.num_entries; i++) {
        if(IFCONFIG_TABLE.ifconfig_entries[i].interface_id == id) {
            return (temp + i);
        }
    }
    return NULL;
}

interface_t* get_interface_by_dest_addr(char * dest_addr) {
    interface_t * temp = IFCONFIG_TABLE.ifconfig_entries;
    int i;
    for (i = 0; i< IFCONFIG_TABLE.num_entries; i++) {
        if(strcmp(IFCONFIG_TABLE.ifconfig_entries[i].other_vip, dest_addr) == 0) {
            return (temp + i);
        }
    }
    return NULL;
}

forwarding_entry_t* get_forwarding_entry_by_dest_addr(char * dest_addr) {
    forwarding_entry_t * temp = FORWARDING_TABLE.forwarding_entries;
    int i;
    for(i = 0; i< FORWARDING_TABLE.num_entries; i++) {
        if(strcmp(FORWARDING_TABLE.forwarding_entries[i].dest_addr, dest_addr) == 0) {
            return (temp + i);
        }
    }
    return NULL;
}

/**
* Creates an ifconfig entry and puts it into the ifconfig table
**/
void create_ifconfig_entry(int ID, uint16_t port, char *myIP, char *myVIP, char *otherVIP) {
    interface_t entry;
    entry.interface_id = ID;
    entry.my_port = port;
    strcpy(entry.my_ip,myIP);
    strcpy(entry.my_vip,myVIP);
    strcpy(entry.other_vip,otherVIP);
    entry.is_up = true;
    entry.mtu_size = MAX_MTU_SIZE;

    initialize_interface(&entry);
    IFCONFIG_TABLE.ifconfig_entries[ID] = entry;
    IFCONFIG_TABLE.num_entries++;
}


//TODO: make timing work
void update_forwarding_entry(char * src_addr, char * next_addr, char * dest_addr, int cost) {
    forwarding_entry_t * entry = get_forwarding_entry_by_dest_addr(dest_addr);
    interface_t * interface = get_interface_by_dest_addr(next_addr);
    //int current_entry_index = FORWARDING_TABLE.num_entries;

    if (entry == NULL) {
        if (interface == NULL) {
            FORWARDING_TABLE.forwarding_entries[FORWARDING_TABLE.num_entries].interface_id = -1;
        }
        else {
            FORWARDING_TABLE.forwarding_entries[FORWARDING_TABLE.num_entries].interface_id = interface-> interface_id;
        }

        if(cost == MAX_TTL) {
            FORWARDING_TABLE.forwarding_entries[FORWARDING_TABLE.num_entries].cost = MAX_TTL;
        }
        else {
            FORWARDING_TABLE.forwarding_entries[FORWARDING_TABLE.num_entries].cost = cost + 1;
        }

        strcpy(FORWARDING_TABLE.forwarding_entries[FORWARDING_TABLE.num_entries].entry_src_addr, src_addr);
        strcpy(FORWARDING_TABLE.forwarding_entries[FORWARDING_TABLE.num_entries].dest_addr, dest_addr);
        FORWARDING_TABLE.forwarding_entries[FORWARDING_TABLE.num_entries].last_updated = time(NULL);
        FORWARDING_TABLE.num_entries++;
    }
    else if((entry -> cost > (cost + 1)) & (entry -> interface_id != -1)) {
        strcpy(entry -> entry_src_addr, src_addr);
        entry -> interface_id = interface -> interface_id;
        entry -> cost = cost + 1;
        entry -> last_updated = time(NULL);
    }
    else if((strcmp(entry -> dest_addr, dest_addr) == 0) & (entry -> cost == (cost + 1))) {
        entry -> last_updated = time(NULL);
    } 

}

void build_tables(FILE *fp) {
    int ID;
    char other_port[IP_ADDR_LEN], other_vip[IP_ADDR_LEN], my_vip[IP_ADDR_LEN], myIP[IP_ADDR_LEN];
    uint16_t port;

    ID = 0;
    while(feof(fp) == false) {
        fscanf(fp, "%s %s %s", other_port, my_vip, other_vip);
        
        strcpy(myIP, strtok (other_port,":"));
        if(strcmp(myIP, "localhost")==0) {
            strcpy(myIP, LOCALHOST);
        } 
        port = atoi(strtok (NULL,": "));
        create_ifconfig_entry(ID, port, myIP, my_vip, other_vip);

        update_forwarding_entry((char *) LOCALHOST, (char *) other_vip, (char *) other_vip, 0);
        update_forwarding_entry((char *) LOCALHOST, (char *) LOCALHOST, (char *) my_vip, -1);

        ID++;

    }
}

void load_from_file() {

    char content[2000], file_name[25];
    FILE *fp;
    printf("Enter file name you wish to upload\n");
    gets(file_name);
    //printf("%s\n", file_name);

    fp = fopen(file_name,"r");

    if( fp == NULL ) {
      perror("Error while opening the file.\n");
      exit(EXIT_FAILURE);
    }

    printf("The contents of %s file are :\n", file_name);

    fscanf(fp, "%s", content);
    printf("%s\n", content);

    strcpy(SELF.my_ip,strtok (content,":"));
    SELF.port = atoi(strtok (NULL,": "));

    printf("myIP: %s\nmyPort: %d\n", SELF.my_ip, (int) SELF.port);

    build_tables(fp);
    
   fclose(fp);
}

// bool is_dest_equal_to_me(char * dest_addr) {
//     interface_t * temp = IFCONFIG_TABLE.ifconfig_entries;
//     int i;
//     for (i = 0; i< IFCONFIG_TABLE.num_entries; i++) {
//         if(strcmp(IFCONFIG_TABLE.ifconfig_entries[i].my_vip, dest_addr)) {
//             return true;
//         }
//     }
//     return false;
// }

void send_packet(char * dest_addr, char * msg, int msg_size, int TTL, int protocol) {
    forwarding_entry_t *f_entry;
    interface_t * interface;

    f_entry = get_forwarding_entry_by_dest_addr(dest_addr);
    if (f_entry == NULL) {
        printf("\nPath does not exist in forwarding table.");
        return;
    }

    interface = get_interface_by_id(f_entry -> interface_id);
    if (interface == NULL) {
        printf("Path does not exist in ifconfig table.\n");
        return;
    }

    char packet[MAX_MTU_SIZE];
    memset(&packet[0], 0, sizeof(packet));

    struct iphdr * ip_header = (struct iphdr*) packet;

    ip_header -> id = rand();
    ip_header -> saddr = inet_addr(interface->my_vip);
    ip_header -> daddr = inet_addr(f_entry -> dest_addr);
    ip_header -> version = 4;
    ip_header -> ttl = TTL;
    ip_header -> protocol = protocol;
    ip_header -> ihl = 5;

    ip_header -> check = 0;
    ip_header -> tot_len = 0;
    ip_header -> frag_off = 0;


    send_packet_with_interface(interface, msg, msg_size, ip_header);
    return;
}


void set_as_up(int ID) {
    interface_t * interface = get_interface_by_id(ID);
    if (interface == NULL) {
        printf("\nInterface %d is not found.\n\n", ID);
        return;
    }
    interface->is_up = true;
    printf("\nInterface %d is up.\n\n", ID);
    return;
}

void set_as_down(int ID) {
    interface_t * interface = get_interface_by_id(ID);
    if (interface == NULL) {
        printf("\nInterface %d is not found.\n\n", ID);
        return;
    }
    interface->is_up = false;
    printf("\nInterface %d is down.\n\n", ID);
    return;
}

void print_routes() {
    printf("\nStart finding routes....\n");
    int i;
    for (i = 0; i < FORWARDING_TABLE.num_entries; ++i) {
        forwarding_entry_t entry = FORWARDING_TABLE.forwarding_entries[i];
        printf("%s %d %d\n", entry.dest_addr, entry.interface_id, entry.cost);
    }
    printf("....end finding routes.\n\n");
}

void print_ifconfig() {
    printf("\nStart ifconfig....\n");
    int i;
    for (i = 0; i < IFCONFIG_TABLE.num_entries ; ++i) {
        interface_t entry = IFCONFIG_TABLE.ifconfig_entries[i];
        printf("%d %s %s\n", entry.interface_id, entry.my_vip, entry.is_up ? "up" : "down");
    }
    printf("....end ifconfig.\n\n");
}

/**
* Sends an RIP update to a specified destination
**/
void send_forwarding_update(char * dest_addr) {
    //refresh_routes();

    rip_packet_t * RIP_packet = (rip_packet_t *) malloc(sizeof(rip_packet_t *));
    RIP_packet -> command = 2; //Sends a response RIP packet
    RIP_packet -> num_entries = FORWARDING_TABLE.num_entries;

    int i;
    for(i = 0; i < FORWARDING_TABLE.num_entries; i++) {
        RIP_packet -> entries[i].address = inet_addr(FORWARDING_TABLE.forwarding_entries[i].dest_addr);
        if(strcmp(dest_addr, FORWARDING_TABLE.forwarding_entries[i].entry_src_addr) == 0) {
            RIP_packet->entries[i].cost = MAX_TTL - 1;
        }
        else {
            RIP_packet-> entries[i].cost = FORWARDING_TABLE.forwarding_entries[i].cost;
        }
    }
    send_packet(dest_addr, (char *) RIP_packet, sizeof(rip_packet_t), MAX_TTL, RIP_PROTOCOL_VAL);
}

void activate_RIP_update() {
    int i;
    for(i = 0; i <IFCONFIG_TABLE.num_entries; i++) {
        if(IFCONFIG_TABLE.ifconfig_entries[i].is_up) {
            send_forwarding_update(IFCONFIG_TABLE.ifconfig_entries[i].other_vip);
        }
    }
} 

void request_routes() {
    int i;
    for(i = 0; i < IFCONFIG_TABLE.num_entries; i ++ ) {
        rip_packet_t* RIP_packet = (rip_packet_t *) malloc(sizeof(rip_packet_t *));
        RIP_packet -> command = 1;
        RIP_packet -> num_entries = 0;

        send_packet(IFCONFIG_TABLE.ifconfig_entries[i].other_vip, (char *) RIP_packet, sizeof(rip_packet_t), MAX_TTL, RIP_PROTOCOL_VAL);
    }
}

void check_for_expired_routes() {
    forwarding_entry_t * forwarding_entries = FORWARDING_TABLE.forwarding_entries;
    
    int i;
    for(i = 0; i < FORWARDING_TABLE.num_entries; i += 1){
        if (forwarding_entries[i].interface_id != -1 & ((int) time(NULL) - (int) forwarding_entries[i].last_updated > 12)) {
            printf("found expired entry!");
            forwarding_entries[i].cost = 16; // entry expired
            forwarding_entries[i].last_updated = time(NULL);
        }
    }
}

void choose_command(char * command) {
    char temp_char;
    int ID;
    if(strcmp("ifconfig", command) == 0) {
        print_ifconfig();
    }
    else if (strcmp("routes", command) == 0) {
        print_routes();
    }
    else if (strcmp("up", command) == 0) {
        scanf("%d", &ID);
        set_as_up(ID);
        activate_RIP_update();
    }
    else if (strcmp("down", command) == 0) {
        scanf("%d", &ID);
        set_as_down(ID);
        activate_RIP_update();
    }
    else if (strcmp("send", command) == 0) { 
        char dest_addr[20], msg[MAX_MTU_SIZE];
        scanf("%s %[^\n]s", dest_addr, msg);
        printf("destination: %s message: %s", dest_addr, msg);
        send_packet(dest_addr, msg, strlen(msg), MAX_TTL, TEST_PROTOCOL_VAL);
    }
    else if (strcmp("die", command) == 0) { 
        printf("....*BANG*-*clatter*-*thud*.......\n");
        exit(0);
    }
    else {
        printf("\nCommand not found. Please enter a different command.\n");
    }
    while ((temp_char = getchar()) != '\n' && temp_char != EOF); // clear stdin buffer
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

    fcntl(listen_socket,  F_SETFL,  O_NONBLOCK, 1); // non-blocking interactions

    return listen_socket;
}

void handle_packet(int listen_socket) {
    char recv_buffer[MAX_RECV_SIZE];
    struct iphdr * recv_header;
    char * recv_data_ptr;
    char recv_data_buffer[MAX_RECV_SIZE];
    int received_ip_checksum, calculated_ip_checksum;

    memset(&recv_buffer[0], 0, (MAX_RECV_SIZE * sizeof(char)));

    recv(listen_socket, recv_buffer, MAX_RECV_SIZE, 0);
    printf("Received Packet\n");
    recv_header = (struct iphdr *) recv_buffer;
    recv_data_ptr = (recv_buffer + recv_header->ihl * 4);


    received_ip_checksum = recv_header->check;
    recv_header->check = 0;
    calculated_ip_checksum = ip_sum((char *) recv_header, recv_header->ihl * 4);

    if(received_ip_checksum != calculated_ip_checksum){ 
        printf("Broken checksum, dropping packet\n");
        return;
    }

    if(recv_header->ttl <= 0) {
        printf("TTL surpassed, dropping packet\n");
        return;
    }

    char src_addr[IP_ADDR_LEN];
    inet_ntop(AF_INET, &(recv_header->saddr), src_addr, INET_ADDRSTRLEN);

    memset(&recv_data_buffer[0], 0, (MAX_RECV_SIZE));
    memcpy(recv_data_buffer, recv_data_ptr, MAX_RECV_SIZE - recv_header->ihl * 4);

    if(recv_header->protocol == TEST_PROTOCOL_VAL){
        //if(isMe(dest_addr) < 0){ // not in the table, need to forward
        //    int df_bit = (recv_ip->frag_off & IP_DF) == IP_DF;
        //    send_packet(dest_addr, payload, strlen(payload), df_bit, (recv_ip->ttl) - 1, recv_ip->protocol, recv_ip->ihl); // decrement ttl by 1
        //}
        //else{
            printf("Recieved Message: %s\n", recv_data_buffer);
        //}
    } 
    else if (recv_header->protocol == RIP_PROTOCOL_VAL) {
        rip_packet_t * RIP_packet = (rip_packet_t *) recv_data_buffer;
        if(RIP_packet -> command == 1) {
            send_forwarding_update(src_addr);
        }
        else if(RIP_packet -> command == 2) {
            int i;
            printf("Updating %d forwarding entries", RIP_packet->num_entries);
            for(i = 0; i < RIP_packet -> num_entries; i++ ) {
                char dest[IP_ADDR_LEN];
                inet_ntop(AF_INET, &(RIP_packet->entries[i].address), dest, INET_ADDRSTRLEN);
                update_forwarding_entry(src_addr, src_addr, dest, RIP_packet->entries[i].cost);
            }
        }
    }
    else {
        printf("Got something other than test or RIP protocol: %s\n", recv_data_buffer);
    }
}

int main(int argc, char ** argv) {
    // Initialize based on input file
    
    load_from_file();
    // initialize routing information
    int listen_socket;
    fd_set full_fd_set;
    fd_set *running_ptr;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    time_t last_updated;
    time(&last_updated);

    running_ptr = & full_fd_set;
    listen_socket = init_listen_socket(SELF.port, running_ptr);

    char command_line[1500];

    request_routes();

    while (1) {
        FD_ZERO (running_ptr);
        FD_SET (STDIN_FILENO, running_ptr);
        FD_SET (listen_socket, running_ptr);

    	// check for user input
    		// handle
    	// check for received packet
    		// handle

        if (select (FD_SETSIZE, running_ptr, NULL, NULL, &timeout) < 0){ 
            perror ("Select error: ");
            exit (EXIT_FAILURE);
        }

        if (FD_ISSET (listen_socket, running_ptr)){ // data ready on the read socket
            //TODO: receive data and pass directly to ALL interfaces
                // Only an up and directly attached interface (by source port) should act on this and call handle_packet
            handle_packet(listen_socket);
        }

        if (FD_ISSET(STDIN_FILENO, running_ptr)) {
            scanf("%s", command_line);
            //fgets(command_line, 100, stdin);
            //printf( "\n%s", commandLine);
            choose_command(command_line);
            fflush(STDIN_FILENO);
        }

        if ( ((int)time(NULL)-(int)last_updated) >= 5) {
            request_routes();
            time(&last_updated);
            //printf("been 5 seconds\n");
        }
        check_for_expired_routes();
    }
}
