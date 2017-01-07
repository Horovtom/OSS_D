/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdlib.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>

#define BUFSIZE 256
using namespace std;

void error(string message) {
    cerr << message << endl;
    exit(-1);
}

int main(int argc, char *argv[]) {
    /**
     * File descriptors.
     * These two variables store the values returned by the socket system call and the accept system call.
     */
    int sockfd, newsockfd;
    /**
     * portno stores the port number on which the server accepts connections.
     */
    int portno;
    /**
     * clilen stores the size of the address of the client. This is needed for the accept system call.
     */
    socklen_t clilen;
    char buffer[BUFSIZE];
    /**
     * The variable serv_addr will contain the address of the server, and cli_addr will contain the address of the client which connects to the server.
     */
    struct sockaddr_in serv_addr, cli_addr;
    /**
     * n is the return value for the read() and write() calls; i.e. it contains the number of characters read or written.
     */
    int n;
    /**
     * The user needs to pass in the port number on which the server will accept connections as an argument.
     * This code displays an error message if the user fails to do this.
     */
    if (argc < 2) {
        cerr << "ERROR, no port provided" << endl;
        exit(1);
    }

    /**
     * The socket() system call creates a new socket. It takes three arguments.
     * The first is the address domain of the socket.
     * Recall that there are two possible address domains, the unix domain for two processes which
     * share a common file system, and the Internet domain for any two hosts on the Internet.
     * The symbol constant AF_UNIX is used for the former, and AF_INET for the latter
     *
     *  The second argument is the type of socket.
     *  Recall that there are two choices here, a stream socket in which characters are read in a continuous
     *  stream as if from a file or pipe, and a datagram socket, in which messages are read in chunks.
     *  The two symbolic constants are SOCK_STREAM and SOCK_DGRAM. The third argument is the protocol.
     *  If this argument is zero (and it always should be except for unusual circumstances), the operating system
     *  will choose the most appropriate protocol. It will choose TCP for stream sockets and UDP for datagram sockets.
     *
     *  The socket system call returns an entry into the file descriptor table (i.e. a small integer).
     *  This value is used for all subsequent references to this socket. If the socket call fails, it returns -1.
     *  In this case the program displays and error message and exits.
     *  However, this system call is unlikely to fail.
     */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    /**
     * The function bzero() sets all values in a buffer to zero.
     * It takes two arguments, the first is a pointer to the buffer and the second is the size of the buffer.
     * Thus, this line initializes serv_addr to zeros.
     */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    /**
     * The port number on which the server will listen for connections is passed in as an argument,
     * and this statement uses the atoi() function to convert this from a string of digits to an integer.
     */
    portno = atoi(argv[1]);
    /**
     * The first field is short sin_family, which contains a code for the address family.
     * It should always be set to the symbolic constant AF_INET.
     * -> There are other families: AF_INET6, AF_BLUETOOTH etc.
     */
    serv_addr.sin_family = AF_INET;
    /**
     * The third field of sockaddr_in is a structure of type struct in_addr which
     * contains only a single field unsigned long s_addr. This field contains the IP address of the host. For server code,
     * this will always be the IP address of the machine on which the server is running,
     * and there is a symbolic constant INADDR_ANY which gets this address.
     */
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    /**
     * The second field of serv_addr is unsigned short sin_port , which contain the port number.
     * However, instead of simply copying the port number to this field,
     * it is necessary to convert this to network byte order using the function htons() which converts a port number
     * in host byte order to a port number in network byte order.
     */
    serv_addr.sin_port = htons(portno);
    /**
     * The bind() system call binds a socket to an address,
     * in this case the address of the current host and port number on which the server will run.
     * It takes three arguments, the socket file descriptor, the address to which is bound,
     * and the size of the address to which it is bound. The second argument is a pointer to a structure of type sockaddr,
     * but what is passed in is a structure of type sockaddr_in, and so this must be cast to the correct type.
     * This can fail for a number of reasons, the most obvious being that this socket is already in use on this machine.
     */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    /**
     * The listen system call allows the process to listen on the socket for connections.
     * The first argument is the socket file descriptor, and the second is the size of the backlog queue, i.e.,
     * the number of connections that can be waiting while the process is handling a particular connection.
     * This should be set to 5, the maximum size permitted by most systems.
     * If the first argument is a valid socket, this call cannot fail, and so the code doesn't check for errors.
     */
    listen(sockfd, 5);

    clilen = sizeof(cli_addr);
    cout << "Awaiting connection..." << endl;
    /**
     * The accept() system call causes the process to block until a client connects to the server.
     * Thus, it wakes up the process when a connection from a client has been successfully established.
     * It returns a new file descriptor, and all communication on this connection should be done using the new file descriptor.
     * The second argument is a reference pointer to the address of the client on the other end of the connection,
     * and the third argument is the size of this structure.
     *
     * So accept fills address of the client on the other end of the connection to the cli_addr variable
     */
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");
    /**
     * Initialize the buffer using the bzero() function
     */
    bzero(buffer, BUFSIZE);
    /**
     * Note that we would only get to this point after a client has successfully connected to our server.
     * This code reads from the socket. Note that the read call uses the new file descriptor,
     * the one returned by accept(), not the original file descriptor returned by socket().
     * Note also that the read() will block until there is something for it to read in the socket, i.e. after the client has executed a write().
     * It will read either the total number of characters in the socket or BUFSIZE, whichever is less, and return the number of characters read
     */
    n = read(newsockfd, buffer, BUFSIZE);
    if (n < 0) error("ERROR reading from socket");
    cout << "Here is the message: " << buffer << endl;
    /**
     * Once a connection has been established, both ends can both read and write to the connection.
     * Naturally, everything written by the client will be read by the server, and everything written by the server will be read by the client.
     * This code simply writes a short message to the client. The last argument of write is the size of the message.
     */
    n = write(newsockfd, "I got your message", 18);
    if (n < 0) error("ERROR writing to socket");
    //This close might cause problems, but it should be there...
    close(newsockfd);
    close(sockfd);
    return 0;
}