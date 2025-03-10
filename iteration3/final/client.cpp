#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "client.h"

#define SCHEDULER_IP "127.0.0.1"
#define SCHEDULER_PORT 5002 // Matches the scheduler's port
#define BUFFER_SIZE 1024
#define INPUT_FILE "input.txt"

Client::Client(){
    std::cout << "[Client] Initialized client ... " << std::endl;
}

void Client::sendRequest(int floor, std::string direction, int targetFloor) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in schedulerAddr{};
    memset(&schedulerAddr, 0, sizeof(schedulerAddr));
    schedulerAddr.sin_family = AF_INET;
    schedulerAddr.sin_port = htons(SCHEDULER_PORT);
   
    if (inet_pton(AF_INET, SCHEDULER_IP, &schedulerAddr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::string message = std::to_string(floor) + " " + direction + " " + std::to_string(targetFloor);
    sendto(sockfd, message.c_str(), message.size(), 0,
           (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));

    std::cout << "[Client] Sent request: " << message << std::endl;
    close(sockfd);
}

void processRequestsFromFile() {
    std::ifstream inputFile(INPUT_FILE);
    if (!inputFile) {
        std::cerr << "[Client] Error: Could not open input file: " << INPUT_FILE << std::endl;
        return;
    }
    Client client;
    std::string line;
    while (std::getline(inputFile, line)) {
        std::istringstream iss(line);
        int time, floor, targetFloor;
        std::string direction;
       
        if (!(iss >> time >> floor >> direction >> targetFloor)) {
            std::cerr << "[Client] Error: Invalid line format -> " << line << std::endl;
            continue;
        }

        client.sendRequest(floor, direction, targetFloor);
        sleep(2); // Small delay to simulate time between requests
    }

    inputFile.close();
}

#ifndef TEST_BUILD
int main() {
    while (true) {
        processRequestsFromFile();
        sleep(5); // Wait before reading file again (optional)
    }
    return 0;

}

#endif
