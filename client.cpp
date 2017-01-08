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
 */
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
 *
 */
#define HEADER_TARGET 3

using namespace std;

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
 * ID of this NODE
 */
int localID;
int localPort;
int connectionCount;
TConnection connections[100];
sockaddr_in serv_addr, cli_addr;
int sockFD;

/**
 * This is here in order to terminate the whole app...
 */
void sigusr1Handler(int signalNum) {
    cerr << "Terminated..." << endl;
    int returnVal = 0;
    if (kill(pid1, SIGUSR1) != 0) returnVal = -1;
    wait(NULL);
    if (kill(pid2, SIGUSR1) != 0) returnVal = -1;
    wait(NULL);
    exit(returnVal);
}

///**
// * This is here for SEND and RECV parts of app to terminate app on error
// */
//void sigusr2Handler(int signalNum) {
//    cerr << "Im about to terminate app.." << endl;
//    kill(pid1)
//}

void communicate(int sockfd);

void inputing(int writeFD);

void sending(int readFD);

void recieving(int writeFD);

void startForking();

void parseMessage(string message, int fileDescriptor);

void sendReceived(int fileDescriptor);

int generateMessageID();

int getIDFromPath(string PATH, int position);

bool isInPath(int id, string PATH);

string addMyIDToPath(string PATH);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Wrong number of arguments! \nUsage: ./node <ID> <cfg file>" << endl;
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
        cout << "Local port: " << localPort << endl;
        //Testing log
        if (connectionCount > 0) {
            for (int i = 0; i < connectionCount; ++i) {
                cout << "Connection to the node " << connections[i].id << " at " <<
                     connections[i].ip_address << (connections[i].ip_address[0] ? ":" : "port ") <<
                     connections[i].port << endl;
            }
        } else {
            cout << "No connections from this node!" << endl;
        }
    } else {
        cerr << "Error parsing file!" << endl;
        exit(-1);
    }
    cout << "Starting client with ID " << argv[1] << endl;

    sockFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFD < 0) {
        cerr << "Error opening socket.. " << endl;
        exit(-1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((uint16_t) localPort);

    //This might not work with fork...
    if (bind(sockFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Error on binding" << endl;
    }

    startForking();
    exit(0);









//    if (sockFD < 0) {
//        cerr << "Error opening socket..." << endl;
//        exit(-1);
//    }
//    bzero((char * ) &serv_addr, sizeof(serv_addr));
//    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_addr.s_addr = INADDR_ANY;
//    serv_addr.sin_port = htons((uint16_t) localPort);
//
//    if (bind(sockFD, (struct sockaddr * ) &serv_addr, sizeof(serv_addr)) < 0) {
//        cerr << "Error on binding" << endl;
//        exit(-1);
//    }

//    listen(sockFD, 5);
//    socklen_t clilen = sizeof(cli_addr);
//    while(1) {
//        newsockFD = accept(sockFD, (struct sockaddr *) &cli_addr, &clilen);
//        if (newsockFD < 0) {
//            cerr << "Error on accept..." << endl;
//        }
//
//        communicate(newsockFD);
//
//        close(newsockFD);
//    }







    return 0;


}

void startForking() {
    pid0 = ::getpid();
    int pipeFD[2];
    if (pipe(pipeFD) < 0) {
        cerr << "Error creating pipe!" << endl;
        exit(-1);
    }

    pid1 = fork();
    if (pid1 < 0) {
        cerr << "Error forking! " << endl;
        exit(-1);
    } else if (pid1 == 0) {
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
            wait(NULL);
            exit(-1);
        } else if (pid2 == 0) {
            //SEND code:
            close(pipeFD[WRITE_END]);
            signal(SIGUSR1, sigusr1Handler);

            sending(pipeFD[READ_END]);

            //Terminate this... (Unreachable)
            close(pipeFD[READ_END]);
            cerr << "SEND app terminating..." << endl;
            exit(EXIT_SUCCESS);
        } else {
            //INPUT code:
            close(pipeFD[READ_END]);
            signal(SIGUSR1, sigusr1Handler);
            inputing(pipeFD[WRITE_END]);

            //Terminate app (unreachable?):
            kill(pid1, SIGUSR1);
            wait(NULL);
            kill(pid2, SIGUSR1);
            wait(NULL);
            cerr << "Main app terminating.. " << endl;
            close(pipeFD[WRITE_END]);
            exit(EXIT_SUCCESS);
        }

    }

}

/*
 * This section acts like a server
 */
void recieving(int writeFD) {
    int newSockFD = -1;
    listen(sockFD, 5);
    socklen_t clilen = sizeof(cli_addr);
    while (1) {
        newSockFD = accept(sockFD, (struct sockaddr *) &cli_addr, &clilen);
        if (newSockFD < 0) {
            cerr << "Error on accept... " << endl;
        }
        string message;
        char ch;
        while (read(newSockFD, &ch, 1) > 0) {
            if (ch != 0) message.push_back(ch);
            else {
                message.push_back('\n');
                break;
            }
        }
        parseMessage(message, newSockFD, writeFD);

    }
}

/**
 * Decides what to do by the header of the message.
 */
void parseMessage(string message, int fileDescriptor, int pipeDescriptor) {
    string header[4];
    string text;
    int currChar = 0;
    for (int i = 0; i < 4; ++i) {
        while (message[currChar] != ' ') {
            header[i].push_back(message[currChar]);
            currChar++;
        }
    }
    while (message[currChar] == ' ') currChar++;
    while (currChar < message.length()) text.push_back(message[currChar]);
    //Loaded:

    //If this message had already been here
    for (int j = 0; j < listOfMessages.size(); ++j) {
        if (listOfMessages[j] == atoi(header[HEADER_ID].c_str())) return;
    }
    //Add it to the list of messages
    listOfMessages.push_back(atoi(header[HEADER_ID].c_str()));

    switch (atoi(header[HEADER_TYPE].c_str())) {
        case MSG:
            sendReceived(fileDescriptor);
            if (atoi(header[HEADER_TARGET].c_str()) == localID) {
                //This message was for me... Print it to cout
                int from = getIDFromPath(header[HEADER_PATH], 0);
                cout << "Message from: " << from << endl;
                cout << text;
            } else {
                //This message was not for me... Hubbing it
                //Adding my ID to its PATH:
                header[HEADER_PATH] = addMyIDToPath(header[HEADER_PATH]);

                string index = "";
                for (int i = 0; i < connectionCount; ++i) {
                    if (!isInPath(connections[i].id, header[HEADER_PATH])) {
                        //Fill string of messages:
                        index.append(to_string(i)).append(";");
                    }
                }
                //  We sent a string which looks like this:
                //  1;4;23;15;
                write(pipeDescriptor, index.c_str(), index.length() + 1);

                string newDocument = "";
                newDocument.append(header[HEADER_TYPE]).append(" ").append(header[HEADER_ID]).append(" ").
                        append(header[HEADER_PATH]).append(" ").append(header[HEADER_TARGET]).append(" ");
                newDocument.append(text);
                write(pipeDescriptor, newDocument.c_str(), newDocument.length() + 1);
            }

            //Should be finished...
            break;
        case DELIVERED:

            break;
        case DELIVERED_BROKEN:
            sendReceived(fileDescriptor);
            break;
        case UNREACHABLE:

            break;
        case UNREACHABLE_BROKEN:
            sendReceived(fileDescriptor);
            break;
        case RECEIVED:

            break;
        default:
            cerr << "Header type not recognised!" << endl;
            //TODO: DO SOMETHING
    }


}

string addMyIDToPath(string PATH) {
    return PATH.append(";").append(to_string(localID));
}

/**
 * Returns whether a specified id is in PATH string
 */
bool isInPath(int id, string PATH) {
    int i = 0;
    while(i < 1000) {
        int currID = getIDFromPath(PATH, i);
        if (currID < 0) {
            return false;
        } else if (id == currID) {
            return true;
        }
        i++;
    }
    cerr << "isInPath was looping for way too long! Check it out!" << endl;
    return false;
}

/**
 * Gets ID writeen in PATH on position.
 */
int getIDFromPath(string PATH, int position) {
    string ID = "";
    int i = 0;
    for (int j = 0; j < position + 1; ++j) {

        ID = "";
        while (i < PATH.length() && PATH[i] != ';') {
            ID.push_back(PATH[i]);
            i++;
            if (i > PATH.length()) {
                return -1;
            } else if (i == PATH.length()) {
                break;
            }
        }
        i++;

    }
    if (ID == "") {
        return -1;
    }
    return atoi(ID.c_str());

}

/**
 * Sends received message back to neighbour
 */
void sendReceived(int fileDescriptor) {
    string receivedMessage = "";
    receivedMessage.push_back(RECEIVED);
    receivedMessage.append(" ");
    receivedMessage.append(to_string(generateMessageID()));
    receivedMessage.append(" ").append(to_string(localID)).append(" ");
    receivedMessage.append("YOU It came!");
    write(fileDescriptor, receivedMessage.c_str(), receivedMessage.length());
}


int generateMessageID() {
    time_t currTime;
    time(&currTime);

    //Make this more complex maybe
    return (int) currTime;
}

/**
 * Sending section of the application. Handles send-requests
 */
void sending(int readFD) {
    //TODO: COMPLETE THIS
}

/**
 * Input section of the application. Handles User Input
 * @param writeFD
 */
void inputing(int writeFD) {
    //TODO: COMPLETE THIS
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