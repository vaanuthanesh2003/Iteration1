#include "client.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>

#define SCHEDULER_PORT 5002 // Port number for the scheduler
#define SCHEDULER_IP "127.0.0.1" // IP address of the scheduler

// Client constructor: Initializes the socket and scheduler address
Client::Client() {
    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Client] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the scheduler address structure
    memset(&schedulerAddr, 0, sizeof(schedulerAddr)); // Clear memory for the address struct
    schedulerAddr.sin_family = AF_INET; // Use IPv4 address family
    schedulerAddr.sin_port = htons(SCHEDULER_PORT); // Set the port number (network byte order)
    schedulerAddr.sin_addr.s_addr = inet_addr(SCHEDULER_IP); // Convert the IP address from string to binary
}

// Convert a timestamp (hh:mm:ss) to seconds
int Client::getSecondsFromTimestamp(const std::string& timestamp) {
    int h, m, s;
    // Parse the timestamp string into hours, minutes, and seconds
    sscanf(timestamp.c_str(), "%d:%d:%d", &h, &m, &s);
    // Return the total number of seconds since midnight
    return h * 3600 + m * 60 + s;
}

// Send a request to the scheduler with the given floor, direction, and target floor
void Client::sendRequest(int floor, std::string direction, int targetFloor) {
    // Format the message to be sent
    std::string message = std::to_string(floor) + " " + direction + " " + std::to_string(targetFloor);
    // Send the message via UDP to the scheduler
    sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
    // Print the sent request to the console
    std::cout << "[Client] Sent request: Floor " << floor << " -> Floor " << targetFloor << " (" << direction << ")" << std::endl;
}

// Process requests from an input file
void Client::processRequestsFromFile(const std::string& filename) {
    // Open the file containing requests
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[Client] Error: Unable to open input file: " << filename << std::endl;
        return;
    }

    std::string line;
    auto startTime = std::chrono::steady_clock::now(); // Get the start time for timing the requests

    // Process each line in the input file
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string timestamp, direction;
        int floor, targetFloor;

        // Parse the line to extract the timestamp, floor, direction, and target floor
        if (!(iss >> timestamp >> floor >> direction >> targetFloor)) {
            std::cerr << "[Client] Error: Invalid request format -> " << line << std::endl;
            continue;
        }

        // Calculate the delay before sending the request based on the timestamp
        int delay = getSecondsFromTimestamp(timestamp);
        std::this_thread::sleep_until(startTime + std::chrono::seconds(delay)); // Sleep until the appropriate time

        // Send the request to the scheduler
        sendRequest(floor, direction, targetFloor);
    }

    // Close the file after processing all requests
    file.close();
}

// Destructor: Close the socket
Client::~Client() {
    close(sockfd);
}

#ifndef TEST_BUILD
// Main function: Create a client and process requests from an input file
int main() {
    Client client;
    client.processRequestsFromFile("input.txt"); // Process requests from 'input.txt'
    return 0;
}
#endif

