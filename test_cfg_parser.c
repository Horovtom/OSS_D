/*
 * File name: test.c
 * Date:      2012/09/11 13:25
 * Author:    Jan Chudoba
 *
 * This is example usage of parseRouteConfiguration() function
 *
 */

#include <stdio.h>

#include "route_cfg_parser.h"

int main(int argc, char ** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Not enough arguments\nUsage: %s <ID> [ <cfg-file-name> ]\n", argv[0]);
		return -1;
	}
	int localId = atoi(argv[1]);
	int connectionCount = 100;
	int localPort;
	TConnection connections[connectionCount];
	int result = parseRouteConfiguration((argc>2)?argv[2]:NULL, localId, &localPort, &connectionCount, connections);
	if (result) {
		printf("OK, local port: %d\n", localPort);

		if (connectionCount > 0) {
			int i;
			for (i=0; i<connectionCount; i++) {
				printf("Connection to node %d at %s%s%d\n", connections[i].id, connections[i].ip_address, (connections[i].ip_address[0])?":":"port ", connections[i].port);
			}
		} else {
			printf("No connections from this node\n");
		}
	} else {
		printf("ERROR\n");
	}
}

/* end of test.c */
