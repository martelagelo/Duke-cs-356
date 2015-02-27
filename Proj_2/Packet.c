#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

struct Packet {
	uint32_t sourceAddress;
	uint32_t destAddress;
	int totalLength;
	int TTL;
	int checksum;
	char message[1400];
}