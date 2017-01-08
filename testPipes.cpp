//
// Created by lactosis on 8.1.17.
//

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

using namespace std;
int main() {
    //int pipe1FD[2], pipe2FD[2], pipe3FD[2], rs1, rs2, rs3;
    int pipe1FD[2], rs1;
    rs1 = pipe(pipe1FD);
//    rs2 = pipe(pipe2FD);
//    rs3 = pipe(pipe3FD);

//    if (rs1 < 0 || rs2 < 0 || rs3 < 0){
//        cerr << "Error creating pipe" << endl;
//        exit(-1);
//    }

    if (rs1<0) {
        cerr << "Error creating pipe" << endl;
        exit(-1);
    }

    int pid1 = 0;

    pid1 = fork();
    if (pid1 == 0) {
        //This is the child code: Lets scan:
        close(pipe1FD[0]);
        string message;
        getline(cin, message);
        cout << "Im the child and you entered: " << message << endl;
        write(pipe1FD[1], message.c_str(), message.length() + 1);
        exit(EXIT_SUCCESS);
    } else {
        //This is the parent code: Lets write:
        sleep(5);
        close(pipe1FD[1]);
        char ch;
        string message = "";
        while(read(pipe1FD[0], &ch, 1) > 0) {
            if (ch != 0) message.push_back(ch);
            else cout << message << endl;
        }

        int returnVal;
        wait(&returnVal);
        return returnVal;
    }

}