//
// Created by lactosis on 6.1.17.
//

#include <stdlib.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>

using namespace std;

int main() {
    int socketFileDecriptor;
    struct sockaddr_in socketAddress;
    struct {
        uint32_t number;
        char message[5];
    } message;
    int bytes_sent, buffer_length;

    socketFileDecriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketFileDecriptor < 0 ) {
        cout << "Error creating socket!" << endl;
        exit(EXIT_FAILURE);
    }

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(7654);

    cout << "Gimme number: ";
    cin >> message.number;
    message.number = htonl(message.number);
    cout << "\nGimme string: ";
    cin >> message.message;

    bytes_sent = sendto(socketFileDecriptor, &message, sizeof(message) , 0, (struct sockaddr *)&socketAddress, sizeof socketAddress);
    if (bytes_sent < 0) {
        cout << "Error sending packets! " << strerror(errno)<< endl;
        exit(EXIT_FAILURE);
    }

    close(socketFileDecriptor);
    return 0;

}

