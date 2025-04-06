#include "scheduler.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <climits>
#include <unistd.h>
#include <arpa/inet.h>

// Constructor: Initializes the scheduler with the number of elevators and floors
Scheduler::Scheduler(int elevCount, int floorMax) : elevatorCount(elevCount), floorCount(floorMax) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Create a UDP socket for communication
    if (sockfd < 0) {
        perror("[Scheduler] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    selfAddr = {AF_INET, htons(SCHEDULER_PORT), INADDR_ANY};  // Bind the socket to the scheduler's port
    if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("[Scheduler] Bind failed");
        exit(EXIT_FAILURE);
    }

    // Initialize elevator floors, load, and status
    for (int i = 1; i <= elevatorCount; ++i) {
        elevatorFloors[i] = 0;
        elevatorLoad[i] = 0;
        elevatorStatus[i] = "OK";
    }

    std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
}

// Destructor: Closes the socket to release resources
Scheduler::~Scheduler() {
    close(sockfd);
}

// Starts the scheduler system, launching separate threads for various tasks
void Scheduler::start() {
    startTime = std::chrono::steady_clock::now();
    // Start threads for receiving messages, processing requests, and displaying status
    std::thread(&Scheduler::receiveMessages, this).detach();
    std::thread(&Scheduler::processRequests, this).detach();
    std::thread(&Scheduler::displayStatus, this).detach();

    // Keep the scheduler running indefinitely
    while (true) std::this_thread::sleep_for(std::chrono::seconds(60));
}

// Receives messages from the elevators or other components in the system
void Scheduler::receiveMessages() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while (true) {
        // Receive incoming UDP messages
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) continue;  // Skip if there was an error receiving the message
        buffer[n] = '\0';  // Null-terminate the message

        std::stringstream ss(buffer);
        std::string type;
        ss >> type;

        std::cout << "[Scheduler] Received: " << buffer << std::endl;

        // Process different types of messages
        if (type == "STATUS") {  // Elevator status update
            int id, floor;
            ss >> id >> floor;
            elevatorFloors[id] = floor;
            elevatorLoad[id] = std::max(0, elevatorLoad[id] - 1);  // Decrease load
            auto now = std::chrono::steady_clock::now();
            // Update elevator status if necessary
            if (warningTimestamps.count(id) == 0 ||
                std::chrono::duration_cast<std::chrono::seconds>(now - warningTimestamps[id]).count() > 5) {
                elevatorStatus[id] = "REACHED";
            }
        } else if (type == "FAULT") {  // Fault update
            int id;
            ss >> id;
            elevatorStatus[id] = "FAULT";  // Mark elevator as in fault state
        } else if (type == "WARNING") {  // Warning update
            int id; std::string warning;
            ss >> id >> warning;
            elevatorStatus[id] = "WARNING(" + warning + ")";  // Update elevator status with warning
            warningTimestamps[id] = std::chrono::steady_clock::now();  // Record time of warning
        } else {
            handleClientRequest(ss, type);  // Handle a client request
        }
    }
}

// Handles client requests (requests for elevator movement)
void Scheduler::handleClientRequest(std::stringstream& ss, const std::string& firstToken) {
    int floor = std::stoi(firstToken);
    std::string direction;
    int targetFloor;
    ss >> direction >> targetFloor;

    std::lock_guard<std::mutex> lock(queueMutex);  // Lock the queue for thread safety
    std::cout << "[Scheduler] Queued request: " << floor << " -> " << targetFloor << " (" << direction << ")" << std::endl;
    requestQueue.emplace(floor, targetFloor, direction);  // Add the request to the queue
    cv.notify_one();  // Notify the thread processing requests
}

// Processes queued requests, assigning elevators to handle them
void Scheduler::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this] { return !requestQueue.empty(); });  // Wait until there is a request to process

        Request req = requestQueue.front();  // Get the next request from the queue
        requestQueue.pop();  // Remove the request from the queue
        lock.unlock();

        // Find the best elevator to handle the request
        int elevatorID = findBestElevator(req);
        if (elevatorID != -1) {
            sendMoveCommand(elevatorID, req.targetFloor);  // Send the move command to the selected elevator
            moveCount++;  // Increment the move count
            requestsHandled++;  // Increment the handled requests count
            elevatorLoad[elevatorID]++;  // Increase the load on the elevator
        } else {
            std::cout << "[Scheduler] No elevator available, requeuing request\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Wait before requeuing
            std::lock_guard<std::mutex> relock(queueMutex);  // Lock the queue again
            requestQueue.push(req);  // Re-add the request to the queue
            cv.notify_one();  // Notify the request-processing thread
        }
    }
}

// Finds the best elevator to handle the given request based on proximity and availability
int Scheduler::findBestElevator(const Request& req) {
    int best = -1, minDist = INT_MAX;
    for (int i = 1; i <= elevatorCount; ++i) {
        // Skip elevators that are not operational or are already too full
        if (elevatorStatus[i] != "OK" && elevatorStatus[i] != "REACHED") continue;
        if (elevatorLoad[i] >= MAX_CAPACITY) continue;

        // Calculate the distance between the elevator and the requested floor
        int dist = std::abs(elevatorFloors[i] - req.floor);
        if (dist < minDist) {  // Select the closest elevator
            minDist = dist;
            best = i;
        }
    }
    return best;
}

// Sends a move command to the selected elevator
void Scheduler::sendMoveCommand(int elevatorID, int targetFloor) {
    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(BASE_PORT + elevatorID);  // Elevator's port
    destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // IP address of the elevator (localhost)

    std::string cmd = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(targetFloor);
    sendto(sockfd, cmd.c_str(), cmd.length(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));  // Send the move command

    std::cout << "[Scheduler] Sent command to Elevator " << elevatorID << ": MOVE to " << targetFloor << std::endl;

    elevatorStatus[elevatorID] = "MOVING";  // Mark the elevator as moving
    elevatorFloors[elevatorID] = -1;  // Reset the elevator's floor as it is on the move
}

// Displays the status of all elevators and the request queue every 10 seconds
void Scheduler::displayStatus() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));  // Wait for 10 seconds
        std::lock_guard<std::mutex> lock(queueMutex);  // Lock the queue to prevent race conditions
        std::cout << "\n=== Elevator System Status ===\n";
        for (int i = 1; i <= elevatorCount; ++i) {
            std::cout << "Elevator " << i << ": Floor=" << elevatorFloors[i]
                      << ", Load=" << elevatorLoad[i]
                      << ", Status=" << elevatorStatus[i] << std::endl;  // Display elevator status
        }
        std::cout << "Requests in queue: " << requestQueue.size() << std::endl;  // Display the number of pending requests
        std::cout << "===============================\n";
    }
}

// Main entry point for the program (if not built for testing)
#ifndef TEST_BUILD
int main() {
    int elevators, floors;
    std::cout << "Enter number of elevators: ";
    std::cin >> elevators;
    std::cout << "Enter number of floors: ";
    std::cin >> floors;

    Scheduler scheduler(elevators, floors);  // Initialize the scheduler with user input
    scheduler.start();  // Start the scheduler system
    return 0;
}
#endif
