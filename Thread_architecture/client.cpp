//
// Created by lactosis on 9.1.17.
//

/**
 * DELIVERED message looks like:
 *
 *      Header: <DELIVERED> <Reply's ID> <original message PATH> <to whom this DELIVERED should go>
 *      text: <ID of original message>
 *
 * RECEIVED messag looks like:
 *      Header: <RECEIVED> <Reply's ID> <this node's ID> <to whom this RECEIVED was sent>
 *      text: <ID of original message>
 */

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
#include <sstream>

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
/**
 * This header argument is optional.. Is used with return-type messages to fill in the old-message ID
 */
#define HEADER_OPTIONAL 4


/**
 * This is number of port from which will the application send messages to the outside world
 */
#define SENDING_PORT 19976

using namespace std;

/**
 * This variable stores all the IDs of messages that went through this node.
 */
std::vector<long> listOfMessages;
int localID;
int localPort;

int connectionCount;
TConnection connections[100];
sockaddr_in serv_addr;
sockaddr_in cli_addr;
/**
 * This is a map of <ID, net address> of all the neighbours
 */
map<int, sockaddr_in> neighbours;
int servSockFD, cliSockFD;
mutex m;
condition_variable cv;
condition_variable cv2;

/**
 * Class that contains info that is shared among processes
 */
class Dataholder{
private:
    string message;
    vector<int> toWho;
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
    string read_message() {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [&]{return isReadyToRead();});

        if (!readyToRead) {
            cerr << "Somebody is trying to read, when he does not have the right to do so!" << endl;
            return NULL;
        } else {
            return message;
        }
    }
    vector<int> read_toWho() {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [&]{return isReadyToRead();});

        if (!readyToRead) {
            cerr << "Somebody is trying to read, when he does not have the right to do so!" << endl;
            vector<int> empty;
            return empty;
        } else {
            return toWho;
        }
    }
    void write(string text, vector<int> recipients) {
        unique_lock<mutex> lk(m);
        cv2.wait(lk, [&]{return isReadyToWrite();});
        if (!readyToWrite) {
            cerr << "Somebody is trying to write when he does not have the right to!" << endl;
        } else {
            //Tady je díra... mělo by to tam být hned...
            readyToWrite = false;
            readyToRead = false;
            message = text;
            //Tohle by mělo samo dělat copy()
            toWho = recipients;
        }

    }
};

void testing();

int generateMessageID();

void putToMailbox(string message, int toWho);

void putToMailbox(string message, vector<int> toWho);

void parseMessage(string message);

int getIDFromPath(string messageID, int senderID);

void queueSendingReceived(string messageID, int senderID);

void queueSendingDelivered(string PATH, string ID_ofOriginal);

string addMyIDToPath(string PATH);

bool isInPath(int id, string PATH);

int getLastInPath(string PATH);

void queueResendingMessage(string text, vector<int> toWho, string Header[5]);

Dataholder mailbox;



// region PART SEND



/**
 * Má na starosti odesílání message
 */
void partSend() {

}

// endregion

//region PART RECEIVE

/**
 * Má na starosti obsluhu příchozích message na pozadí
 */
void partReceive() {
    char buffer[MAX_MESSAGE_LEN];
    while(1) {
        bzero(&buffer, MAX_MESSAGE_LEN);
        sockaddr_in newAddr;
        bzero(&newAddr, sizeof(newAddr));
        socklen_t slen = sizeof(newAddr);
        int rect_len = (int) recvfrom(servSockFD, buffer, MAX_MESSAGE_LEN, 0, (struct sockaddr*) &newAddr, &slen);
        string message = buffer;

        if (rect_len > 0) {
            parseMessage(message);
        }
    }
}

