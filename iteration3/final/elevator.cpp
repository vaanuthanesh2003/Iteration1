#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "elevator.h"
#include <thread>
#define SCHEDULER_IP "127.0.0.1"
#define SCHEDULER_PORT 5002  // Matches the scheduler's port
#define BASE_PORT 5100 // Elevators bind from 5101, 5102, etc.
#define BUFFER_SIZE 1024
#define DOOR_WAIT_TIME 2  // Simulated door open/close time

#include "elevator.h"

//Constructor: Initializes elevator and sets up UDP socket
Elevator::Elevator(int elevator_id) : id(elevator_id), currentFloor(0) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Elevator] Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind the elevator to a unique port
    memset(&selfAddr, 0, sizeof(selfAddr));
    selfAddr.sin_family = AF_INET;
    selfAddr.sin_addr.s_addr = INADDR_ANY;
    selfAddr.sin_port = htons(BASE_PORT + id);

    if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("[Elevator] Bind failed");
        exit(EXIT_FAILURE);
    }

    // Configure the scheduler address
    memset(&schedulerAddr, 0, sizeof(schedulerAddr));
    schedulerAddr.sin_family = AF_INET;
    schedulerAddr.sin_port = htons(SCHEDULER_PORT);

    if (inet_pton(AF_INET, SCHEDULER_IP, &schedulerAddr.sin_addr) <= 0) {
        perror("[Elevator] Invalid scheduler address");
        exit(EXIT_FAILURE);
    }

    std::cout << "[Elevator " << id << "] Initialized on port " << BASE_PORT + id << std::endl;
}

//Virtual destructor
Elevator::~Elevator() {
    close(sockfd);
    std::cout << "[Elevator " << id << "] Shutting down..." << std::endl;
}

//Send status update to scheduler
void Elevator::sendStatus() {
    std::string message = "STATUS " + std::to_string(this->id) + " " + std::to_string(this->currentFloor);
    sendto(sockfd, message.c_str(), message.size(), 0,
           (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));

    std::cout << "[Elevator " << this->id << "] Sending status" << std::endl;
}

//Receive command from scheduler
void Elevator::receiveCommand() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in srcAddr;
    socklen_t addrLen = sizeof(srcAddr);
    
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&srcAddr, &addrLen);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::cerr << "[Elevator " << id << "] Warning: No command received (timeout reached." << std::endl;
        } else {
            perror("[Elevator] recvfrom() failed");
        }
        return;
    }

    buffer[n] = '\0';
    //std::cout << "[DEBUG] [Elevator " << id << "] Raw message received: " << buffer << std::endl;

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

//Move elevator smoothly floor-by-floor
void Elevator::moveTo(int targetFloor) {
    std::cout << "[Elevator " << id << "] Arrived at Floor " << currentFloor << " - Doors Opening" << std::endl;
    sleep(DOOR_WAIT_TIME);
    std::cout << "[Elevator " << id << "] Doors Closing" << std::endl;

    std::cout << "[Elevator " << id << "] Moving from " << currentFloor << " to " << targetFloor << std::endl;

    while (currentFloor != targetFloor) {
        sleep(1);  // Simulates gradual movement
        if (currentFloor < targetFloor)
            currentFloor++;
        else
            currentFloor--;

        std::cout << "[Elevator " << id << "] Passing Floor " << currentFloor << std::endl;
    }

    std::cout << "[Elevator " << id << "] Arrived at Floor " << currentFloor << std::endl;
    sendStatus();  // Notify scheduler of new floor
}

//Main run loop for receiving commands
void Elevator::run() {
    while (true) {
        receiveCommand();
    }
}

#ifndef TEST_BUILD
int main(int argc, char* argv[]) {
    if (argc != 2) {
    std::cerr << "Usage: ./elevator <levator_id> " << std::endl;
    return EXIT_FAILURE;
    }
    
    int elevatorID = atoi(argv[1]);
    if (elevatorID <= 0) {
    std::cerr << "Error: Elevator ID must be a positive integer. " << std::endl;
    return EXIT_FAILURE;
   }
   
   Elevator elevator(elevatorID);
   
   std::cout << "[Main] Starting Elevator " << elevatorID << " ... " << std::endl;
   
   std::thread elevatorThread(&Elevator::run, &elevator);
   
   elevatorThread.join();
   return EXIT_SUCCESS;
   }
  #endif
