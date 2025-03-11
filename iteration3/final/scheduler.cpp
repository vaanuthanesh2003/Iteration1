#include "scheduler.h"
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <climits>
#define SCHEDULER_PORT 5002
#define BUFFER_SIZE 1024
#define BASE_PORT 5100

Scheduler::Scheduler(int numElevators) {
    for (int i = 1; i <= numElevators; i++) {
        elevatorPositions[i] = 0;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Scheduler] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&selfAddr, 0, sizeof(selfAddr));
    selfAddr.sin_family = AF_INET;
    selfAddr.sin_addr.s_addr = INADDR_ANY;
    selfAddr.sin_port = htons(SCHEDULER_PORT);

    if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("[Scheduler] Bind failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
}

Scheduler::~Scheduler() {
    close(sockfd);
}

void Scheduler::receiveRequests() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while (true) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) {
            perror("[Scheduler] recvfrom() failed");
            sleep(1);
            continue;
        }

        buffer[n] = '\0';
        int floor, targetFloor;
        char direction[10];

        if (sscanf(buffer, "%d %s %d", &floor, direction, &targetFloor) == 3) {
            std::lock_guard<std::mutex> lock(queueMutex);
            requestQueue.push(Request(floor, targetFloor, std::string(direction)));
            cv.notify_one();
            std::cout << "[Scheduler] Received request: Floor " << floor << " -> Floor " << targetFloor << " (" << direction << ")" << std::endl;
        }
    }
}

int Scheduler::assignElevator(const Request& req) {
    int bestElevator = -1;
    int minDistance = INT_MAX;

    for (const auto& elevator : elevatorPositions) {
        int elevatorID = elevator.first;
        int elevatorFloor = elevator.second;

        int distance = abs(elevatorFloor - req.floor);
        if (distance < minDistance) {
            minDistance = distance;
            bestElevator = elevatorID;
        }
    }

    if (bestElevator != -1) {
        elevatorPositions[bestElevator] = req.targetFloor;
        std::cout << "[Scheduler] Assigned Elevator " << bestElevator
                  << " to request: Floor " << req.floor << " -> Floor "
                  << req.targetFloor << std::endl;
    } else {
        std::cerr << "[Scheduler] No available elevators." << std::endl;
    }

    return bestElevator;
}


void Scheduler::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this] { return !requestQueue.empty(); });

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        int assignedElevator = assignElevator(req);
        if (assignedElevator != -1) {
            sendCommandToElevator(assignedElevator, req);
        }
    }
}

void Scheduler::sendCommandToElevator(int elevatorID, const Request& req) {
    struct sockaddr_in elevatorAddr;
    memset(&elevatorAddr, 0, sizeof(elevatorAddr));

    elevatorAddr.sin_family = AF_INET;
    elevatorAddr.sin_port = htons(BASE_PORT + elevatorID);  //  Elevator listens on dynamic port (e.g., 5101, 5102)
    elevatorAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::string command = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(req.targetFloor);

    int sent = sendto(sockfd, command.c_str(), command.size(), 0,
                      (struct sockaddr*)&elevatorAddr, sizeof(elevatorAddr));
   
    if (sent < 0) {
        perror("[Scheduler] sendto() failed");
    } else {
        std::cout << "[Scheduler] Sent command to Elevator " << elevatorID << " -> Move to Floor " << req.targetFloor << std::endl;
    }
}

int main() {
    Scheduler scheduler(3);
    std::thread requestThread(&Scheduler::receiveRequests, &scheduler);
    std::thread processThread(&Scheduler::processRequests, &scheduler);

    requestThread.join();
    processThread.join();

    return 0;
}
