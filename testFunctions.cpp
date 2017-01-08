//
// Created by lactosis on 8.1.17.
//

#include <iostream>
#include <string>
#include <stdio.h>
#include <assert.h>

using namespace std;

bool isInPath(int id, string PATH);
int getIDFromPath(string PATH, int position);

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

}