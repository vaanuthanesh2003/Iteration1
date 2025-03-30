#include "elevator.h"
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <ctime>

Elevator::Elevator(int elevatorID) : id(elevatorID), currentFloor(0), stuck(false), doorStuck(false) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Elevator] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in selfAddr;
    memset(&selfAddr, 0, sizeof(selfAddr));
    selfAddr.sin_family = AF_INET;
    selfAddr.sin_addr.s_addr = INADDR_ANY;
    selfAddr.sin_port = htons(BASE_PORT + id);

    if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("[Elevator] Bind failed");
        exit(EXIT_FAILURE);
    }

    memset(&schedulerAddr, 0, sizeof(schedulerAddr));
    schedulerAddr.sin_family = AF_INET;
    schedulerAddr.sin_port = htons(SCHEDULER_PORT);
    schedulerAddr.sin_addr.s_addr = inet_addr(SCHEDULER_IP);

    std::cout << "[Elevator " << id << "] Listening on port " << (BASE_PORT + id) << std::endl;
}

void Elevator::receiveCommand() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender;
    socklen_t len = sizeof(sender);

    int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&sender, &len);
    if (n <= 0) return;
    buffer[n] = '\0';

    int eid, targetFloor;
    if (sscanf(buffer, "MOVE %d %d", &eid, &targetFloor) == 2 && eid == id) {
        std::cout << "[Elevator " << id << "] Received move command to Floor " << targetFloor << std::endl;
        moveTo(targetFloor);
    }
}

void Elevator::moveTo(int floor) {
    std::cout << "[Elevator " << id << "] Doors closing..." << std::endl;
    sleep(1);

    time_t startTime = time(nullptr);

    if (floor > currentFloor) {
        for (int f = currentFloor + 1; f <= floor; ++f) {
            std::cout << "[Elevator " << id << "] Moving up... Floor " << f << std::endl;
            sleep(1);
            if (time(nullptr) - startTime > 10) {
                reportHardFault();
                return;
            }
        }
    } else {
        for (int f = currentFloor - 1; f >= floor; --f) {
            std::cout << "[Elevator " << id << "] Moving down... Floor " << f << std::endl;
            sleep(1);
            if (time(nullptr) - startTime > 10) {
                reportHardFault();
                return;
            }
        }
    }

    currentFloor = floor;
    std::cout << "[Elevator " << id << "] Arrived at Floor " << currentFloor << std::endl;
    sleep(1);
}

void Elevator::sendStatus() {
    std::string msg = "STATUS " + std::to_string(id) + " " + std::to_string(currentFloor);
    sendto(sockfd, msg.c_str(), msg.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
}

void Elevator::sendFaultMessage(const std::string& msg) {
    sendto(sockfd, msg.c_str(), msg.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
}

void Elevator::reportHardFault() {
    std::cerr << "[Elevator " << id << "] HARD FAULT: Movement timeout. Shutting down." << std::endl;
    sendFaultMessage("FAULT " + std::to_string(id));
    exit(EXIT_FAILURE);
}

Elevator::~Elevator() {
    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./elevator <id>" << std::endl;
        return 1;
    }

    Elevator elevator(std::atoi(argv[1]));
    while (true) {
        elevator.receiveCommand();
        elevator.sendStatus();
        sleep(2);
    }
    return 0;
}
