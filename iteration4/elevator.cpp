#include "elevator.h"
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <iostream>

#define BASE_PORT 5100
#define MOVE_TIMEOUT 10  // Timeout in seconds for floor movement
#define DOOR_RETRY_LIMIT 3  // Number of retries for stuck door

// Constructor: Initializes the elevator with the given elevatorID
Elevator::Elevator(int elevatorID) : id(elevatorID), currentFloor(0) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Create a UDP socket
    if (sockfd < 0) {
        perror("[Elevator] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  // Set socket options

    struct sockaddr_in selfAddr;
    memset(&selfAddr, 0, sizeof(selfAddr));  // Initialize address structure
    selfAddr.sin_family = AF_INET;
    selfAddr.sin_addr.s_addr = INADDR_ANY;  // Accept any incoming connection
    selfAddr.sin_port = htons(BASE_PORT + id);  // Set port based on elevator ID

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("[Elevator] Bind failed");
        exit(EXIT_FAILURE);
    }

    // Set up the scheduler's address for communication
    schedulerAddr.sin_family = AF_INET;
    schedulerAddr.sin_port = htons(5002);  // Scheduler's fixed port
    schedulerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Scheduler's IP address

    std::cout << "[Elevator " << id << "] Initialized on port " << (BASE_PORT + id) << std::endl;
}

// Receives a command from the scheduler or other systems
void Elevator::receiveCommand() {
    char buffer[1024];
    struct sockaddr_in srcAddr;
    socklen_t addrLen = sizeof(srcAddr);

    std::cout << "[Elevator " << id << "] Waiting for command on port " << (BASE_PORT + id) << "..." << std::endl;

    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&srcAddr, &addrLen);  // Receive command
    if (n < 0) {
        perror("[Elevator] recvfrom() failed");
        return;
    }

    buffer[n] = '\0';  // Null-terminate the received message

    int receivedID, targetFloor;
    // Parse the received command (expected format: "MOVE <elevatorID> <targetFloor>")
    if (sscanf(buffer, "MOVE %d %d", &receivedID, &targetFloor) == 2) {
        // If the command is for this elevator, execute the move
        if (receivedID == this->id) {
            std::cout << "[Elevator " << id << "] Received move command to Floor " << targetFloor << std::endl;
            moveTo(targetFloor);
        }
    } else {
        std::cerr << "[Elevator " << id << "] Error: Invalid command format -> " << buffer << std::endl;
    }
}

// Moves the elevator to the target floor
void Elevator::moveTo(int floor) {
    std::cout << "[Elevator " << id << "] Doors closing..." << std::endl;
    
    int retryCount = 0;
    while (retryCount < DOOR_RETRY_LIMIT) {
        sleep(1);
        if (rand() % 10 < 8) {  // Simulate an 80% chance of successful door closure
            break;
        }
        std::cerr << "[Elevator " << id << "] Warning: Door failed to close, retrying..." << std::endl;
        retryCount++;
    }

    // If the door fails to close after retries, send a fault message
    if (retryCount == DOOR_RETRY_LIMIT) {
        std::cerr << "[Elevator " << id << "] Warning: Door was stuck but finally closed." << std::endl;
        sendFaultMessage("WARNING " + std::to_string(id) + " DOOR_STUCK");
    }

    auto startTime = time(nullptr);
    // Moving up or down depending on the target floor
    if (floor > currentFloor) {
        for (int f = currentFloor + 1; f <= floor; f++) {
            std::cout << "[Elevator " << id << "] Moving up... Floor " << f << std::endl;
            sleep(1);
            // If the move takes longer than the allowed time, report a hard fault
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

    currentFloor = floor;  // Update the current floor
    std::cout << "[Elevator " << id << "] Doors opening..." << std::endl;
    sleep(1);  // Simulate time taken for doors to open
    std::cout << "[Elevator " << id << "] Arrived at Floor " << currentFloor << std::endl;
}

// Sends the current status of the elevator to the scheduler
void Elevator::sendStatus() {
    std::string statusMessage = "STATUS " + std::to_string(id) + " " + std::to_string(currentFloor);
    sendto(sockfd, statusMessage.c_str(), statusMessage.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
    std::cout << "[Elevator " << id << "] Sent status: Floor " << currentFloor << std::endl;
}

// Sends a fault message to the scheduler in case of an error or issue
void Elevator::sendFaultMessage(const std::string& message) {
    sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
    std::cout << "[Elevator " << id << "] Sent fault message: " << message << std::endl;
}

// Reports a hard fault (elevator stuck or other critical issues)
void Elevator::reportHardFault() {
    std::cerr << "[Elevator " << id << "] ERROR: Movement timeout. Assuming elevator is stuck." << std::endl;
    sendFaultMessage("FAULT " + std::to_string(id));
    exit(EXIT_FAILURE);  // Exit the process if a hard fault occurs
}

// Sleep for the given number of seconds (used for simulating delays)
void Elevator::sleepSec(int sec) {
    sleep(sec);
}

// Exit the elevator process with the given exit code
void Elevator::exitProcess(int code) {
    exit(code);
}

// Send a UDP message to the scheduler (used for various commands)
int Elevator::sendUDP(const std::string& msg) {
    return sendto(sockfd, msg.c_str(), msg.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
}

// Destructor: Clean up and close the socket
Elevator::~Elevator() {
    close(sockfd);
}

// Main entry point for the program (if not built for testing)
#ifndef TEST_BUILD
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

    // Initialize the elevator with the given ID
    Elevator elevator(elevatorID);

    // Continuously receive commands and send status updates
    while (true) {
        elevator.receiveCommand();  // Wait for and process commands
        elevator.sendStatus();  // Send the current status of the elevator
        sleep(2);  // Simulate a small delay before the next iteration
    }

    return 0;
}
#endif
