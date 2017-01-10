//
// Created by lactosis on 9.1.17.
//

/**
 * Site works like this:
 * Say node wants to send a message to another node on specified <ID>, <mesID> is a part of the message, <PATH> is a part of the message.
 * Node sends MSG to neighbours, they log that they had a message with such <mesID>, append node <ID> to <PATH>
 * After that, they send the message to their neighbours.
 * Every other iteration, nodes look at <PATH> the message had taken. If their Neighbour is already in the <PATH> they do not send it to him.
 * If node sees, that the message is for his neighbour, sends it to him directly. This node is waiting for RECEIVED message from the target node.
 * If it doesnt get it, send UNREACHABLE back by the <PATH> message had taken.
 * The target node receives message and immediately sends RECEIVED message back to the sender. After that he displays the message to user.
 * Target send DELIVERED type of message back to the original sender, by the <PATH> of original message.
 *
 * OPTIONAL:
 *  Every node one the returning way (DELIVERED, UNREACHABLE) should send RECEIVED message back to the neighbour.
 *  If RECEIVED doesnt make it in time to the neighbour, he sends DELIVERED_BROKEN/UNREACHABLE_BROKEN
 *  These behave just like normal messages... (Do not take the shortest path, but blindly wander around the network)
 *  If they do not make it to the original sender in time, he throws target_unreachable back at the user.
 *
 *
 * DELIVERED/UNREACHABLE message looks like:
 *      Header: <DELIVERED> <Reply's ID> <original message PATH> <to whom this DELIVERED should go> <ID of original message>
 *
 * RECEIVED messag looks like:
 *      Header: <RECEIVED> <Reply's ID> <this node's ID> <to whom this RECEIVED was sent> <ID of original message>
 *
 * BROKEN message looks like:
 *      Header: <DELIVERED_BROKEN> <generatedID> <this node's ID> <to whom this meessage should go> <ID of original message>
 *
 * MSG message looks like:
 *      Header <MSG> <generatedID> <this node's ID> <TARGET>
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
#include <algorithm>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>

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
class Dataholder {
private:
    string message;
    vector<int> toWho;
    bool readyToRead;
    bool readyToWrite;
    bool shouldBeRunning;

public:
    Dataholder(void) {
        readyToRead = false;
        readyToWrite = true;
        shouldBeRunning = true;
    }

    bool shouldRun() {
        return shouldBeRunning;
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

    /**
     * This is used for stopping the whole app...
     */
    void shouldStop() {
        cerr << "Im stopping the app..." << endl;
        shouldBeRunning = false;
        readyToRead = true;
        readyToWrite = true;
        cv.notify_all();
        cv2.notify_all();
    }

    string read_message() {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [&] { return isReadyToRead(); });
        if (!shouldBeRunning) {
            return "";
        }

        if (!readyToRead) {
            cerr << "Somebody is trying to read, when he does not have the right to do so!" << endl;
            return NULL;
        } else {
            return message;
        }
    }

    vector<int> read_toWho() {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [&] { return isReadyToRead(); });
        vector<int> empty;
        if (!shouldBeRunning) {
            return empty;
        }

        if (!readyToRead) {
            cerr << "Somebody is trying to read, when he does not have the right to do so!" << endl;
            return empty;
        } else {
            return toWho;
        }
    }

    void write(string text, vector<int> recipients) {
        unique_lock<mutex> lk(m);
        cv2.wait(lk, [&] { return isReadyToWrite(); });
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

inline bool isInteger(const std::string &s);

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

void waitingForReceived(int targetID, string PATH, string messageID, int type);

void gotDelivered(string ID);

void gotUnreachable(string ID);

int removeTimer(string ID);

void passBroken(string header[5]);

/**
 * This function resends message.
 */
void passMessage(string header[5], string text);

void queueSendMessage(int toWhom, string text);

void waitingForDelivered(int id, int toWhom);

Dataholder mailbox;



// region PART SEND



/**
 * Takes care of sending messages
 */
void partSend() {
    while (mailbox.shouldRun()) {
        string message = mailbox.read_message();
        vector<int> toWho = mailbox.read_toWho();
        mailbox.setReadyToWrite();

        if (message == "" || toWho.size() == 0) {
            break;
        }

        for (int i = 0; i < toWho.size(); ++i) {
            //TODO: TeSTING
//            cout << "Sending message to : " << toWho[i] << endl;
            int result = (int) sendto(cliSockFD, message.c_str(), message.length(), 0,
                                      (struct sockaddr *) &neighbours[toWho[i]], sizeof(neighbours[toWho[i]]));
            //TODO: TESTING
//            cout << "Sending message returned: " << result << endl;
        }

    }
    cerr << "Terminate SEND" << endl;
}

// endregion

//region PART RECEIVE

/**
 * Má na starosti obsluhu příchozích message na pozadí
 */
void partReceive() {
    struct timeval timeOutVal;
    char buffer[MAX_MESSAGE_LEN];
    ssize_t rect_len;

    fd_set set;

    while (mailbox.shouldRun()) {
        bzero(&buffer, MAX_MESSAGE_LEN);

        FD_ZERO(&set);
        FD_SET(servSockFD, &set);
        timeOutVal.tv_sec = 1;
        rect_len = 0;
        if (select(servSockFD + 1, &set, NULL, NULL, &timeOutVal) > 0) {
//            cout << "I am receiving something!" << endl;
            rect_len = (int) recv(servSockFD, &buffer, MAX_MESSAGE_LEN, 0);
//            cout << "I ve received " << rect_len << " chars" << endl;
        }

        if (rect_len > 0) {
            string message = buffer;
            parseMessage(message);
        }
    }
    cerr << "Terminating RECEIVE" << endl;
}

void parseMessage(string message) {
    string header[5];
    string text;

    std::istringstream stream(message);
    string headerLine;
    getline(stream, headerLine);
    int currChar = 0;
    for (int k = 0; k < 5; ++k) {
        if (currChar >= headerLine.length() && k < 4){
            cerr << "Error parsing header! Throwing away!" << endl;
            return;
        }
        while (currChar < headerLine.length() && message[currChar] != ' ') {
            header[k].push_back(message[currChar]);
            currChar++;
        }
        currChar++;
    }
    string currLine;
    while (getline(stream, currLine)) {
        text.append(currLine);
    }


    //Loaded:

    //If this message had already been here
    for (int j = 0; j < listOfMessages.size(); ++j) {
        if (listOfMessages[j] == atol(header[HEADER_ID].c_str()) &&
            atoi(header[HEADER_TYPE].c_str()) != RECEIVED)
            return;
    }
    //Add it to the list of messages
    listOfMessages.push_back(atol(header[HEADER_ID].c_str()));
    switch (atoi(header[HEADER_TYPE].c_str())) {
        case MSG: {
            if (atoi(header[HEADER_TARGET].c_str()) == localID) {
                //This message was for me... Print it to cout and send received
                int from = getIDFromPath(header[HEADER_PATH], 0);
                queueSendingReceived(header[HEADER_ID], from);
                cout << "Message from: " << from << endl;
                cout << text << endl;
                cout << "-------------\n" << endl;
                //Send back DELIVERED
                queueSendingDelivered(header[HEADER_PATH], header[HEADER_ID]);
            } else {
                passMessage(header, text);
            }

            //Should be finished...
            break;
        }
        case DELIVERED: {
            queueSendingDelivered(header[HEADER_PATH], header[HEADER_ID]);
            if (atoi(header[HEADER_TARGET].c_str()) == localID) {
                //Was for me...
                gotDelivered(header[HEADER_OPTIONAL]);
            }
            break;
        }
        case DELIVERED_BROKEN: {
            if (atoi(header[HEADER_TARGET].c_str()) == localID) {
                //This was for me!
                gotDelivered(header[HEADER_OPTIONAL]);
            } else {
                //Resend it like a normal message
                passMessage(header, "");
            }
            break;
        }

        case UNREACHABLE_BROKEN: {
            if (atoi(header[HEADER_TARGET].c_str()) == localID) {
                //This was for me!
                gotUnreachable(header[HEADER_OPTIONAL]);
            } else {
                //Resend it like a normal message
                passMessage(header, "");
            }

            break;
        }

        case UNREACHABLE: {

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
 * This function resends messages
 */
void passMessage(string header[5], string text) {
    header[HEADER_PATH] = addMyIDToPath(header[HEADER_PATH]);

    vector<int> toWho;
    for (int i = 0; i < connectionCount; ++i) {
        if (atoi(header[HEADER_TARGET].c_str()) == connections[i].id) {
            //It is for my neighbour!!!
            toWho.clear();
            toWho.push_back(connections[i].id);
            waitingForReceived(connections[i].id, header[HEADER_PATH], header[HEADER_ID],
                               atoi(header[HEADER_TYPE].c_str()));
            break;
        }

        if (!isInPath(connections[i].id, header[HEADER_PATH])) {
            //Fill string of messages:
            toWho.push_back(connections[i].id);
        }
    }
    //ToWho filled

    queueResendingMessage("", toWho, header);
}

/**
 * Remove timer on TIMEOUT
 */
void gotUnreachable(string ID) {
    int who = removeTimer(ID);
    cerr << "Target: " << who << " was unreachable.." << endl;
}

/**
 * This function removes the timer on TIEOUT, because DELIVERED message invoked by message with <ID> had arrived
 */
void gotDelivered(string ID) {
    removeTimer(ID);
}

/**
 * Removes the timer and returns the ID of node it was sent to.
 */
int removeTimer(string ID) {
    //TODO: REMOVE THE TIMER (No idea how to implement this)
}

/**
 * Queues the wait for RECEIVED message...
 * @param targetID the ID of node from which the RECEIVED should come
 * @param PATH is the path the message had taken previously
 * @param messageID
 * @param type of message that it was
 */
void waitingForReceived(int targetID, string PATH, string messageID, int type) {
    //TODO: DoStuff with this (I have no idea how to time this...)

    if (type == MSG) {
        //If this does not come, send UNREACAHBLE back by PATH

    } else {

        if (type == UNREACHABLE || type == DELIVERED) {
            //If this does not come, send UNREACHABLE_BROKEN/DELIVERED_BROKEN

        } else {
            cerr << "I don't know which type this is:" << type << endl;
            return;
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
    cout << "Looking for " << id << " in " << PATH << endl;
    int i = 0;
    while (i < 1000) {
        int currID = getIDFromPath(PATH, i);
        if (currID < 0) {
            cout << "Did nto find it!" << endl;
            return false;
        } else if (id == currID) {
            cout << "Found it!" << endl;
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
    while (1) {
        int returnVal = getIDFromPath(PATH, i);
        if (returnVal == -1) {
            return getIDFromPath(PATH, i - 1);
        } else if (returnVal == localID) {
            return getIDFromPath(PATH, i - 1);
        } else {
            i++;
        }
    }
}

void putToMailbox(string message, int toWho) {
    vector<int> toPass;
    toPass.push_back(toWho);
    putToMailbox(message, toPass);
}

void putToMailbox(string message, vector<int> toWho) {
    mailbox.write(message, toWho);
    mailbox.setReadyToRead();
}

/**
 * Tells SENDING part of the app to send RECEIVED message to the sender (neighbour)
 */
void queueSendingReceived(string messageID, int senderID) {
    string messageToSend = "RECEIVED ";
    messageToSend.append(to_string(generateMessageID())).append(" ").append(to_string(localID)).append(" ").append(
            to_string(senderID)).append(" ");
    messageToSend.append(messageID);

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
 * Serves user-input related stuff
 */
void partInput() {
    //TODO: ADD SUPPORT FOR NEIGHBOUR AS TARGET
    while (mailbox.shouldRun()) {
        string toWhom;
        string text;
        cout << "Enter id of client or END for exit: ";
        cin >> toWhom;
        if (toWhom == "END") {
            //ENDING APP
            mailbox.shouldStop();

        } else if (!isInteger(toWhom) || atoi(toWhom.c_str()) <= 0) {
            cerr << "You haven't entered valid ID" << endl;
        } else {
            //Okay it seems valid
            cout << "Enter text to send to " << toWhom << endl;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin, text);
            if (text == "") {
                cerr << "Text is empty! wont send it!" << endl;
            } else {
                //Send it:
                queueSendMessage(atoi(toWhom.c_str()), string(text));
            }
        }
    }
}

/**
 * This function queues message for sending and it registers timer to look for DELIVERED
 */
void queueSendMessage(int toWho, string text) {
    string message = "MSG ";
    int id = generateMessageID();
    waitingForDelivered(id, toWho);
    message.append(to_string(id)).append(" ").append(to_string(localID)).append(" ");
    message.append(to_string(toWho)).append(" \n");
    message.append(text);

    vector<int> neighbours;
    for (int i = 0; i < connectionCount; ++i) {
        neighbours.push_back(connections[i].id);
    }


    mailbox.write(message, neighbours);
    mailbox.setReadyToRead();
}

/**
 * This function registeres WAIT for DELIVERED message
 */
void waitingForDelivered(int id, int toWhom) {
    //TODO: Register the wait and write down the id->toWhom relation (I have no idea how to implement the wait)
}

//endregion




int generateMessageID() {
    time_t currTime;
    time(&currTime);

    //Make this more complex maybe
    return (int) currTime;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
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
                if (connections[i].ip_address == "") {
                    cout << "Rewriting to localhost:" << endl;
                    strcpy(connections[i].ip_address, "127.0.0.1");
                    cout << "Rewritten to: " << connections[i].ip_address<<endl;
                }
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
    cout << "Local port: " << localPort  << endl;
    serv_addr.sin_port = htons((uint16_t) localPort);

    //Bind socket to port:
    if (bind(servSockFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Unable to bind socket to port!" << endl;
        exit(-1);
    }

    //Set client socket
    bzero(&cli_addr, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    cli_addr.sin_port = htons((uint16_t) (SENDING_PORT - localID));

    //Bind socket to port:
    if (bind(
            cliSockFD, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0) {
        cerr << "Unable to bind socket to port!" << endl;
        exit(-1);
    }

    //Save neighbour addresses...
    for (int j = 0; j < connectionCount; ++j) {
        sockaddr_in toAdd;
        toAdd.sin_family = AF_INET;
        if (!connections[j].ip_address[0]) {
            strcpy(connections[j].ip_address, "127.0.0.1");
//            connections[j].ip_address[0] = '1';
            toAdd.sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
            inet_pton(AF_INET, connections[j].ip_address, &toAdd.sin_addr);
        }
//        cout << "       Node " << connections[j].id << " address: " << connections[j].ip_address << endl;
        toAdd.sin_port = htons((uint16_t) connections[j].port);
        neighbours[connections[j].id] = toAdd;
    }

    //Split into 3 parts SEND, RECV, INPUT:
    thread send(partSend);
    thread receive(partReceive);
    thread input(partInput);
    cout << "Application successfully created all threads" << endl;

    //Now join everything:
    input.join();
    send.join();
    receive.join();
    cout << "Application succesfully stopped all threads" << endl;
    close(servSockFD);
    close(cliSockFD);
    return 0;
}

void testing() {
    //TODO: Insert tests here!

    if ((servSockFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        cerr << "Failed to create socket" << endl;
        exit(EXIT_FAILURE);
    }

    connectionCount = 100;
    localID = 1;
    parseRouteConfiguration("routing.cfg", localID, &localPort, &connectionCount, connections);

    //Set server socket
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons((uint16_t) localPort);

    //Bind socket to port:
    if (bind(
            servSockFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Unable to bind socket to port!" << endl;
        exit(-1);
    }

    partReceive();

//
//    string message = to_string(DELIVERED_BROKEN);
//    message.append(" 2111 2;3 6\nAhoj já jsem týpek!");
//    cout << "\n---------\nMessage was: \n" << message << "\n-----" << endl;
//    parseMessage(message);
}

bool isInteger(const std::string &s) {
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

    char *p;
    strtol(s.c_str(), &p, 10);

    return (*p == 0);
}
