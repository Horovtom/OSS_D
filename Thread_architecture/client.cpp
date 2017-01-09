//
// Created by lactosis on 9.1.17.
//

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

mutex m;
condition_variable cv;
condition_variable cv2;

/**
 * Class that contains info that is shared among processes
 */
class Dataholder{
private:
    string message;
    bool readyToRead;
    bool readyToWrite;
public:
    Dataholder(void) {
        readyToRead = false;
        readyToWrite = true;
    }
    bool isReadyToRead() {
        return readyToRead;
    }
    bool isReadyToWrite() {
        return readyToWrite;
    }
    void setReadyToWrite() {
        readyToWrite = true;
        readyToRead = false;
        cv2.notify_one();
    }
    void setReadyToRead() {

        readyToRead = true;
        readyToWrite = false;
        cv.notify_one();
    }
    string read() {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [&]{return isReadyToRead();});

        if (!readyToRead) {
            cerr << "Somebody is trying to read, when he does not have the right to do so!" << endl;
            return NULL;
        } else {
            return message;
        }
    }
    void write(string toWrite) {
        unique_lock<mutex> lk(m);
        cv2.wait(lk, [&]{return isReadyToWrite();});
        if (!readyToWrite) {
            cerr << "Somebody is trying to write when he does not have the right to!" << endl;
        } else {
            message = toWrite;
        }

    }
};

Dataholder mailbox;

/**
 * Má na starosti odesílání message
 */
void partSend() {
    cout << mailbox.read() << endl;
    mailbox.setReadyToWrite();
    cout << mailbox.read() << endl;
    mailbox.setReadyToWrite();
}

/**
 * Má na starosti obsluhu příchozích message na pozadí
 */
void partReceive() {
    mailbox.write("Ahoj, já jsem RECIEVE");
    mailbox.setReadyToRead();
}

/**
 * Má na starosti obsluhu uživatelského inputu
 */
void partInput() {
    mailbox.write("Ahoj, já jsem INPUT");
    mailbox.setReadyToRead();
}








int main() {
    //Split into 3 parts SEND, RECV, INPUT:
    thread send(partSend);
    thread receive(partReceive);
    thread input(partInput);
    cout << "Application successfully created all threads" << endl;

    //Now join everything:
    send.join();
    receive.join();
    input.join();
    cout << "Application succesfully stopped all threads" << endl;
}
