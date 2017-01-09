//
// Created by lactosis on 9.1.17.
//

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <bits/signum.h>
#include <signal.h>
#include "route_cfg_parser.h"
#include <sys/wait.h>
#include <time.h>
#include <cstring>
#include <map>

/**
 * Message to some TARGET
 */
#define MSG 0
/**
 * Neighbour received message that node sent.
 */
#define RECEIVED 1
/**
 * TARGET received message that node sent.
 */
#define DELIVERED 2
/**
 * TARGET is unreachable by any means in the network
 */
#define UNREACHABLE 3
/**
 * DELIVERED message could not trail through its PATH - Behaves like MSG
 */
#define DELIVERED_BROKEN 4
/**
 * UNREACHABLE message could not trail through its PATH - Behaves like MSG
 */
#define UNREACHABLE_BROKEN 5
#define MAX_MESSAGE_LEN 1024
#define READ_END 0
#define WRITE_END 1
/**
 * Index of word in header that specifies TYPE of message...
 * e.g. RECIEVED, DELIVERED, MSG etc.
 */
#define HEADER_TYPE 0
/**
 * Index of a word in header that specifies the ID of message
 */
#define HEADER_ID 1
/**
 * Index of a word in header that specifies PATH this message has taken
 */
#define HEADER_PATH 2
/**
 * Index of a sectiom of header, that specifies the target (recipient of message)
 */
#define HEADER_TARGET 3

using namespace std;

/**
 * This variable stores all the IDs of messages that went through this node.
 */
std::vector<int> listOfMessages;
int localID;
int localPort;
int connectionCount;
TConnection connections[100];
sockaddr_in serv_addr;
/**
 * This is a map of <ID, net address> of all the neighbours
 */
map<int, sockaddr_in> neighbours;
int sockFD;
mutex m;
condition_variable cv;
condition_variable cv2;

/**
 * Class that contains info that is shared among processes
 */
class Dataholder{
private:
    string message;
    bool readyToRead;
    bool readyToWrite;
public:
    Dataholder(void) {
        readyToRead = false;
        readyToWrite = true;
    }
    bool isReadyToRead() {
        return readyToRead;
    }
    bool isReadyToWrite() {
        return readyToWrite;
    }
    void setReadyToWrite() {
        readyToWrite = true;
        readyToRead = false;
        cv2.notify_one();
    }
    void setReadyToRead() {

        readyToRead = true;
        readyToWrite = false;
        cv.notify_one();
    }
    string read() {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [&]{return isReadyToRead();});

        if (!readyToRead) {
            cerr << "Somebody is trying to read, when he does not have the right to do so!" << endl;
            return NULL;
        } else {
            return message;
        }
    }
    void write(string toWrite) {
        unique_lock<mutex> lk(m);
        cv2.wait(lk, [&]{return isReadyToWrite();});
        if (!readyToWrite) {
            cerr << "Somebody is trying to write when he does not have the right to!" << endl;
        } else {
            message = toWrite;
        }

    }
};

void testing();

Dataholder mailbox;



// region PART SEND


/**
 * Má na starosti odesílání message
 */
void partSend() {
    cout << mailbox.read() << endl;
    mailbox.setReadyToWrite();
    cout << mailbox.read() << endl;
    mailbox.setReadyToWrite();
}

// endregion

//region PART RECEIVE

/**
 * Má na starosti obsluhu příchozích message na pozadí
 */
void partReceive() {
    mailbox.write("Ahoj, já jsem RECIEVE");
    mailbox.setReadyToRead();
}

//endregion

//region PART INPUT

/**
 * Má na starosti obsluhu uživatelského inputu
 */
void partInput() {
    mailbox.write("Ahoj, já jsem INPUT");
    mailbox.setReadyToRead();
}

//endregion


int main(int argc, char * argv[]) {
    if (argc!=3) {
        cerr << "Wrong number of arguments! \nUsage: " << argv[0] << " <ID> <cfg file>" << endl;
        cerr << "Launching in testing mode..." << endl;
        testing();
        return -1;
    }
    localID = atoi(argv[1]);
    if (localID == 0) {
        cerr << "Invalid ID: " << argv[1] << endl;
        exit(-1);
    }

    connectionCount = 100;
    int result = parseRouteConfiguration(argv[2], localID, &localPort, &connectionCount, connections);
    if (result) {
        cout << "Configuration file loaded..." << endl;
        cout << "   Local port: " << localPort << endl;
        //Printing connections:
        if (connectionCount > 0) {
            for (int i = 0; i < connectionCount; ++i) {
                cout << "   Connection to the node " << connections[i].id << " at " <<
                     connections[i].ip_address << (connections[i].ip_address[0] ? ":" : "port ") <<
                     connections[i].port << endl;
            }
        } else {
            cout << "No connections to this node!" << endl;
            cerr << "Terminating..." << endl;
            exit(0);
        }
    } else {
        cerr << "Error parsing file!" << endl;
        exit(-1);
    }
    cout << "Starting client with ID " << argv[1] << endl;

    if ((sockFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        cerr << "Failed to create socket" << endl;
        exit(EXIT_FAILURE);
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons((uint16_t) localPort);

    //Bind socket to port:
    if (bind(sockFD, (struct sockaddr * ) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Unable to bind socket to port!" << endl;
        exit(-1);
    }

    for (int j = 0; j < connectionCount; ++j) {
        sockaddr_in toAdd;
        toAdd.sin_family = AF_INET;
        if (!connections[j].ip_address[0]) {
            connections[j].ip_address[0] = '1';
            cout << connections[j].ip_address << endl;
        }
        toAdd.sin_port = htons((uint16_t) connections[j].port);
        neighbours[connections[j].id] = toAdd;
    }

    //Neighbours loaded...




    //Split into 3 parts SEND, RECV, INPUT:
    thread send(partSend);
    thread receive(partReceive);
    thread input(partInput);
    cout << "Application successfully created all threads" << endl;

    //Now join everything:
    send.join();
    receive.join();
    input.join();
    cout << "Application succesfully stopped all threads" << endl;
}

void testing() {
    //TODO: Insert tests here!
}
