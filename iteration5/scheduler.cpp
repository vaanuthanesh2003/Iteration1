// scheduler.cpp - Iteration 5 Final with Console UI and Configuration

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

#define BUFFER_SIZE 1024   // Maximum size for incoming messages
#define BASE_PORT 5100     // Base port for elevator communication
#define SCHEDULER_PORT 5002 // Port where scheduler listens for requests
#define MAX_CAPACITY 4     // Maximum load an elevator can carry

// Structure to represent a ride request
struct Request {
    int floor;        // Floor where request originated
    int targetFloor;  // Destination floor
    std::string direction; // Direction of request ("UP" or "DOWN")
    Request(int f, int t, const std::string& d) : floor(f), targetFloor(t), direction(d) {}
};

class Scheduler {
private:
    int sockfd; // Socket descriptor for UDP communication
    struct sockaddr_in selfAddr; // Address structure for scheduler

    // Maps to track elevator state
    std::unordered_map<int, int> elevatorFloors; // Current floor of each elevator
    std::unordered_map<int, int> elevatorLoad;   // Current load (number of passengers)
    std::unordered_map<int, std::string> elevatorStatus; // Status (OK, FAULT, WARNING)
    
    std::queue<Request> requestQueue; // Queue of pending ride requests
    std::mutex queueMutex; // Mutex for thread safety
    std::condition_variable cv; // Condition variable to signal request processing
    
    std::unordered_map<int, std::chrono::steady_clock::time_point> warningTimestamps; // Tracks when warnings were issued
    
    int moveCount = 0; // Tracks the number of elevator moves made
    int elevatorCount; // Total number of elevators
    int floorCount; // Total number of floors

public:
    // Constructor initializes elevators and binds scheduler to a port
    Scheduler(int elevCount, int floorMax) : elevatorCount(elevCount), floorCount(floorMax) {
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

        // Initialize elevator status and positions
        for (int i = 1; i <= elevatorCount; ++i) {
            elevatorFloors[i] = 0; // All elevators start at floor 0
            elevatorLoad[i] = 0; // No passengers at start
            elevatorStatus[i] = "OK"; // All elevators are operational
        }

        std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
    }

    ~Scheduler() { close(sockfd); } // Destructor closes socket

    // Starts the scheduler by launching message handling and request processing threads
    void start() {
        std::thread(&Scheduler::receiveMessages, this).detach();
        std::thread(&Scheduler::processRequests, this).detach();
        std::thread(&Scheduler::displayStatusLoop, this).join();
    }

private:
    // Handles incoming messages from elevators and clients
    void receiveMessages() {
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

            if (type == "STATUS") { // Elevator status update
                int id, floor;
                ss >> id >> floor;
                elevatorFloors[id] = floor;
                elevatorLoad[id] = std::max(0, elevatorLoad[id] - 1);

                auto now = std::chrono::steady_clock::now();
                if (warningTimestamps.count(id) == 0 || 
                    std::chrono::duration_cast<std::chrono::seconds>(now - warningTimestamps[id]).count() > 5) {
                    elevatorStatus[id] = "OK";
                }
            } else if (type == "FAULT") { // Elevator failure reported
                int id;
                ss >> id;
                elevatorStatus[id] = "FAULT";
            } else if (type == "WARNING") { // Elevator warning message
                int id; std::string warning;
                ss >> id >> warning;
                elevatorStatus[id] = "WARNING(" + warning + ")";
                warningTimestamps[id] = std::chrono::steady_clock::now();
            } else { // Request from a client (ride request)
                handleClientRequest(ss, type);
            }
        }
    }

    // Handles ride requests from clients
    void handleClientRequest(std::stringstream& ss, const std::string& firstToken) {
        int floor = std::stoi(firstToken);
        std::string direction;
        int targetFloor;
        ss >> direction >> targetFloor;

        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.emplace(floor, targetFloor, direction);
        cv.notify_one();
    }

    // Processes ride requests by assigning elevators
    void processRequests() {
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
                elevatorLoad[elevatorID]++;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                std::lock_guard<std::mutex> relock(queueMutex);
                requestQueue.push(req);
                cv.notify_one();
            }
        }
    }

    // Finds the most suitable elevator for a request
    int findBestElevator(const Request& req) {
        int best = -1, minDist = INT_MAX;
        for (int i = 1; i <= elevatorCount; ++i) {
            if (elevatorStatus[i] != "OK" || elevatorLoad[i] >= MAX_CAPACITY) continue;
            int dist = std::abs(elevatorFloors[i] - req.floor);
            if (dist < minDist) {
                minDist = dist;
                best = i;
            }
        }
        return best;
    }

    // Sends movement command to the selected elevator
    void sendMoveCommand(int elevatorID, int targetFloor) {
        struct sockaddr_in destAddr;
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(BASE_PORT + elevatorID);
        destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        std::string cmd = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(targetFloor);
        sendto(sockfd, cmd.c_str(), cmd.length(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
    }
};

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
