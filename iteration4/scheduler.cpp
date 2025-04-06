#include "scheduler.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <climits>
#include <unistd.h>
#include <arpa/inet.h>

Scheduler::Scheduler(int elevCount, int floorMax) : elevatorCount(elevCount), floorCount(floorMax) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
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
        elevatorLoad[i] = 0;
        elevatorStatus[i] = "OK";
    }

    std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
}

Scheduler::~Scheduler() {
    close(sockfd);
}

void Scheduler::start() {
    startTime = std::chrono::steady_clock::now();
    std::thread(&Scheduler::receiveMessages, this).detach();
    std::thread(&Scheduler::processRequests, this).detach();
    std::thread(&Scheduler::displayStatus, this).detach();

    while (true) std::this_thread::sleep_for(std::chrono::seconds(60));
}

void Scheduler::receiveMessages() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while (true) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) continue;
        buffer[n] = '\0';

        std::stringstream ss(buffer);
        std::string type;
        ss >> type;

        std::cout << "[Scheduler] Received: " << buffer << std::endl;

        if (type == "STATUS") {
            int id, floor;
            ss >> id >> floor;
            elevatorFloors[id] = floor;
            elevatorLoad[id] = std::max(0, elevatorLoad[id] - 1);
            auto now = std::chrono::steady_clock::now();
            if (warningTimestamps.count(id) == 0 ||
                std::chrono::duration_cast<std::chrono::seconds>(now - warningTimestamps[id]).count() > 5) {
                elevatorStatus[id] = "REACHED";
            }
        } else if (type == "FAULT") {
            int id;
            ss >> id;
            elevatorStatus[id] = "FAULT";
        } else if (type == "WARNING") {
            int id; std::string warning;
            ss >> id >> warning;
            elevatorStatus[id] = "WARNING(" + warning + ")";
            warningTimestamps[id] = std::chrono::steady_clock::now();
        } else {
            handleClientRequest(ss, type);
        }
    }
}

void Scheduler::handleClientRequest(std::stringstream& ss, const std::string& firstToken) {
    int floor = std::stoi(firstToken);
    std::string direction;
    int targetFloor;
    ss >> direction >> targetFloor;

    std::lock_guard<std::mutex> lock(queueMutex);
    std::cout << "[Scheduler] Queued request: " << floor << " -> " << targetFloor << " (" << direction << ")" << std::endl;
    requestQueue.emplace(floor, targetFloor, direction);
    cv.notify_one();
}

void Scheduler::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this] { return !requestQueue.empty(); });

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        int elevatorID = findBestElevator(req);
        if (elevatorID != -1) {
            sendMoveCommand(elevatorID, req.targetFloor);
            moveCount++;
            requestsHandled++;
            elevatorLoad[elevatorID]++;
        } else {
            std::cout << "[Scheduler] No elevator available, requeuing request\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::lock_guard<std::mutex> relock(queueMutex);
            requestQueue.push(req);
            cv.notify_one();
        }
    }
}

int Scheduler::findBestElevator(const Request& req) {
    int best = -1, minDist = INT_MAX;
    for (int i = 1; i <= elevatorCount; ++i) {
        if (elevatorStatus[i] != "OK" && elevatorStatus[i] != "REACHED") continue;
        if (elevatorLoad[i] >= MAX_CAPACITY) continue;
        int dist = std::abs(elevatorFloors[i] - req.floor);
        if (dist < minDist) {
            minDist = dist;
            best = i;
        }
    }
    return best;
}

void Scheduler::sendMoveCommand(int elevatorID, int targetFloor) {
    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(BASE_PORT + elevatorID);
    destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::string cmd = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(targetFloor);
    sendto(sockfd, cmd.c_str(), cmd.length(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));

    std::cout << "[Scheduler] Sent command to Elevator " << elevatorID << ": MOVE to " << targetFloor << std::endl;

    elevatorStatus[elevatorID] = "MOVING";
    elevatorFloors[elevatorID] = -1;
}

void Scheduler::displayStatus() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::lock_guard<std::mutex> lock(queueMutex);
        std::cout << "\n=== Elevator System Status ===\n";
        for (int i = 1; i <= elevatorCount; ++i) {
            std::cout << "Elevator " << i << ": Floor=" << elevatorFloors[i]
                      << ", Load=" << elevatorLoad[i]
                      << ", Status=" << elevatorStatus[i] << std::endl;
        }
        std::cout << "Requests in queue: " << requestQueue.size() << std::endl;
        std::cout << "===============================\n";
    }
}

#ifndef TEST_BUILD
int main() {
    int elevators, floors;
    std::cout << "Enter number of elevators: ";
    std::cin >> elevators;
    std::cout << "Enter number of floors: ";
    std::cin >> floors;

    Scheduler scheduler(elevators, floors);
    scheduler.start();
    return 0;
}
#endif

