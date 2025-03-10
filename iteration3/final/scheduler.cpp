#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <climits>
#include "scheduler.h"


#define CLIENT_PORT 5001 // Receives requests from client
#define SCHEDULER_PORT 5002 // Scheduler listens on this port
#define BASE_ELEVATOR_PORT 5100 // Elevators start binding from 5101, 5102, etc.
#define BUFFER_SIZE 1024


// Initializes the scheduler
Scheduler::Scheduler(int numElevators) {
    for (int i = 1; i <= numElevators; i++) {
        elevatorPositions[i] = 0; // Start all elevators at floor 0
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    memset(&selfAddr, 0, sizeof(selfAddr));
    selfAddr.sin_family = AF_INET;
    selfAddr.sin_addr.s_addr = INADDR_ANY;
    selfAddr.sin_port = htons(SCHEDULER_PORT);

    if (bind(sockfd, (const struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(CLIENT_PORT);
    clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "[Scheduler] Initialized and listening on port " << SCHEDULER_PORT << std::endl;
}

// Receives requests from `client.cpp`
void Scheduler::receiveRequests() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while (true) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) continue;

        buffer[n] = '\0';

        int floor, targetFloor;
        char direction[10];
        if (sscanf(buffer, "%d %s %d", &floor, direction, &targetFloor) == 3) {
            std::lock_guard<std::mutex> lock(queueMutex);
            requestQueue.push(Request(floor, targetFloor, std::string(direction)));
            }
            cv.notify_one();
            std::cout << "[Scheduler] New request: Floor " << floor << " -> Floor " << targetFloor
                      << " (" << direction << ")" << std::endl;
        }
    }

// Processes requests and assigns elevators
void Scheduler::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this] {return !requestQueue.empty(); });
        
        if (requestQueue.empty()) continue;

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        int assignedElevator = assignElevator(req);
        if (assignedElevator != -1) {
            sendCommandToElevator(assignedElevator, req);
        } else {
            std::cerr << "[Scheduler] No available elevators, retrying ... " << std::endl;
            sleep(1);
        }
    }
}

// Assigns the best available elevator
int Scheduler::assignElevator(const Request& req) {
    int bestElevator = -1;
    int minDistance = INT_MAX;
    bool foundIdleElevator = false;

    std::cout << "[DEBUG] [Scheduler] Evaluating elevators for request: Floor " << req.floor
              << " to " << req.targetFloor << std::endl << std::flush;

    for (auto& [id, floor] : elevatorPositions) {
        int distance = abs(floor - req.floor);
       
        std::cout << "[DEBUG] [Scheduler] Checking Elevator " << id << " (Currently at Floor " << floor
                  << ", Distance " << distance << ")" << std::endl << std::flush;

        // Prefer an idle elevator close to the requested floor
        if (distance < minDistance) {
            bestElevator = id;
            minDistance = distance;
        }
    }

    std::cout << "[DEBUG] [Scheduler] Assigned Elevator " << bestElevator << " to handle request." << std::endl << std::flush;
   
    return bestElevator;
}


// Sends movement command to an elevator
void Scheduler::sendCommandToElevator(int elevatorID, const Request& req) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "MOVE %d %d", elevatorID, req.targetFloor);

    memset(&elevatorAddr, 0, sizeof(elevatorAddr));
    elevatorAddr.sin_family = AF_INET;
    elevatorAddr.sin_port = htons(BASE_ELEVATOR_PORT + elevatorID);
    elevatorAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&elevatorAddr, sizeof(elevatorAddr));

    std::cout << "[Scheduler] Assigned Elevator " << elevatorID << ": " << message << std::endl << std::flush;
    std::cout << "[DEBUG] [Scheduler] sent command to elevator: " << message << "ElevatorID" << elevatorID << " on base port: " << BASE_ELEVATOR_PORT+elevatorID << std::endl << std::flush;
}

// Receives elevator position updates

void Scheduler::receiveElevatorUpdates() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while (true) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) continue;

        buffer[n] = '\0';

        int elevatorID, floor;
        if (sscanf(buffer, "STATUS %d %d", &elevatorID, &floor) == 2) {
            elevatorPositions[elevatorID] = floor;  // Update elevator floor position
            std::cout << "[DEBUG] [Scheduler] Elevator " << elevatorID
                      << " updated to Floor " << floor << std::endl << std::flush;
        }
    }
}



// Runs the scheduler with threads for handling requests
void Scheduler::run() {
    std::thread requestThread(&Scheduler::receiveRequests, this);
    std::thread processThread(&Scheduler::processRequests, this);
    std::thread updateThread(&Scheduler::receiveElevatorUpdates, this);

    requestThread.join();
    processThread.join();
    updateThread.join();
}

#ifndef TEST_BUILD
int main() {
    Scheduler scheduler(3);
    scheduler.run();
    return 0;
}
#endif
