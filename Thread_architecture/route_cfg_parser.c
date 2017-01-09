/*
 * File name: route_cfg_parser.c
 * Date:      2012/09/11 11:50
 * Author:    Jan Chudoba
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "route_cfg_parser.h"

#define DEFAULT_CFG_FILE_NAME "routing.cfg"
#define MAX_LINE_SIZE 255

int parse_word(char ** str, char * result, int maxLength)
{
	int length = 0;
	char c;
	while ((c=**str) != 0) {
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			if (length > 0) {
				result[length] = 0;
				return 1;
			}
		} else {
			if (length < maxLength-1) {
				result[length++] = c;
			}
		}
		(*str)++;
	}
	result[length] = 0;
	return (length > 0);
}

int parseRouteConfiguration(const char * fileName, int localId, int * localPort, int * connectionCount, TConnection * connections)
{
	int maxConnections = *connectionCount;
	*connectionCount = 0;
	TConnection allConnections[maxConnections];
	int allConnectionCount = 0;

	int result;
	if (!fileName) fileName = DEFAULT_CFG_FILE_NAME;
	if (localPort) *localPort = -1;
	FILE * f = fopen(fileName, "r");
	if (f) {
		result = 1;
		char line[MAX_LINE_SIZE+1];
		while (!feof(f) && fgets(line, MAX_LINE_SIZE, f)) {
			int len = strlen(line);
			if (len > 0 && line[len-1] == '\n') { len--; line[len] = 0; }
			char * l = line;
			const int tmp_size = 20;
			char tmp[tmp_size];
			if (parse_word(&l, tmp, tmp_size)) {
				if (strcmp(tmp, "node") == 0) {
					TConnection c;
					if (parse_word(&l, tmp, tmp_size)) {
						c.id = atoi(tmp);
					} else c.id = 0;
					if (c.id == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid ID '%s' on line '%s'\n", tmp, line);
						result = 0;
					}
					if (parse_word(&l, tmp, tmp_size)) {
						c.port = atoi(tmp);
					} else c.port = 0;
					if (c.port == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid port '%s' on line '%s'\n", tmp, line);
						result = 0;
					}
					parse_word(&l, c.ip_address, IP_ADDRESS_MAX_LENGTH);
					//fprintf(stderr, "CFG_PARSER: [DEBUG] ID=%d, port=%d, IP=%s\n", c.id, c.port, c.ip_address);
					allConnections[allConnectionCount++] = c;
					if (c.id == localId && localPort != NULL) {
						*localPort = c.port;
					}
				} else if (strcmp(tmp, "link") == 0) {
					int id_client, id_server;
					if (parse_word(&l, tmp, tmp_size)) {
						id_client = atoi(tmp);
					} else id_client = 0;
					if (id_client == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid client ID '%s' on line '%s'\n", tmp, line);
						result = 0;
					}
					if (parse_word(&l, tmp, tmp_size)) {
						id_server = atoi(tmp);
					} else id_server = 0;
					if (id_server == 0) {
						fprintf(stderr, "CFG_PARSER: [ERROR] invalid server ID '%s' on line '%s'\n", tmp, line);
						result = 0;
					}
					//fprintf(stderr, "CFG_PARSER: [DEBUG] LINK %d -> %d\n", id_client, id_server);
					if (id_client == localId) {
						int i;
						for (i=0; i<allConnectionCount; i++) {
							if (allConnections[i].id == id_server) {
								connections[(*connectionCount)++] = allConnections[i];
							}
						}
					}
				} else {
					fprintf(stderr, "CFG_PARSER: [ERROR] invalid configuration line '%s'\n", line);
					result = 0;
				}
			}
		}
		fclose(f);
	} else {
		fprintf(stderr, "CFG_PARSER: [ERROR] can not open file '%s'\n", fileName);
		result = 0;
	}
	//fprintf(stderr, "CFG_PARSER: [DEBUG] parsed %d nodes and %d own links\n", allConnectionCount, *connectionCount);
	return result;
}

/* end of route_cfg_parser.c */
