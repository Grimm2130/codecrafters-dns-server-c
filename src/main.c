#include <stdio.h>
#include <stdlib.h>
#include "udp.h"							
#include "dns_server.h"							

int main( int argc, char * argv[]) {

	dns_server_main( argc, argv );
    return 0;
}

