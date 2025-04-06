// scheduler.cpp - Iteration 5 Final with MOVING/REACHED UI and All Fixes
#include <iostream>
#include <unordered_map>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <climits>
#include <chrono>
#include <iomanip>
#include <vector>

#define BUFFER_SIZE 1024
#define BASE_PORT 5100
#define SCHEDULER_PORT 5002
#define MAX_CAPACITY 4
// Structure to represent a client request
struct Request {
    int floor;
    int targetFloor;
    std::string direction;
    Request(int f, int t, const std::string& d) : floor(f), targetFloor(t), direction(d) {}
};

// Main class that handles scheduling logic
class Scheduler {
private:
    int sockfd;
    struct sockaddr_in selfAddr;
    // Elevator tracking
    std::unordered_map<int, int> elevatorFloors;   // Current floor of each elevator
    std::unordered_map<int, int> elevatorLoad;     // Current load of each elevator
    std::unordered_map<int, std::string> elevatorStatus; // Status (OK, MOVING, REACHED, etc.)

    std::queue<Request> requestQueue;  // Queue of pending client requests
    std::mutex queueMutex;             // Mutex for request queue
    std::condition_variable cv;        // Condition variable for queue processing

    std::chrono::steady_clock::time_point startTime; // Simulation start time
    std::unordered_map<int, std::chrono::steady_clock::time_point> warningTimestamps; // Last warning time for elevators

    int moveCount = 0; // Number of move commands sent
    int requestsHandled = 0; //Numbver of requests handled
    int elevatorCount; // Number of elevators
    int floorCount; // Number of floors

public:
    Scheduler(int elevCount, int floorMax) : elevatorCount(elevCount), floorCount(floorMax) {
        //Create UDP socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("[Scheduler] Socket creation failed");
            exit(EXIT_FAILURE);
        }
        //Bind socket to scheduler port
        selfAddr = {AF_INET, htons(SCHEDULER_PORT), INADDR_ANY};
        if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
            perror("[Scheduler] Bind failed");
            exit(EXIT_FAILURE);
        }
        // Initialize elevators to floor 0, load 0, and status OK
        for (int i = 1; i <= elevatorCount; ++i) {
            elevatorFloors[i] = 0;
            elevatorLoad[i] = 0;
            elevatorStatus[i] = "OK";
        }

        std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
    }

    ~Scheduler() { close(sockfd); }

    void start();

private:
    void receiveMessages();                    // Receives messages from elevators and clients
    void handleClientRequest(std::stringstream& ss, const std::string& firstToken); // Parses and enqueues client requests
    void processRequests();                    // Assigns requests to elevators
    int findBestElevator(const Request& req);  // Selects best elevator for a request
    void sendMoveCommand(int elevatorID, int targetFloor); // Sends move command to elevator
    void displayStatusLoop();                 // Periodically displays status of elevators
};

// Main control function: starts threads
void Scheduler::start() {
    startTime = std::chrono::steady_clock::now();
    std::thread(&Scheduler::receiveMessages, this).detach();
    std::thread(&Scheduler::processRequests, this).detach();
    std::thread(&Scheduler::displayStatusLoop, this).join();
}

