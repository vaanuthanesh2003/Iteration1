#include "client.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define SCHEDULER_PORT 5002        // Define the port number for the scheduler
#define SCHEDULER_IP "127.0.0.1"   // Define the IP address of the scheduler (localhost)

Client::Client() {
    // Create a UDP socket using the AF_INET address family and SOCK_DGRAM type
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        // If socket creation fails, print an error and exit
        perror("[Client] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the scheduler address structure to zero
    memset(&schedulerAddr, 0, sizeof(schedulerAddr));

    // Set up the scheduler address structure with IPv4 and the appropriate port and IP
    schedulerAddr.sin_family = AF_INET;
    schedulerAddr.sin_port = htons(SCHEDULER_PORT); // Convert port to network byte order
    schedulerAddr.sin_addr.s_addr = inet_addr(SCHEDULER_IP); // Convert IP address to network byte order
}

void Client::sendRequest(int floor, std::string direction, int targetFloor) {
    // Prepare the message by concatenating floor, direction, and target floor
    std::string message = std::to_string(floor) + " " + direction + " " + std::to_string(targetFloor);

    // Send the message to the scheduler using UDP
    sendto(sockfd, message.c_str(), message.size(), 0,
           (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));

    // Print a log message to show what request has been sent
    std::cout << "[Client] Sent request: Floor " << floor << " -> Floor " << targetFloor
              << " (" << direction << ")" << std::endl;
}

void Client::processRequestsFromFile(const std::string& filename) {
    // Open the input file containing the requests
    std::ifstream file(filename);
    if (!file.is_open()) {
        // If the file can't be opened, print an error message and return
        std::cerr << "[Client] Error: Unable to open input file: " << filename << std::endl;
        return;
    }

    // Process each line in the file
    std::string line;
    while (std::getline(file, line)) {
        // Split the line into its components: timestamp, floor, direction, and targetFloor
        std::istringstream iss(line);
        std::string timestamp, direction;
        int floor, targetFloor;

        // Check if the line has the correct format
        if (!(iss >> timestamp >> floor >> direction >> targetFloor)) {
            // If the line format is invalid, print an error and continue to the next line
            std::cerr << "[Client] Error: Invalid request format -> " << line << std::endl;
            continue;
        }

        // Send the request based on the parsed values
        sendRequest(floor, direction, targetFloor);
        sleep(1); // Pause for 1 second before sending the next request
    }

    // Close the file after processing all lines
    file.close();
}

Client::~Client() {
    // Close the socket when the client object is destroyed
    close(sockfd);
}

#ifndef TEST_BUILD
int main() {
    // Create a Client object
    Client client;
    // Process requests from the input file "input.txt"
    client.processRequestsFromFile("input.txt");
    return 0;
}
#endif
