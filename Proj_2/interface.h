#include <stdint.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

class Interface {
public:
    int id; // my interface ID
    char my_ip[20]; // usually going to be 127.0.0.1
    uint16_t my_port; // port of otherination (init file)
    char other_ip[20]; // usually going to be 127.0.0.1
    uint16_t other_port; // port of otherination (init file)
    char my_vip[20]; // my VIP (in init file)
    char other_vip[20]; // VIP of interface on other end of node (in init file)

    bool is_up;
    int mtu_size;

    int send_socket;

    Interface(int, char [], int, char [], int, char [], char []);
    void send_packet(char *, struct iphdr *);
};


Interface::Interface (int id_in, char my_ip_in[], int my_port_in, char other_ip_in[], int other_port_in, char my_vip_in[], char other_vip_in[]) {
    id = id_in;
    strcpy(my_ip_in, my_ip);
    //my_ip = my_ip_in;
    my_port = my_port_in;
    strcpy(other_ip_in, other_ip);
    //other_ip = other_ip_in;
    other_port = other_port_in;
    strcpy(my_vip_in, my_vip);
    strcpy(other_vip_in, other_vip);
    //my_vip = my_vip_in;
    //other_vip = other_vip_in;

    is_up = true;
    mtu_size = 1400;

    if ( (send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("Failed to start send socket");
        exit(1);
    }
}

void Interface::send_packet(char * data, struct iphdr * ip_header) {
    if (!is_up) return;

    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = inet_addr(other_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(other_port);

    if (sendto(send_socket, data, ip_header->tot_len, 0, (struct sockaddr*) &dest_addr, sizeof(dest_addr)) < 0) {
        perror("Failed to send packet");
    }
}