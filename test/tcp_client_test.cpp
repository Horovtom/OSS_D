#include <stdlib.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
using namespace std;

void error(string message)
{
    cerr << message << endl;
    exit(-1);
}

int main(int argc, char *argv[])
{
    /**
     * viz. server
     */
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    /**
     *  The variable server is a pointer to a structure of type hostent. This structure is defined in the header file netdb.h as follows:

    struct  hostent {
        char    *h_name;                    official name of host
        char    **h_aliases;                alias list
        int     h_addrtype;                 host address type
        int     h_length;                   length of address
        char    **h_addr_list;              list of addresses from name server
        #define h_addr  h_addr_list[0]      address, for backward compatiblity
    };

    *   It defines a host computer on the Internet. The members of this structure are:

    *   h_name       Official name of the host.

    *   h_aliases    A zero  terminated  array  of  alternate names for the host.

    *   h_addrtype   The  type  of  address  being  returned;
    *   currently always AF_INET.

    *   h_length     The length, in bytes, of the address.

    *   h_addr_list  A pointer to a list of network addresses for the named host.  Host addresses are
    *   returned in network byte order.

    *   Note that h_addr is an alias for the first address in the array of network addresses.
    *
    *   I AM NOT SURE IF THIS IS NESSESSARY
    */
    struct hostent *server;
    //region Identical to server
    char buffer[256];
    if (argc < 3) {
        cerr << "usage" << argv[0] << "hostname port" << argv[0] << endl;
        exit(-1);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    //endregion

    /**
     * rgv[1] contains the name of a host on the Internet, e.g. cheerios@cs.rpi.edu. The function:
     *
     *       struct hostent *gethostbyname(char *name)
     *
     * Takes such a name as an argument and returns a pointer to a hostent containing information about that host.
     * The field char *h_addr contains the IP address. If this structure is NULL, the system could not locate a host with this name.
     *
     * The mechanism by which this function works is complex, often involves querying large databases all around the country.
     *
     * horovtom: IM NOT SURE IF WE ARE ALLOWED TO USE THIS. ANYWAY IT IS REDUNDANT FOR OUR CAUSE. ALL OF OUR CLIENTS WILL
     * RUN ON THE SAME MACHINE SO THEIR IP IS GONNA BE THE SAME: 127.0.0.1
     */
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        cerr << "ERROR, no such host" << endl;
        exit(-1);
    }
    /**
     * This code sets the fields in serv_addr. Much of it is the same as in the server.
     * However, because the field server->h_addr is a character string, we use the function:
     *      void bcopy(char *s1, char *s2, int length)
     * which copies length bytes from s1 to s2.
     */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, //Reading from field h_addr of object server
          (char *)&serv_addr.sin_addr.s_addr, //Writing to our address object.
          server->h_length); //Getting the length from field h_length of server object.
    serv_addr.sin_port = htons(portno);

    /**
     * The connect function is called by the client to establish a connection to the server.
     * It takes three arguments, the socket file descriptor,
     * the address of the host to which it wants to connect (including the port number),
     * and the size of this address.
     * This function returns 0 on success and -1 if it fails.
     *
     * Notice that the client needs to know the port number of the server,
     * but it does not need to know its own port number.
     * This is typically assigned by the system when connect is called.
     */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    cout << "Please enter the message: ";
    bzero(buffer,256);
    //If you dont know C++ this is how you get user input:
    cin >> buffer;
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0)
        error("ERROR reading from socket");
    cout << buffer << endl;
    return 0;
}