#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <climits>

#define CLIENT_PORT 5001 // Receives requests from client
#define SCHEDULER_PORT 5002 // Scheduler listens on this port
#define BASE_ELEVATOR_PORT 5100 // Elevators start binding from 5101, 5102, etc.
#define BUFFER_SIZE 1024 // Size of the buffer used for communication

// Struct representing an elevator request
struct Request {
    int floor;  // Current floor where the request is made
    int targetFloor;  // Target floor to move to
    std::string direction;  // Direction of movement (Up or Down)

    // Constructor to initialize the request
    Request(int f, int tf, std::string d) : floor(f), targetFloor(tf), direction(d) {}
};

// Scheduler class that manages elevator requests and coordinates elevators
class Scheduler {
private:
    int sockfd;  // Socket file descriptor for communication
    struct sockaddr_in clientAddr, selfAddr, elevatorAddr;  // Address structures for client, scheduler, and elevators
    std::unordered_map<int, int> elevatorPositions; // ElevatorID -> Floor (tracks current floor of each elevator)
    std::queue<Request> requestQueue;  // Queue to store incoming requests

public:
    // Constructor to initialize the scheduler with a given number of elevators
    Scheduler(int numElevators);

    // Function to receive requests from clients
    void receiveRequests();

    // Function to process requests and assign elevators
    void processRequests();

    // Function to assign the best elevator for a given request
    int assignElevator(const Request& req);

    // Function to send the move command to an assigned elevator
    void sendCommandToElevator(int elevatorID, const Request& req);

    // Function to receive elevator position updates
    void receiveElevatorUpdates();

    // Main function to run the scheduler, including handling threads
    void run();
};

