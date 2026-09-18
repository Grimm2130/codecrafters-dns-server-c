#include <stdio.h>
#include <stdlib.h>
#include "udp.h"							
#include "dns_server.h"							

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
    printf("Logs from your program will appear here!\n");

    // TODO: Uncomment the code below to pass the first stage
	// RunServer();

	dns_service_start();

	dns_service_stop();
 
    return 0;
}

