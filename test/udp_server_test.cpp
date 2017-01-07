//
// Created by lactosis on 6.1.17.
//

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

#define BUFSIZE 1024

int main() {
    int socketFileDescriptor;

    /**
     * This is a structure that contains the address of the server- It is defined as :
     * struct sockaddr_in {
     *  short sin_family;
     *  u_short sin_port;
     *  struct in_addr sin_addr;
     *  char sin_zero[8];
     * }
     */
    struct sockaddr_in socketAddress;
    /**
     * In this code we are sending 1 integer and 1 string
     */
    struct {
        uint32_t number;
        char message[5];
    } message;
    ssize_t recievedSize;
    socklen_t sockAddressLength;

    //Create new socket
    socketFileDescriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketFileDescriptor < 0 ){
        cerr << "Error establishing socket.." << endl;
        exit(1);
    }

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(7654);
    sockAddressLength = sizeof(socketAddress);

    if (bind(socketFileDescriptor, (struct sockaddr *)&socketAddress, sizeof socketAddress) < 0) {
        cout << "Error binding failed!" << endl;
        close (socketFileDescriptor);
        exit(EXIT_FAILURE);
    }

    for (;;) {
        cout << "Ready to recieve...." << endl;
        recievedSize = recv(socketFileDescriptor, (void *) &message, sizeof(message), 0);
        if (recievedSize < 0) {
            cout << "An error has occurred: " << strerror(errno) << endl;
            exit(EXIT_FAILURE);
        } else if (recievedSize != sizeof(message)) {
            cout << "Recieved invalid message! " << endl;
        } else {
            message.number = ntohl(message.number);
            cout << "Size of the recieved message: " << recievedSize << endl;
            sleep(1);
            cout << "Datagram - number: " << message.number << " message: " << message.message << endl;
        }
    }

}