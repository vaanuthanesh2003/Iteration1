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
#include <scheduler.h>

#define BUFFER_SIZE 1024
#define BASE_PORT 5100
#define SCHEDULER_PORT 5002
#define MAX_CAPACITY 4

struct Request {
    int floor;
    int targetFloor;
    std::string direction;
    Request(int f, int t, const std::string& d) : floor(f), targetFloor(t), direction(d) {}
};

class Scheduler {
private:
    int sockfd;
    struct sockaddr_in selfAddr;
    std::unordered_map<int, int> elevatorFloors;
    std::unordered_map<int, int> elevatorLoad;
    std::unordered_map<int, std::string> elevatorStatus;
    std::queue<Request> requestQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::chrono::steady_clock::time_point startTime;
    std::unordered_map<int, std::chrono::steady_clock::time_point> warningTimestamps;
    int moveCount = 0;
    int requestsHandled = 0;
    int elevatorCount;
    int floorCount;

public:
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

        for (int i = 1; i <= elevatorCount; ++i) {
            elevatorFloors[i] = 0;
            elevatorLoad[i] = 0;
            elevatorStatus[i] = "OK";
        }

        std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
    }

    ~Scheduler() { close(sockfd); }

    void start() {
        startTime = std::chrono::steady_clock::now();
        std::thread(&Scheduler::receiveMessages, this).detach();
        std::thread(&Scheduler::processRequests, this).detach();
        std::thread(&Scheduler::displayStatusLoop, this).join();
    }

private:
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

            if (type == "STATUS") {
                int id, floor;
                ss >> id >> floor;
                elevatorFloors[id] = floor;
                elevatorLoad[id] = std::max(0, elevatorLoad[id] - 1);

                auto now = std::chrono::steady_clock::now();
                if (warningTimestamps.count(id) == 0 ||
            std::chrono::duration_cast<std::chrono::seconds>(now - warningTimestamps[id]).count() > 5) {
                elevatorStatus[id] = "OK";

            }
            } else if (type == "FAULT") {
                int id;
                ss >> id;
                elevatorStatus[id] = "FAULT";
            } else if (type == "WARNING") {
                int id; std::string warning;
                ss >> id >> warning;
                elevatorStatus[id] = "WARNING(" + warning + ")";
                warningTimestamps[id] = std::chrono::steady_clock::now();
            } else {
                handleClientRequest(ss, type);
            }
        }
    }

    void handleClientRequest(std::stringstream& ss, const std::string& firstToken) {
        int floor = std::stoi(firstToken);
        std::string direction;
        int targetFloor;
        ss >> direction >> targetFloor;

        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.emplace(floor, targetFloor, direction);
        cv.notify_one();
    }

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

    void sendMoveCommand(int elevatorID, int targetFloor) {
        struct sockaddr_in destAddr;
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(BASE_PORT + elevatorID);
        destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        std::string cmd = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(targetFloor);
        sendto(sockfd, cmd.c_str(), cmd.length(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
    }

    void displayStatusLoop() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::cout << "\n---------------------------------------------\n";
            std::cout << "| Elevator | Floor | Load | Status          |\n";
            std::cout << "---------------------------------------------\n";
            for (int i = 1; i <= elevatorCount; ++i) {
                std::cout << "|    " << std::setw(3) << i << "   |   " << std::setw(3) << elevatorFloors[i]
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
