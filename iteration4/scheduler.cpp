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
#include <cstring>
#include <sstream>
#include <climits>

#define SCHEDULER_PORT 5002
#define BASE_PORT 5100
#define BUFFER_SIZE 1024

struct Request {
    int floor;
    std::string direction;
    int targetFloor;

    Request(int f, int t, std::string d) : floor(f), direction(d), targetFloor(t) {}
};

class Scheduler {
private:
    int sockfd;
    struct sockaddr_in selfAddr;
    std::unordered_map<int, int> elevatorFloorMap;       // elevatorID -> currentFloor
    std::unordered_map<int, bool> elevatorActiveMap;     // elevatorID -> isActive
    std::queue<Request> requestQueue;
    std::mutex queueMutex;
    std::condition_variable cv;

public:
    Scheduler(int elevatorCount) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("[Scheduler] Socket creation failed");
            exit(EXIT_FAILURE);
        }

        memset(&selfAddr, 0, sizeof(selfAddr));
        selfAddr.sin_family = AF_INET;
        selfAddr.sin_addr.s_addr = INADDR_ANY;
        selfAddr.sin_port = htons(SCHEDULER_PORT);

        if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
            perror("[Scheduler] Bind failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 1; i <= elevatorCount; ++i) {
            elevatorFloorMap[i] = 0;
            elevatorActiveMap[i] = true;
        }

        std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
    }

    void start() {
        std::thread requestReceiver(&Scheduler::receiveMessages, this);
        std::thread requestProcessor(&Scheduler::processRequests, this);
        requestReceiver.join();
        requestProcessor.join();
    }

    void receiveMessages() {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in senderAddr;
        socklen_t len = sizeof(senderAddr);

        while (true) {
            int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                             (struct sockaddr*)&senderAddr, &len);
            if (n < 0) {
                perror("[Scheduler] recvfrom() failed");
                continue;
            }

            buffer[n] = '\0';

            std::stringstream ss(buffer);
            std::string token;
            ss >> token;

            if (token == "STATUS" || token == "FAULT" || token == "WARNING") {
                int elevatorID;
                if (token == "STATUS") {
                    int floor;
                    ss >> elevatorID >> floor;
                    elevatorFloorMap[elevatorID] = floor;
                    elevatorActiveMap[elevatorID] = true;
                    std::cout << "[Scheduler] Elevator " << elevatorID << " is at Floor " << floor << std::endl;
                } else if (token == "FAULT") {
                    ss >> elevatorID;
                    elevatorActiveMap[elevatorID] = false;
                    std::cerr << "[Scheduler] HARD FAULT reported by Elevator " << elevatorID << ". Marking as inactive." << std::endl;
                } else if (token == "WARNING") {
                    std::string warningType;
                    ss >> elevatorID >> warningType;
                    std::cerr << "[Scheduler] WARNING from Elevator " << elevatorID << ": " << warningType << std::endl;
                }
            } else {
                // Client request assumed format: floor direction targetFloor
                int floor = std::stoi(token);
                std::string direction;
                int targetFloor;
                ss >> direction >> targetFloor;

                Request req(floor, targetFloor, direction);
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    requestQueue.push(req);
                    std::cout << "[Scheduler] Received client request: Floor " << req.floor << " -> Floor " << req.targetFloor << " (" << req.direction << ")" << std::endl;
                }
                cv.notify_one();
            }
        }
    }

    void processRequests() {
        while (true) {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this]() { return !requestQueue.empty(); });
            Request req = requestQueue.front();
            requestQueue.pop();
            lock.unlock();

            int assignedElevator = assignElevator(req);
            if (assignedElevator != -1) {
                sendCommandToElevator(assignedElevator, req);
            } else {
                std::cerr << "[Scheduler] No active elevators available for request." << std::endl;
            }
        }
    }

    int assignElevator(const Request& req) {
        int minDistance = INT_MAX;
        int selectedElevator = -1;
        for (const auto& entry : elevatorFloorMap) {
            int eid = entry.first;
            if (!elevatorActiveMap[eid]) continue;
            int distance = abs(entry.second - req.floor);
            if (distance < minDistance) {
                minDistance = distance;
                selectedElevator = eid;
            }
        }
        return selectedElevator;
    }

    void sendCommandToElevator(int elevatorID, const Request& req) {
        struct sockaddr_in destAddr;
        memset(&destAddr, 0, sizeof(destAddr));
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(BASE_PORT + elevatorID);
        destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        std::string command = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(req.targetFloor);
        int sent = sendto(sockfd, command.c_str(), command.length(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
        if (sent >= 0) {
            std::cout << "[Scheduler] Sent command to Elevator " << elevatorID << ": " << command << std::endl;
        } else {
            perror("[Scheduler] Failed to send command");
        }
    }

    ~Scheduler() {
        close(sockfd);
    }
};

int main() {
    Scheduler scheduler(3);  // 3 elevators
    scheduler.start();
    return 0;
}

