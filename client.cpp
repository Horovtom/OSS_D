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

    /*
     * General plan:
     *
     * Message:
     *  4 words on start of message:
     *      I:  Type of message
     *          a) MSG                -> Message to some TARGET
     *          b) RECEIVED           -> Neighbour received message that node sent.
     *          c) DELIVERED          -> TARGET received message that node sent.
     *          d) UNREACHABLE        -> TARGET is unreachable by any means in the network
     *          e) DELIVERED_BROKEN   -> DELIVERED message could not trail through its PATH - Behaves like MSG
     *          f) UNREACHABLE_BROKEN -> UNREACHABLE message could not trail through its PATH - Behaves like MSG
     *      II: Message ID (currentTime)
     *      III: Path it has taken
     *      IV: Target (target node ID)
     *
     * Node:
     *
     *      $1 -> If it receives message {'MSG', 'DELIVERED_BROKEN', 'UNREACHABLE_BROKEN'} that is not meant for this node:
     *              I: Send 'RECEIVED' message to the node that is the last in the PATH
     *              II: look in my list of messages that went through here:
     *                  a) If it is in the list, do nothing.
     *                  b) Else, add the message ID into list and look at the PATH message had taken. Write 'my ID' to the PATH.
     *              IIIa: If the recipient is a neighbour:
     *                  a) Send it to him directly:
     *                  b) If he is unavailable (no 'RECEIVED'), send 'UNREACHABLE'
     *              IIIb: If the recipient is not a neighbour:
     *                  a) send copy of the message to every available neighbour (that is not written in the PATH)
     *      $2 -> If it receives message {'DELIVERED', 'UNREACHABLE'} that is not meant for this node:
     *              I: Send copy of message to neighbour that is in the PATH before my ID.
     *              II: If 'RECEIVED' did not come, send '#_BROKEN' to the target ($6)
     *      $3 -> If it receives message {'MSG'} that is meant for this node:
     *
     *      $4 -> If it receives message {'DELIVERED_BROKEN', 'UNREACHABLE_BROKEN', 'UNREACHABLE', 'DELIVERED'} that is meant for this node:
     *
     *      $5 -> If it is to send a message {'MSG'}
     *
     *      $6 -> If it is to send a message {'DELIVERED_BROKEN', 'UNREACHABLE_BROKEN'}
     *
     *
     *
     *
     */





    return 0;



}