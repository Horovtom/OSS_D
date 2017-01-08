//
// Created by lactosis on 8.1.17.
//

#include <iostream>
#include <string>
#include <stdio.h>
using namespace std;

string getIDFromPath(string PATH, int position) {
    string ID = "";
    int i = 0;
    for (int j = 0; j < position +1; ++j) {

        ID = "";
        while(i < PATH.length() && PATH[i] != ';'){
            ID.push_back(PATH[i]);
            i++;
            if (i > PATH.length()) {
                return NULL;
            } else if (i == PATH.length()) {
                break;
            }
        }
        i++;

    }

    return ID;

}

int main() {
    string PATH = "123;213;44415;9214;21";
    cout << "Expected: 123 | Got: " << getIDFromPath(PATH, 0) << endl;
    cout << "Expected: 9214 | Got: " << getIDFromPath(PATH, 3) << endl;
    cout << "Expected: 21 | Got: " << getIDFromPath(PATH, 4) << endl;
    cout << "Expected: <nothing> | Got: " << getIDFromPath(PATH, 7) << endl;
}