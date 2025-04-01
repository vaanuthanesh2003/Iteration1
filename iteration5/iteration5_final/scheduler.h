#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <iostream>
#include <unordered_map>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define BASE_PORT 5100
#define SCHEDULER_PORT 5002
#define MAX_CAPACITY 4

struct Request {
    int floor;
    int targetFloor;
    std::string direction;

    Request(int f, int t, const std::string& d) : floor(f), targetFloor(t), direction(d) {}
};

class Scheduler {
protected:  // changed from private to protected so tests can subclass (optional)
    int sockfd;
    struct sockaddr_in selfAddr;

    std::unordered_map<int, int> elevatorFloors;
    std::unordered_map<int, int> elevatorLoad;
    std::unordered_map<int, std::string> elevatorStatus;
    std::unordered_map<int, std::chrono::steady_clock::time_point> warningTimestamps;

    std::queue<Request> requestQueue;
    std::mutex queueMutex;
    std::condition_variable cv;

    int moveCount = 0;
    int elevatorCount;
    int floorCount;
    
    void processRequests();
    
    #ifndef TEST_BUILD
        bool stop_requested = false;
    #endif

public:
    Scheduler(int elevCount, int floorMax);
    virtual ~Scheduler();

    void start();

    //  For testing only: safely inject a request into the queue
    #ifdef TEST_BUILD

    public:
        void runProcessThread() {
            processRequests();
         }
        void testInjectRequest(const Request& req) {
            std::lock_guard<std::mutex> lock(queueMutex);
            requestQueue.push(req);
            cv.notify_one();
        }
    #endif

    // Virtual so we can mock in tests
    virtual void sendMoveCommand(int elevatorID, int targetFloor);

private:
    void receiveMessages();
    void handleClientRequest(std::stringstream& ss, const std::string& firstToken);
    int findBestElevator(const Request& req);
    void displayStatusLoop();
    
};

#endif // SCHEDULER_H