// Initializes the scheduler
Scheduler::Scheduler(int numElevators) {
    for (int i = 1; i <= numElevators; i++) {
        elevatorPositions[i] = 0; // Start all elevators at floor 0
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Create a UDP socket
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setup the scheduler's listening address
    memset(&selfAddr, 0, sizeof(selfAddr));  // Clear the address structure
    selfAddr.sin_family = AF_INET;  // Set address family to IPv4
    selfAddr.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any address
    selfAddr.sin_port = htons(SCHEDULER_PORT);  // Set scheduler port

    // Bind the socket to the scheduler's address
    if (bind(sockfd, (const struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Setup the client's address for communication
    memset(&clientAddr, 0, sizeof(clientAddr));  // Clear the address structure
    clientAddr.sin_family = AF_INET;  // Set address family to IPv4
    clientAddr.sin_port = htons(CLIENT_PORT);  // Set client port
    clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Set client IP to localhost

    std::cout << "[Scheduler] Initialized and listening on port " << SCHEDULER_PORT << std::endl;
}

// Receives requests from `client.cpp` and stores them in the request queue
void Scheduler::receiveRequests() {
    char buffer[BUFFER_SIZE];  // Buffer to store incoming data
    struct sockaddr_in senderAddr;  // Address of the sender
    socklen_t len = sizeof(senderAddr);  // Length of sender's address

    while (true) {
        // Receive incoming request
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) continue;  // Skip if receiving fails

        buffer[n] = '\0';  // Null-terminate the received message

        int floor, targetFloor;
        char direction[10];
        // Parse the request and store it in the requestQueue
        if (sscanf(buffer, "%d %s %d", &floor, direction, &targetFloor) == 3) {
            requestQueue.push(Request(floor, targetFloor, std::string(direction)));
            std::cout << "[Scheduler] New request: Floor " << floor << " -> Floor " << targetFloor
                      << " (" << direction << ")" << std::endl;
        }
    }
}

// Processes requests and assigns elevators to handle them
void Scheduler::processRequests() {
    while (true) {
        if (requestQueue.empty()) continue;  // Skip if there are no requests

        // Get the next request from the queue
        Request req = requestQueue.front();
        requestQueue.pop();

        // Assign the best elevator to the request
        int assignedElevator = assignElevator(req);
        if (assignedElevator != -1) {
            sendCommandToElevator(assignedElevator, req);  // Send move command to assigned elevator
        } else {
            requestQueue.push(req);  // Retry later if no elevator is available
            sleep(1);  // Wait before retrying
        }
    }
}

// Assigns the best available elevator based on proximity to the requested floor
int Scheduler::assignElevator(const Request& req) {
    int bestElevator = -1;
    int minDistance = INT_MAX;  // Initialize minimum distance to a large value
    bool foundIdleElevator = false;

    std::cout << "[DEBUG] [Scheduler] Evaluating elevators for request: Floor " << req.floor
              << " to " << req.targetFloor << std::endl << std::flush;

    // Evaluate all elevators for the best match
    for (auto& [id, floor] : elevatorPositions) {
        int distance = abs(floor - req.floor);  // Calculate the distance to the requested floor
        
        std::cout << "[DEBUG] [Scheduler] Checking Elevator " << id << " (Currently at Floor " << floor
                  << ", Distance " << distance << ")" << std::endl << std::flush;

        // Prefer an idle elevator close to the requested floor
        if (distance < minDistance) {
            bestElevator = id;
            minDistance = distance;
        }
    }

    std::cout << "[DEBUG] [Scheduler] Assigned Elevator " << bestElevator << " to handle request." << std::endl << std::flush;
    return bestElevator;
}

// Sends movement command to the assigned elevator
void Scheduler::sendCommandToElevator(int elevatorID, const Request& req) {
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "MOVE %d %d", elevatorID, req.targetFloor);  // Format move command

    memset(&elevatorAddr, 0, sizeof(elevatorAddr));  // Clear the address structure for the elevator
    elevatorAddr.sin_family = AF_INET;  // Set address family to IPv4
    elevatorAddr.sin_port = htons(BASE_ELEVATOR_PORT + elevatorID);  // Set elevator's port
    elevatorAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Set elevator's IP to localhost

    sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&elevatorAddr, sizeof(elevatorAddr));  // Send command

    std::cout << "[Scheduler] Assigned Elevator " << elevatorID << ": " << message << std::endl << std::flush;
    std::cout << "[DEBUG] [Scheduler] sent command to elevator: " << message << "ElevatorID" << elevatorID << " on base port: " << BASE_ELEVATOR_PORT + elevatorID << std::endl << std::flush;
}

// Receives position updates from elevators and updates the scheduler's record
void Scheduler::receiveElevatorUpdates() {
    char buffer[BUFFER_SIZE];  // Buffer to store incoming data
    struct sockaddr_in senderAddr;  // Address of the sender
    socklen_t len = sizeof(senderAddr);  // Length of sender's address

    while (true) {
        // Receive position update from an elevator
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) continue;  // Skip if receiving fails

        buffer[n] = '\0';  // Null-terminate the received message

        int elevatorID, floor;
        // Parse the position update and update the elevator's floor
        if (sscanf(buffer, "STATUS %d %d", &elevatorID, &floor) == 2) {
            elevatorPositions[elevatorID] = floor;  // Update the elevator floor position
            std::cout << "[DEBUG] [Scheduler] Elevator " << elevatorID
                      << " updated to Floor " << floor << std::endl << std::flush;
        }
    }
}

// Runs the scheduler with threads for handling requests
void Scheduler::run() {
    // Start threads for receiving requests, processing requests, and receiving elevator updates
    std::thread requestThread(&Scheduler::receiveRequests, this);
    std::thread processThread(&Scheduler::processRequests, this);
    std::thread updateThread(&Scheduler::receiveElevatorUpdates, this);

    // Wait for all threads to finish
    requestThread.join();
    processThread.join();
    updateThread.join();
}

int main() {
    Scheduler scheduler(3);  // Initialize the scheduler with 3 elevators
    scheduler.run();  // Start the scheduler
    return 0;
}
