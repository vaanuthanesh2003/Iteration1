
#include "elevator.h"
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <ctime>

// Constructor: Initializes the elevator with a given ID
Elevator::Elevator(int elevatorID) : id(elevatorID), currentFloor(0), stuck(false), doorStuck(false) {
    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Elevator] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the address for this elevator to bind to (using its unique ID)
    struct sockaddr_in selfAddr;
    memset(&selfAddr, 0, sizeof(selfAddr));
    selfAddr.sin_family = AF_INET;
    selfAddr.sin_addr.s_addr = INADDR_ANY; // Listen on any available IP address
    selfAddr.sin_port = htons(BASE_PORT + id); // Use a unique port for each elevator

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("[Elevator] Bind failed");
        exit(EXIT_FAILURE);
    }

    // Set up the address for the scheduler
    memset(&schedulerAddr, 0, sizeof(schedulerAddr));
    schedulerAddr.sin_family = AF_INET;
    schedulerAddr.sin_port = htons(SCHEDULER_PORT);
    schedulerAddr.sin_addr.s_addr = inet_addr(SCHEDULER_IP);

    std::cout << "[Elevator " << id << "] Listening on port " << (BASE_PORT + id) << std::endl;
}

// Receive commands from the scheduler (e.g., move commands)
void Elevator::receiveCommand() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender;
    socklen_t len = sizeof(sender);

    // Receive data from the socket
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&sender, &len);
    if (n <= 0) return; // Return if no data or error
    buffer[n] = '\0'; // Null-terminate the received string

    int eid, targetFloor;
    // Parse the received message (e.g., "MOVE <elevatorID> <targetFloor>")
    if (sscanf(buffer, "MOVE %d %d", &eid, &targetFloor) == 2 && eid == id) {
        std::cout << "[Elevator " << id << "] Received move command to Floor " << targetFloor << std::endl;
        moveTo(targetFloor); // Call move function if the command is for this elevator
    }
}

// Simulate moving the elevator to a target floor
void Elevator::moveTo(int floor) {
    std::cout << "[Elevator " << id << "] Doors closing..." << std::endl;
    sleep(1); // Simulate door closing delay

    time_t startTime = time(nullptr); // Record the start time for fault detection

    // Simulate fault conditions
    if (floor == 99) {
        std::cerr << "[Elevator " << id << "] Simulated HARD FAULT triggered. " << std::endl;
        reportHardFault(); // Trigger hard fault if the floor is 99
        return;
    }

    if (floor == 77) {
        std::cerr << "[Elevator " << id << "] Simulated WARNING Door Stuck. " << std::endl;
        sendFaultMessage("WARNING " + std::to_string(id) + " door stuck"); // Send a warning message
        return;
    }

    // Move up or down to the target floor
    if (floor > currentFloor) {
        for (int f = currentFloor + 1; f <= floor; ++f) {
            std::cout << "[Elevator " << id << "] Moving up... Floor " << f << std::endl;
            sleep(1); // Simulate moving up with a delay
            if (time(nullptr) - startTime > 10) { // Timeout after 10 seconds to simulate a fault
                reportHardFault();
                return;
            }
        }
    } else {
        for (int f = currentFloor - 1; f >= floor; --f) {
            std::cout << "[Elevator " << id << "] Moving down... Floor " << f << std::endl;
            sleep(1); // Simulate moving down with a delay
            if (time(nullptr) - startTime > 10) { // Timeout after 10 seconds to simulate a fault
                reportHardFault();
                return;
            }
        }
    }

    // Update the current floor and print the result
    currentFloor = floor;
    std::cout << "[Elevator " << id << "] Arrived at Floor " << currentFloor << std::endl;
    sleep(1); // Simulate the elevator stopping at the target floor
}

// Send the elevator's current status to the scheduler
void Elevator::sendStatus() {
    std::string msg = "STATUS " + std::to_string(id) + " " + std::to_string(currentFloor);
    sendto(sockfd, msg.c_str(), msg.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
}

// Return the current floor of the elevator
int Elevator::getCurrentFloor() const {
    return currentFloor;
}

// Return the elevator's ID
int Elevator::getID() const {
    return id;
}

// Send a fault message to the scheduler
void Elevator::sendFaultMessage(const std::string& msg) {
    sendto(sockfd, msg.c_str(), msg.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
}

// Handle a hard fault scenario (e.g., movement timeout)
void Elevator::reportHardFault() {
    std::cerr << "[Elevator " << id << "] HARD FAULT: Movement timeout. Shutting down." << std::endl;
    sendFaultMessage("FAULT " + std::to_string(id)); // Send fault message to scheduler
    exit(EXIT_FAILURE); // Exit the program on hard fault
}

// Destructor: Close the socket when the elevator object is destroyed
Elevator::~Elevator() {
    close(sockfd);
}

#ifndef TEST_BUILD
// Main function: Create an elevator object and continuously receive commands
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./elevator <id>" << std::endl;
        return 1; // Exit if the elevator ID is not provided
    }

    Elevator elevator(std::atoi(argv[1])); // Create an elevator with the given ID
    while (true) {
        elevator.receiveCommand(); // Receive and process commands from the scheduler
        elevator.sendStatus(); // Send the elevator's status to the scheduler
        sleep(2); // Wait for a short time before receiving the next command
    }
    return 0;
}
#endif
