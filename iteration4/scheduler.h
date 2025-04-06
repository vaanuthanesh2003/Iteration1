#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <unordered_map>
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <netinet/in.h>

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
public:
    Scheduler(int elevCount, int floorMax);
    ~Scheduler();

    void start();

    // Public for unit testing
    int findBestElevator(const Request& req);

    // For testing directly (not called externally in runtime)
    std::unordered_map<int, int> elevatorFloors;
    std::unordered_map<int, int> elevatorLoad;
    std::unordered_map<int, std::string> elevatorStatus;

private:
    int sockfd;
    struct sockaddr_in selfAddr;
    std::queue<Request> requestQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::chrono::steady_clock::time_point startTime;
    std::unordered_map<int, std::chrono::steady_clock::time_point> warningTimestamps;
    int moveCount = 0;
    int requestsHandled = 0;
    int elevatorCount;
    int floorCount;

    void receiveMessages();
    void handleClientRequest(std::stringstream& ss, const std::string& firstToken);
    void processRequests();
    void sendMoveCommand(int elevatorID, int targetFloor);
    void displayStatus();
};

#endif // SCHEDULER_H

