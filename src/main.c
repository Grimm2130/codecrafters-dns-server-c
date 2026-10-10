#include <stdio.h>
#include <stdlib.h>
#include "udp.h"							
#include "dns_server.h"							

int main( int argc, char * argv[]) {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
    printf("Logs from your program will appear here!\n");

    // TODO: Uncomment the code below to pass the first stage
	// RunServer();
	char *resolver_addr = NULL;

	if(argc == 3 && (strcmp( argv[1], "--resolver" ) == 0 ))
	{
		resolver_addr = argv[2];
	}
	
	dns_service_start( resolver_addr );

	dns_service_stop();
 
    return 0;
}

