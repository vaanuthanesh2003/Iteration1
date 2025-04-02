#include "scheduler.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <climits>

// Constructor initializes the Scheduler, setting up the socket and listening on a port
Scheduler::Scheduler(int elevCount, int floorMax) : elevatorCount(elevCount), floorCount(floorMax) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Scheduler] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    selfAddr = {AF_INET, htons(SCHEDULER_PORT), INADDR_ANY};
    if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("[Scheduler] Bind failed");
        exit(EXIT_FAILURE);
    }

    // Initialize elevator status, floor, and load
    for (int i = 1; i <= elevatorCount; ++i) {
        elevatorFloors[i] = 0;
        elevatorLoad[i] = 0;
        elevatorStatus[i] = "OK";
    }

    std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
}

// Destructor to close the socket when the Scheduler is destroyed
Scheduler::~Scheduler() {
    close(sockfd);
}

// Start the scheduler, spawn threads for receiving messages, processing requests, and displaying status
void Scheduler::start() {
    std::thread(&Scheduler::receiveMessages, this).detach();  // Receive messages from elevators and clients
    std::thread(&Scheduler::processRequests, this).detach();   // Process floor requests and assign elevators
    std::thread(&Scheduler::displayStatusLoop, this).join();   // Display elevator statuses periodically
}

// Thread to receive messages from elevators and handle status updates
void Scheduler::receiveMessages() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while (true) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) continue;  // Ignore error and continue receiving
        buffer[n] = ' ';

        std::stringstream ss(buffer);
        std::string type;
        ss >> type;

        int id;
        if (type == "STATUS") {
            int floor;
            ss >> id >> floor;
            elevatorFloors[id] = floor;  // Update floor info for the elevator
            elevatorLoad[id] = std::max(0, elevatorLoad[id] - 1);  // Decrease load on elevator

            // Check for status updates and reset warning after a short period
            auto now = std::chrono::steady_clock::now();
            if (warningTimestamps.count(id) == 0 ||
                std::chrono::duration_cast<std::chrono::seconds>(now - warningTimestamps[id]).count() > 5) {
                elevatorStatus[id] = "OK";
            }
        } else if (type == "FAULT") {
            ss >> id;
            elevatorStatus[id] = "FAULT";  // Set status to FAULT
        } else if (type == "WARNING") {
            std::string warning;
            ss >> id >> warning;
            elevatorStatus[id] = "WARNING(" + warning + ")";  // Set status to WARNING with message
            warningTimestamps[id] = std::chrono::steady_clock::now();  // Record timestamp for warning
        } else {
            handleClientRequest(ss, type);  // Handle client requests for floor movement
        }
    }
}

// Handle client requests to move elevators
void Scheduler::handleClientRequest(std::stringstream& ss, const std::string& firstToken) {
    int floor = std::stoi(firstToken);  // Get floor from request
    std::string direction;
    int targetFloor;
    ss >> direction >> targetFloor;  // Get direction and target floor

    // Add request to the queue
    std::lock_guard<std::mutex> lock(queueMutex);
    requestQueue.emplace(floor, targetFloor, direction);
    cv.notify_one();  // Notify worker thread to process the request
}

// Main loop that processes floor requests and assigns elevators to move
void Scheduler::processRequests() {
    #ifndef TEST_BUILD
        while (!stop_requested)
    #else
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this] { return !requestQueue.empty(); });  // Wait for a request to process

        Request req = requestQueue.front();
        requestQueue.pop();  // Get the next request
        lock.unlock();

        // Find the best elevator to handle the request
        int elevatorID = findBestElevator(req);
        if (elevatorID != -1) {
            sendMoveCommand(elevatorID, req.targetFloor);  // Send MOVE command to the best elevator
            moveCount++;  // Increment move count
            elevatorLoad[elevatorID]++;  // Increase load on the elevator
        } else {
            // If no available elevator, retry after some time
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::lock_guard<std::mutex> relock(queueMutex);
            requestQueue.push(req);  // Requeue the request
            cv.notify_one();  // Notify worker thread to process again
        }
    }
    #endif
}

// Find the elevator that is best suited to handle the request (closest, available, under capacity)
int Scheduler::findBestElevator(const Request& req) {
    int best = -1, minDist = INT_MAX;
    for (int i = 1; i <= elevatorCount; ++i) {
        if (elevatorStatus[i] != "OK" || elevatorLoad[i] >= MAX_CAPACITY) continue;  // Skip faulty or full elevators
        int dist = std::abs(elevatorFloors[i] - req.floor);  // Calculate distance from requested floor
        if (dist < minDist) {
            minDist = dist;
            best = i;  // Choose elevator with the shortest distance
        }
    }
    return best;
}

// Send the MOVE command to the selected elevator
void Scheduler::sendMoveCommand(int elevatorID, int targetFloor) {
    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(BASE_PORT + elevatorID);
    destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::string cmd = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(targetFloor);
    sendto(sockfd, cmd.c_str(), cmd.length(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));  // Send command to elevator
}

// Periodically display the status of all elevators
void Scheduler::displayStatusLoop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));  // Refresh every 2 seconds
        std::lock_guard<std::mutex> lock(queueMutex);

        std::cout << "\n+-----------+--------+------+---------------------------------+" << std::endl;
        std::cout << "| Elevator  | Floor  | Load | Status                          |" << std::endl;
        std::cout << "+-----------+--------+------+---------------------------------+" << std::endl;

        // Display status for each elevator
        for (int i = 1; i <= elevatorCount; ++i) {
            std::cout << "| "
                      << std::setw(9) << std::left << i << " | "
                      << std::setw(6) << elevatorFloors[i] << " | "
                      << std::setw(4) << elevatorLoad[i] << " | "
                      << std::setw(40) << std::left << elevatorStatus[i] << " |" << std::endl;
        }

        std::cout << "+-----------+--------+------+---------------------------------+" << std::endl;
    }
}

#ifndef TEST_BUILD
int main() {
    int elevators, floors;
    std::cout << "Enter number of elevators: ";
    std::cin >> elevators;  // Get the number of elevators from
