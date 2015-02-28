#include <stdint.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

using namespace std;

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

Interface::Interface (int a, char b[], int c, char d[], int e, char f[], char g[]) {
    id = a;
    strcpy(b, my_ip);
    //my_ip = b;
    my_port = c;
    strcpy(d, other_ip);
    //other_ip = d;
    other_port = e;
    strcpy(f, my_vip);
    strcpy(g, other_vip);
    //my_vip = f;
    //other_vip = g;

    is_up = true;
    mtu_size = 1400;

    if ( (send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("Failed to start send socket");
        exit(1);
    }
}

void Interface::send_packet(char * data, struct iphdr * ip_header) {
	struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = inet_addr(other_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(other_port);

    if (sendto(send_socket, data, ip_header->tot_len, 0, (struct sockaddr*) &dest_addr, sizeof(dest_addr)) == -1) {
        perror("Failed to send packet");
    }
}