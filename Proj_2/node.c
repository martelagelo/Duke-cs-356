

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

int main(int argc, char ** argv) {
    // Initialize based on input file
    // setup non-blocking UDP recieve socket
    // setup non-blocking UDP send socket
    // initialize routing information
    while (1) {
    	// check for user input
    		// handle
    	// check for recieved packet
    		// handle
    }
}