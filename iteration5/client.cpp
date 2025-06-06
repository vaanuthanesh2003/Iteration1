#include "client.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>

#define SCHEDULER_PORT 5002
#define SCHEDULER_IP "127.0.0.1"

Client::Client() {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Client] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&schedulerAddr, 0, sizeof(schedulerAddr));
    schedulerAddr.sin_family = AF_INET;
    schedulerAddr.sin_port = htons(SCHEDULER_PORT);
    schedulerAddr.sin_addr.s_addr = inet_addr(SCHEDULER_IP);
}

int Client::getSecondsFromTimestamp(const std::string& timestamp) {
    int h, m, s;
    sscanf(timestamp.c_str(), "%d:%d:%d", &h, &m, &s);
    return h * 3600 + m * 60 + s;
}

void Client::sendRequest(int floor, std::string direction, int targetFloor) {
    std::string message = std::to_string(floor) + " " + direction + " " + std::to_string(targetFloor);
    sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
    std::cout << "[Client] Sent request: Floor " << floor << " -> Floor " << targetFloor << " (" << direction << ")" << std::endl;
}

void Client::processRequestsFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[Client] Error: Unable to open input file: " << filename << std::endl;
        return;
    }

    std::string line;
    auto startTime = std::chrono::steady_clock::now();

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string timestamp, direction;
        int floor, targetFloor;

        if (!(iss >> timestamp >> floor >> direction >> targetFloor)) {
            std::cerr << "[Client] Error: Invalid request format -> " << line << std::endl;
            continue;
        }

        int delay = getSecondsFromTimestamp(timestamp);
        std::this_thread::sleep_until(startTime + std::chrono::seconds(delay));
        sendRequest(floor, direction, targetFloor);
    }

    file.close();
}

Client::~Client() {
    close(sockfd);
}

int main() {
    Client client;
    client.processRequestsFromFile("input.txt");
    return 0;
}