// Receives UDP messages from elevators and clients
void Scheduler::receiveMessages() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while (true) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) continue;
        buffer[n] = '\0';

        std::stringstream ss(buffer);
        std::string type;
        ss >> type;
  // Handle status update from elevator
        if (type == "STATUS") {
            int id, floor;
            ss >> id >> floor;
            elevatorFloors[id] = floor;
            elevatorLoad[id] = std::max(0, elevatorLoad[id] - 1);

            // Mark elevator as REACHED if not warned in last 5 seconds
            
            auto now = std::chrono::steady_clock::now();
            if (warningTimestamps.count(id) == 0 ||
                std::chrono::duration_cast<std::chrono::seconds>(now - warningTimestamps[id]).count() > 5) {
                elevatorStatus[id] = "REACHED";
            }
        } else if (type == "FAULT") {
             // Handle elevator fault
            int id;
            ss >> id;
            elevatorStatus[id] = "FAULT";
        } else if (type == "WARNING") {
            // Handle elevator warning
            int id; std::string warning;
            ss >> id >> warning;
            elevatorStatus[id] = "WARNING(" + warning + ")";
            warningTimestamps[id] = std::chrono::steady_clock::now();
        } else {
            handleClientRequest(ss, type);
        }
    }
}
// Parse and enqueue a new client request
void Scheduler::handleClientRequest(std::stringstream& ss, const std::string& firstToken) {
    int floor = std::stoi(firstToken);
    std::string direction;
    int targetFloor;
    ss >> direction >> targetFloor;

    std::lock_guard<std::mutex> lock(queueMutex);
    requestQueue.emplace(floor, targetFloor, direction);
    cv.notify_one();
}
// Continuously processes queued requests and assigns them to elevators
void Scheduler::processRequests() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this] { return !requestQueue.empty(); });

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        int elevatorID = findBestElevator(req);
        if (elevatorID != -1) {
            sendMoveCommand(elevatorID, req.targetFloor);
            moveCount++;
            requestsHandled++;
            elevatorLoad[elevatorID]++;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::lock_guard<std::mutex> relock(queueMutex);
            requestQueue.push(req);
            cv.notify_one();
        }
    }
}
// Finds the best elevator for a request based on availability and proximity
int Scheduler::findBestElevator(const Request& req) {
    int best = -1, minDist = INT_MAX;
    for (int i = 1; i <= elevatorCount; ++i) {
        if (elevatorStatus[i] != "OK" && elevatorStatus[i] != "REACHED") continue;
        if (elevatorLoad[i] >= MAX_CAPACITY) continue;
        int dist = std::abs(elevatorFloors[i] - req.floor);
        if (dist < minDist) {
            minDist = dist;
            best = i;
        }
    }
    return best;
}
// Sends a move command to a specific elevator
void Scheduler::sendMoveCommand(int elevatorID, int targetFloor) {
    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(BASE_PORT + elevatorID);
    destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::string cmd = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(targetFloor);
    sendto(sockfd, cmd.c_str(), cmd.length(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));

    elevatorStatus[elevatorID] = "MOVING";
    elevatorFloors[elevatorID] = -1;
}
// Periodically prints elevator statuses and stats
void Scheduler::displayStatusLoop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "\n---------------------------------------------\n";
        std::cout << "| Elevator | Floor | Load | Status          |\n";
        std::cout << "---------------------------------------------\n";
        for (int i = 1; i <= elevatorCount; ++i) {
            std::string floorDisplay = (elevatorFloors[i] == -1 ? "-" : std::to_string(elevatorFloors[i]));
            std::cout << "|    " << std::setw(3) << i << "   |   " << std::setw(3) << floorDisplay
                      << "  |  " << std::setw(2) << elevatorLoad[i] << "  | "
                      << std::setw(35) << elevatorStatus[i] << " |\n";
        }
        static int cycleCount = 0;
        cycleCount++;
        if (cycleCount % 5 == 0) {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

            std::cout << "\n=== Simulation Stats ===\n";
            std::cout << "Simulation Time: " << duration.count() << " seconds\n";
            std::cout << "Total Moves: " << moveCount << "\n";
            std::cout << "Requests Handled: " << requestsHandled << "\n";
            std::cout << "---------------------------------------------\n";
        }
    }
}
// Entry point: initializes scheduler with user-defined elevator and floor count
#ifndef TEST_BUILD
int main() {
    int elevators, floors;
    std::cout << "Enter number of elevators: ";
    std::cin >> elevators;
    std::cout << "Enter number of floors: ";
    std::cin >> floors;

    Scheduler scheduler(elevators, floors);
    scheduler.start();
    return 0;
}
#endif