void parseMessage(string message) {
    string header[5];
    string text;

    std::istringstream stream(message);
    string headerLine;
    getline(stream, headerLine);
    int currChar = 0;
    for (int k = 0; k < 5; ++k) {
        while(currChar < headerLine.length() && message[currChar] != ' ') {
            header[k].push_back(message[currChar]);
            currChar++;
        }
        currChar++;
    }
    string currLine;
    while(getline(stream, currLine)) {
        text.append(currLine);
    }


    //Loaded:

    //If this message had already been here
    for (int j = 0; j < listOfMessages.size(); ++j) {
        if (listOfMessages[j] == atol(header[HEADER_ID].c_str()) && atoi(header[HEADER_TYPE].c_str()) != RECEIVED) return;
    }
    //Add it to the list of messages
    listOfMessages.push_back(atol(header[HEADER_ID].c_str()));

    switch (atoi(header[HEADER_TYPE].c_str())) {
        case MSG: {
            //TODO: Probably need something to wait for received messages and throw panic if it doesnt come in time
            int from = getIDFromPath(header[HEADER_PATH], 0);
            queueSendingReceived(header[HEADER_ID], from);
            if (atoi(header[HEADER_TARGET].c_str()) == localID) {
                //This message was for me... Print it to cout
                cout << "Message from: " << from << endl;
                cout << text << endl;
                cout << "-------------\n" << endl;
                //Send back DELIVERED
                queueSendingDelivered(header[HEADER_PATH], header[HEADER_ID]);
            } else {
                //This message was not for me... Hubbing it
                //Adding my ID to its PATH:
                header[HEADER_PATH] = addMyIDToPath(header[HEADER_PATH]);

                vector<int> toWho;
                for (int i = 0; i < connectionCount; ++i) {
                    if (!isInPath(connections[i].id, header[HEADER_PATH])) {
                        //Fill string of messages:
                        toWho.push_back(connections[i].id);
                    }
                }
                //ToWho filled

                queueResendingMessage(text, toWho, header);
            }

            //Should be finished...
            break;
    }
        case DELIVERED: {

            queueSendingDelivered(header[HEADER_PATH], header[HEADER_ID]);
            if (atoi(header[HEADER_TARGET].c_str()) == localID) {
                //Was for me...
                //TODO: DOSTUFF


            }
            break;
        }
        case DELIVERED_BROKEN: {

            break;
        }

        case UNREACHABLE:{

            break;
        }

        case UNREACHABLE_BROKEN: {

            break;
        }

        case RECEIVED: {

            break;
        }

        default: {

            cerr << "Header type not recognised!" << endl;
            //TODO: DO SOMETHING
        }
    }
}

/**
 * Tells SENDING part of the app to forward message to toWho list.
 */
void queueResendingMessage(string text, vector<int> toWho, string Header[5]) {
    string newMessage = "";
    for (int i = 0; i < 5; ++i) {
        newMessage.append(Header[i]).append(" ");
    }
    newMessage.push_back('\n');
    newMessage.append(text);

    putToMailbox(newMessage, toWho);
}

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

string addMyIDToPath(string PATH) {
        return PATH.append(";").append(to_string(localID));
}

/**
 * Tells SENDING part of the app to send Delivered message to the sender (target)
 */
void queueSendingDelivered(string PATH, string ID_ofOriginal) {
    int from = getIDFromPath(PATH, 0);
    string messageToSend = "DELIVERED ";
    messageToSend.append(to_string(generateMessageID())).append(" ").append(PATH).append(" ").append(to_string(from));
    messageToSend.append(" ").append(ID_ofOriginal);

    int to = getLastInPath(PATH);
    putToMailbox(messageToSend, to);
}

/**
 * Gets the last ID of node before this node.
 */
int getLastInPath(string PATH) {
    int i = 0;
    while(1) {
        int returnVal = getIDFromPath(PATH, i);
        if (returnVal == -1) {
            return getIDFromPath(PATH, i-1);
        } else if (returnVal == localID) {
            return getIDFromPath(PATH, i - 1);
        } else {
            i++;
        }
    }
}

void putToMailbox(string message, int toWho){
    vector<int> toPass;
    toPass.push_back(toWho);
    putToMailbox(message, toPass);
}
void putToMailbox(string message, vector<int> toWho){
    mailbox.write(message,toWho);
    mailbox.setReadyToRead();
}

/**
 * Tells SENDING part of the app to send RECEIVED message to the sender (neighbour)
 */
void queueSendingReceived(string messageID, int senderID) {
    string messageToSend = "RECEIVED ";
    messageToSend.append(to_string(generateMessageID())).append(" ").append(to_string(localID)).append(" ").append(to_string(senderID)).append(" ");
    messageToSend.append(messageID);
    //TODO: Create counterpart listener in SENDING part of app

    putToMailbox(messageToSend, senderID);
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

//endregion

//region PART INPUT

/**
 * Má na starosti obsluhu uživatelského inputu
 */
void partInput() {
    //TODO: COMPLETE
}

//endregion


int generateMessageID() {
    time_t currTime;
    time(&currTime);

    //Make this more complex maybe
    return (int) currTime;
}

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

    if ((servSockFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        cerr << "Failed to create socket" << endl;
        exit(EXIT_FAILURE);
    }

    if ((cliSockFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        cerr << "Failed to create socket" << endl;
        exit(EXIT_FAILURE);
    }
    //Set server socket
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons((uint16_t) localPort);

    //Bind socket to port:
    if (bind(
            servSockFD, (struct sockaddr * ) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Unable to bind socket to port!" << endl;
        exit(-1);
    }

    //Set client socket
    bzero(&cli_addr, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    cli_addr.sin_port = htons(SENDING_PORT);

    //Bind socket to port:
    if (bind(
            cliSockFD, (struct sockaddr * ) &cli_addr, sizeof(cli_addr)) < 0) {
        cerr << "Unable to bind socket to port!" << endl;
        exit(-1);
    }

    //Save neighbour addresses...
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
    string message = to_string(MSG);
    message.append(" 2111 2;3 1\nAhoj já jsem týpek!");
    cout << "Message was: \n" << message << endl;
    localID = 1;
    parseMessage(message);
}
