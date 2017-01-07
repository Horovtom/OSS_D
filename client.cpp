#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>

using namespace std;
int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Wrong number of arguments! \nUsage: ./node <ID>" << endl;
        return -1;
    }
    int ID = atoi(argv[1]);
    if (ID == 0) {
        cerr << "Invalid ID: " << argv[1] << endl;
        return -1;
    }
    cout << "Starting client with ID " << argv[1]  << endl;


}