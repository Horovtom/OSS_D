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

bool isInPath(int id, string PATH);
int getIDFromPath(string PATH, int position);
void startForking();
void sigusr1Handler(int signalNum);
void recieving(), sending(),inputing();

//region startForking
int pid0, pid1, pid2;
int pipeFD[2];

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



void startForking() {
    pid0 = ::getpid();

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

        recieving();

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

            sending();

            //Terminate this... (Unreachable)
            close(pipeFD[READ_END]);
            cerr << "SEND app terminating..." << endl;
            exit(EXIT_SUCCESS);
        } else {
            //INPUT code:
            close(pipeFD[READ_END]);
            signal(SIGUSR1, sigusr1Handler);
            inputing();

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

void recieving() {
    sleep(1);
    cout << "RECIEVING -> sending 'Hello' to SENDING" << endl;
    write(pipeFD[WRITE_END], "Hello", 6);
    sleep(30);

}

void inputing() {
    sleep(7);
    cout << "INPUTING -> sending 'Karel' to SENDING" << endl;
    write(pipeFD[WRITE_END], "Karel", 6);
    sleep(3);
    cout << "INPUTING -> gimme string: ";
    string message = "";
    cin >> message;
    cout << "INPUTING -> sending '" << message << "' to SENDING" << endl;
    write(pipeFD[WRITE_END], message.c_str(), message.length() + 1);
    sleep(2);
}

void sending() {
    while(1) {
        string message = "";
        char ch;

        while (read(pipeFD[READ_END], &ch, 1) > 0) {
            if (ch != 0) {
                //cout << "   SENDING -> got char '" << ch << "'" << endl;
                message.push_back(ch);
            } else {
                //cout << "   SENDING -> got 0" << endl;
                break;
            }
        }
        cout << "SENDING -> got message '" << message << "' " << endl;
    }
}

//endregion

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

int main() {
    string PATH = "123;213;44415;9214;21";
    cout << "Starting test for PATH: " << PATH << endl;
    cout << "testGetIDFromPath()" << endl;
    cout << "Expected: 123 | Got: " << getIDFromPath(PATH, 0) << endl;
    cout << "Expected: 9214 | Got: " << getIDFromPath(PATH, 3) << endl;
    cout << "Expected: 21 | Got: " << getIDFromPath(PATH, 4) << endl;
    cout << "Expected: <nothing> | Got: " << getIDFromPath(PATH, 7) << endl;
    cout << "--------------------------" << endl;
    cout << "test isInPath()" << endl;
    cout << "Expected: 1 | Got: " << isInPath(213, PATH) << endl;
    cout << "Expected: 0 | Got: " << isInPath(111, PATH) << endl;
    cout << "Expected: 0 | Got: " << isInPath(1023, PATH) << endl;
    sleep(1);
    cout << "---------------------------" << endl;
    cout << "Starting fork test:" << endl;
    startForking();

    return 0;
}