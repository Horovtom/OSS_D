#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include "route_cfg_parser.h"

using namespace std;
int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Wrong number of arguments! \nUsage: ./node <ID> <cfg file>" << endl;
        return -1;
    }
    int localID = atoi(argv[1]);
    if (localID == 0) {
        cerr << "Invalid ID: " << argv[1] << endl;
        exit(-1);
    }
    int localPort;
    int connectionCount = 100;
    TConnection connections[connectionCount];
    int result = parseRouteConfiguration(argv[2], localID, &localPort, &connectionCount, connections);
    if (result) {
        cout << "Configuration file loaded..." << endl;
        cout << "Local port: " << localPort << endl;
        //Testing log
        if (connectionCount > 0) {
            for (int i = 0; i < connectionCount; ++i) {
                cout << "Connection to the node " << connections[i].id << " at " <<
                     connections[i].ip_address << (connections[i].ip_address[0] ? ":":"port ") <<
                     connections[i].port << endl;
            }
        } else {
            cout << "No connections from this node!" << endl;
        }
    } else {
        cerr << "Error parsing file!" << endl;
        exit(-1);
    }
    cout << "Starting client with ID " << argv[1]  << endl;





    return 0;



}