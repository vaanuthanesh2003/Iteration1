#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <iostream>
#include <unordered_map>
#include <queue>
#include <string>
#include <netinet/in.h>
#include <mutex>
#include <condition_variable>

struct Request {
    int floor;
    int targetFloor;
    std::string direction;

    Request(int f, int t, std::string d) : floor(f), targetFloor(t), direction(d) {}
};

class Scheduler {
public:
    int sockfd;
    struct sockaddr_in clientAddr, selfAddr, elevatorAddr;
    std::unordered_map<int, int> elevatorPositions; // ElevatorID -> Floor
    std::queue<Request> requestQueue;
    std::mutex queueMutex;
    std::condition_variable cv;

public:
    Scheduler(int numElevators);
    void receiveRequests();
    void processRequests();
    int assignElevator(const Request& req);
    virtual void sendCommandToElevator(int elevatorID, const Request& req);
    void receiveElevatorUpdates();
    void run();
};

#endif // SCHEDULER_H
