//
// Created by lactosis on 9.1.17.
//
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;
int karel;
mutex myMutex;
condition_variable cv;
bool mainReady, workerReady;

void writeToConsole(string message) {
    lock_guard<mutex> guard(myMutex);
    cout << message << endl;
}

void changeKarel(int newKarel){

    karel = newKarel;
}

int readKarel() {
   return karel;
}


void thread_function()
{
    {
        unique_lock<mutex> lk(myMutex);
        cv.wait(lk, [] { return mainReady; });
    }
    //DoStuff
    writeToConsole(((string) "Karel was ").append(to_string(readKarel())));
    changeKarel(4);
    cout << "Thread changed karel to " << readKarel() << endl;
    {
        lock_guard<mutex> lk(myMutex);
        workerReady = true;
    }
    cv.notify_one();
}

int main()
{
    std::thread t(thread_function);
    writeToConsole("Karel = 2");
    changeKarel(2);
    {
        lock_guard<mutex> lk(myMutex);
        mainReady = true;
    }
    cv.notify_one();

    {
        unique_lock<mutex> lk(myMutex);
        cv.wait(lk, []{return workerReady;});
    }

    writeToConsole(((string) "Karel je " ).append(to_string(readKarel())));



    t.join();
    return 0;
}