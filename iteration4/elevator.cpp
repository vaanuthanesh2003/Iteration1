#include "elevator.h"
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <iostream>

#define BASE_PORT 5100
#define MOVE_TIMEOUT 10  // Timeout in seconds for floor movement
#define DOOR_RETRY_LIMIT 3  // Number of retries for stuck door

Elevator::Elevator(int elevatorID) : id(elevatorID), currentFloor(0) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Elevator] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in selfAddr;
    memset(&selfAddr, 0, sizeof(selfAddr));
    selfAddr.sin_family = AF_INET;
    selfAddr.sin_addr.s_addr = INADDR_ANY;
    selfAddr.sin_port = htons(BASE_PORT + id);

    if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("[Elevator] Bind failed");
        exit(EXIT_FAILURE);
    }

    schedulerAddr.sin_family = AF_INET;
    schedulerAddr.sin_port = htons(5002);
    schedulerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "[Elevator " << id << "] Initialized on port " << (BASE_PORT + id) << std::endl;
}

void Elevator::receiveCommand() {
    char buffer[1024];
    struct sockaddr_in srcAddr;
    socklen_t addrLen = sizeof(srcAddr);

    std::cout << "[Elevator " << id << "] Waiting for command on port " << (BASE_PORT + id) << "..." << std::endl;

    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&srcAddr, &addrLen);
    if (n < 0) {
        perror("[Elevator] recvfrom() failed");
        return;
    }

    buffer[n] = '\0';

    int receivedID, targetFloor;
    if (sscanf(buffer, "MOVE %d %d", &receivedID, &targetFloor) == 2) {
        if (receivedID == this->id) {
            std::cout << "[Elevator " << id << "] Received move command to Floor " << targetFloor << std::endl;
            moveTo(targetFloor);
        }
    } else {
        std::cerr << "[Elevator " << id << "] Error: Invalid command format -> " << buffer << std::endl;
    }
}

void Elevator::moveTo(int floor) {
    std::cout << "[Elevator " << id << "] Doors closing..." << std::endl;
    
    int retryCount = 0;
    while (retryCount < DOOR_RETRY_LIMIT) {
        sleep(1);
        if (rand() % 10 < 8) {  // Simulating an 80% chance of successful door closure
            break;
        }
        std::cerr << "[Elevator " << id << "] Warning: Door failed to close, retrying..." << std::endl;
        retryCount++;
    }

    if (retryCount == DOOR_RETRY_LIMIT) {
        std::cerr << "[Elevator " << id << "] Warning: Door was stuck but finally closed." << std::endl;
        sendFaultMessage("WARNING " + std::to_string(id) + " DOOR_STUCK");
    }

    auto startTime = time(nullptr);
    if (floor > currentFloor) {
        for (int f = currentFloor + 1; f <= floor; f++) {
            std::cout << "[Elevator " << id << "] Moving up... Floor " << f << std::endl;
            sleep(1);
            if (time(nullptr) - startTime > MOVE_TIMEOUT) {
                reportHardFault();
                return;
            }
        }
    } else if (floor < currentFloor) {
        for (int f = currentFloor - 1; f >= floor; f--) {
            std::cout << "[Elevator " << id << "] Moving down... Floor " << f << std::endl;
            sleep(1);
            if (time(nullptr) - startTime > MOVE_TIMEOUT) {
                reportHardFault();
                return;
            }
        }
    }

    currentFloor = floor;
    std::cout << "[Elevator " << id << "] Doors opening..." << std::endl;
    sleep(1);
    std::cout << "[Elevator " << id << "] Arrived at Floor " << currentFloor << std::endl;
}

void Elevator::sendStatus() {
    std::string statusMessage = "STATUS " + std::to_string(id) + " " + std::to_string(currentFloor);
    sendto(sockfd, statusMessage.c_str(), statusMessage.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
    std::cout << "[Elevator " << id << "] Sent status: Floor " << currentFloor << std::endl;
}

void Elevator::sendFaultMessage(const std::string& message) {
    sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
    std::cout << "[Elevator " << id << "] Sent fault message: " << message << std::endl;
}

void Elevator::reportHardFault() {
    std::cerr << "[Elevator " << id << "] ERROR: Movement timeout. Assuming elevator is stuck." << std::endl;
    sendFaultMessage("FAULT " + std::to_string(id));
    exit(EXIT_FAILURE);
}

Elevator::~Elevator() {
    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <elevator_id>" << std::endl;
        return 1;
    }

    int elevatorID = std::atoi(argv[1]);
    if (elevatorID <= 0) {
        std::cerr << "[Error] Invalid elevator ID. Must be a positive number." << std::endl;
        return 1;
    }

    Elevator elevator(elevatorID);

    while (true) {
        elevator.receiveCommand();
        elevator.sendStatus();
        sleep(2);
    }

    return 0;
}
