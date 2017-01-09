/*
 * File name: route_cfg_parser.h
 * Date:      2012/09/11 13:16
 * Author:    Jan Chudoba
 */




#ifndef __ROUTE_CFG_PARSER_H__
#define __ROUTE_CFG_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#define IP_ADDRESS_MAX_LENGTH 16

typedef struct {
    int id;
    int port;
    char ip_address[IP_ADDRESS_MAX_LENGTH];
} TConnection;


/**
 * parseRouteConfiguration()
 *
 * Read table of node connections from the configuration file
 *
 *   fileName        - file name, when NULL, default file name "routing.cfg" will be considered
 *   localId         - ID of this node
 *   localPort       - pointer to int, which will be set to local node port number (port needed to be opened)
 *   connectionCount - pointer to int, which will contain size of array 'connections';
 *                     after function call, it will contain the real number of connections stored in array
 *   connections     - array of TConnection structures, containing connection information after function call
 *
 */
int parseRouteConfiguration(const char *fileName, int localId, int *localPort, int *connectionCount,
                            TConnection *connections);


#ifdef __cplusplus
}
#endif


#endif

/* end of route_cfg_parser.h */
