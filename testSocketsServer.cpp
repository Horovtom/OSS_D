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
    struct sockaddr_in si_me, si_other;

    int s, i , recv_len;
    socklen_t slen = sizeof(si_other);
    char buf[256];
    int buflen = 256;

    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        cerr << "Bullshit" << endl;
    }

    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(7648);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        cerr << "Bullshit2 " << endl;
    }

    //keep listening for data

        printf("Waiting for data...");
        fflush(stdout);

        //try to receive some data, this is a blocking call
        if ((recv_len = (int) recvfrom(s, buf, (size_t) buflen, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            cerr << "Bullshit3" << endl;
        }

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Data: %s\n" , buf);


    close(s);
    return 0;




}