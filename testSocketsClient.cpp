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
#include<arpa/inet.h>
#include <cstring>
#include <stdio.h>

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

int main(int argc, char* argv[]) {

    struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other);
    string message;

    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    cerr << "Bullshit " << endl;

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(7648);

    if (inet_aton("127.0.0.1" , &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

        printf("Enter message : ");
        cin >> message;

        //send the message
        if (sendto(s, message.c_str(), message.length() , 0 , (struct sockaddr *) &si_other, (socklen_t) slen) == -1)
        {
            cerr << "Bullshit2 "<< endl;
        }

    close(s);
    return 0;


}