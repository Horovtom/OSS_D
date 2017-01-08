#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <vector>
#include <bits/signum.h>
#include <signal.h>
#include "route_cfg_parser.h"
#include <sys/wait.h>

#define MSG 0
#define RECEIVED 1
#define DELIVERED 2
#define UNREACHABLE 3
#define DELIVERED_BROKEN 4
#define UNREACHABLE_BROKEN 5
#define MAX_MESSAGE_LEN 1024
#define READ_END 0
#define WRITE_END 1
/**
 * This variable stores all the IDs of messages that went through this node.
 */
std::vector<int> listOfMessages;
/**
 * This variable stores pid of child...
 */

/**
 * PID of INPUT section that handles user-input
 */
int pid0;
/**
 * PID of RECV section which handles incoming messages
 */
int pid1;
/**
 * PID of SEND section, which handles outgoing messages
 */
int pid2;

/**
 * This is here in order to terminate the whole app from INPUT part
 */
void sigusr1Handler(int signalNum) {
    cerr << "Terminated..." << endl;
    int returnVal;
    wait(&returnVal);
    exit(returnVal);
}

/**
 * This is here for SEND and RECV parts of app to terminate app on error
 */
void sigusr2Handler(int signalNum) {
    cerr << "Im about to terminate app.." << endl;
    //TODO:COMPLETE
}

void communicate(int sockfd);

using namespace std;
int main(int argc, char *argv[]) {

    pid0 = ::getpid();
    int pipeFD[2];
    if (pipe(pipeFD) < 0) {
        cerr << "Error creating pipe!" << endl;
        exit(-1);
    }

    pid1 = fork();
    if (pid1 < 0 ){
        cerr << "Error forking! " << endl;
        exit(-1);
    } else if (pid1 == 0 ){
        //RECV code:
        close(pipeFD[READ_END]);
        //Registering signal that terminates the whole app
        signal(SIGUSR1, sigusr1Handler);

        recieving(pipeFD[WRITE_END]);

        //Terminate this... (Unreachable)
        close(pipeFD[WRITE_END]);
        cerr << "RECV app terminating.. " << endl;
        exit(EXIT_SUCCESS);
    } else {
        pid2 = fork();
        if (pid2 < 0) {
            cerr << "There was an error forking!" << endl;
            kill(pid1, SIGUSR1);
            wait();
            exit(-1);
        } else if (pid2 == 0) {
            //SEND code:
            close(pipeFD[WRITE_END]);
            signal(SIGUSR1, sigusr1Handler);

            sending();

            //Terminate this... (Unreachable)
            close(pipeFD[READ_END]);
            cerr << "SEND app terminating..." << endl;
            exit(EXIT_SUCCESS);
        } else {
            //INPUT code:
            close(pipeFD[READ_END]);
            signal(SIGUSR2, sigusr2Handler);
            inputing(pipeFD[WRITE_END]);

            //Terminate app (unreachable?):
            kill(pid1, SIGUSR1);
            wait();
            kill(pid2, SIGUSR1);
            wait();
            cerr << "Main app terminating.. " << endl;
            close(pipeFD[WRITE_END]);
            exit(EXIT_SUCCESS);
        }

    }



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
    sockaddr_in serv_addr, cli_addr;
    int sockFD = socket(AF_INET, SOCK_DGRAM, 0);
    int newsockFD = -1;
    if (sockFD < 0) {
        cerr << "Error opening socket..." << endl;
        exit(-1);
    }
    bzero((char * ) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((uint16_t) localPort);

    if (bind(sockFD, (struct sockaddr * ) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Error on binding" << endl;
        exit(-1);
    }

    listen(sockFD, 5);
    int clilen = sizeof(cli_addr);
    while(1) {
        newsockFD = accept(sockFD, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockFD < 0) {
            cerr << "Error on accept..." << endl;
        }

        communicate(newsockFD);

        close(newsockFD);
    }

    /*
     * General plan:
     *
     * Need it to be able to:
     *  1. process: Be ready to accept user input, communicate via pipe with sending process
     *  2. process: Sending process, be ready to accept any commands from input process or receiving process
     *  3. process: Be ready to accept messages, if its needed, send via 2. process or write to console.
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

void communicate(int sockFD) {
    int n = 0;
    char buffer[MAX_MESSAGE_LEN];
    bzero(buffer, MAX_MESSAGE_LEN);
    n = (int) read(sockFD, buffer, MAX_MESSAGE_LEN - 1);
    if (n < 0) {
        cerr << "Error reading from socket..." << endl;
    }

    string message(buffer);
    cout << message << endl;
}