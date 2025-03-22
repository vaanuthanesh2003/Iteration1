#include <iostream>
#include <unordered_map>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <climits>

#define SCHEDULER_PORT 5002
#define BASE_PORT 5100
#define BUFFER_SIZE 1024

struct Request {
    int floor;
    int targetFloor;
    std::string direction;

    Request(int f, int t, const std::string& d) : floor(f), targetFloor(t), direction(d) {}
};

class Scheduler {
private:
    int sockfd;
    struct sockaddr_in selfAddr;
    std::unordered_map<int, int> elevatorFloors;
    std::unordered_map<int, bool> elevatorStatus;
    std::queue<Request> requestQueue;
    std::mutex queueMutex;
    std::condition_variable cv;

public:
    Scheduler(int elevatorCount) : sockfd(socket(AF_INET, SOCK_DGRAM, 0)) {
        if (sockfd < 0) {
            perror("[Scheduler] Socket creation failed");
            exit(EXIT_FAILURE);
        }

        selfAddr = {AF_INET, htons(SCHEDULER_PORT), INADDR_ANY};
        if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
            perror("[Scheduler] Bind failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 1; i <= elevatorCount; ++i) {
            elevatorFloors[i] = 0;
            elevatorStatus[i] = true;
        }

        std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
    }

    void start() {
        std::thread(&Scheduler::receiveMessages, this).detach();
        std::thread(&Scheduler::processRequests, this).join();
    }

private:
    void receiveMessages() {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in senderAddr;
        socklen_t len = sizeof(senderAddr);

        while (true) {
            int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&senderAddr, &len);
            if (n < 0) {
                perror("[Scheduler] recvfrom() failed");
                continue;
            }
            buffer[n] = '\0';

            handleMessage(buffer);
        }
    }

    void handleMessage(const std::string& message) {
        std::stringstream ss(message);
        std::string type;
        ss >> type;

        if (type == "STATUS" || type == "FAULT" || type == "WARNING") {
            handleElevatorUpdate(ss, type);
        } else {
            handleClientRequest(ss, type);
        }
    }

    void handleElevatorUpdate(std::stringstream& ss, const std::string& type) {
        int elevatorID;
        if (type == "STATUS") {
            int floor;
            ss >> elevatorID >> floor;
            elevatorFloors[elevatorID] = floor;
            elevatorStatus[elevatorID] = true;
            std::cout << "[Scheduler] Elevator " << elevatorID << " is at Floor " << floor << std::endl;
        } else if (type == "FAULT") {
            ss >> elevatorID;
            elevatorStatus[elevatorID] = false;
            std::cerr << "[Scheduler] HARD FAULT: Elevator " << elevatorID << " is inactive." << std::endl;
        } else if (type == "WARNING") {
            std::string warning;
            ss >> elevatorID >> warning;
            std::cerr << "[Scheduler] WARNING from Elevator " << elevatorID << ": " << warning << std::endl;
        }
    }

    void handleClientRequest(std::stringstream& ss, const std::string& firstToken) {
        int floor = std::stoi(firstToken);
        std::string direction;
        int targetFloor;
        ss >> direction >> targetFloor;

        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.emplace(floor, targetFloor, direction);
        std::cout << "[Scheduler] Client Request: Floor " << floor << " -> Floor " << targetFloor << " (" << direction << ")" << std::endl;
        cv.notify_one();
    }

    void processRequests() {
        while (true) {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] { return !requestQueue.empty(); });

            Request req = requestQueue.front();
            requestQueue.pop();
            lock.unlock();

            int assignedElevator = findNearestElevator(req);
            if (assignedElevator != -1) {
                sendCommandToElevator(assignedElevator, req);
            } else {
                std::cerr << "[Scheduler] No active elevators available." << std::endl;
            }
        }
    }

    int findNearestElevator(const Request& req) {
        int bestElevator = -1, minDist = INT_MAX;
        for (const auto& [id, floor] : elevatorFloors) {
            if (!elevatorStatus[id]) continue;
            int dist = std::abs(floor - req.floor);
            if (dist < minDist) {
                minDist = dist;
                bestElevator = id;
            }
        }
        return bestElevator;
    }

    void sendCommandToElevator(int elevatorID, const Request& req) {
        struct sockaddr_in destAddr = {AF_INET, htons(BASE_PORT + elevatorID), inet_addr("127.0.0.1")};
        std::string command = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(req.targetFloor);

        if (sendto(sockfd, command.c_str(), command.size(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr)) >= 0) {
            std::cout << "[Scheduler] Sent command to Elevator " << elevatorID << ": " << command << std::endl;
        } else {
            perror("[Scheduler] Failed to send command");
        }
    }

    ~Scheduler() { close(sockfd); }
};

int main() {
    Scheduler scheduler(3);
    scheduler.start();
    return 0;
}
