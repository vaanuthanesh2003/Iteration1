#include "scheduler.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <climits>

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

    for (int i = 1; i <= elevatorCount; ++i) {
        elevatorFloors[i] = 0;
        elevatorLoad[i] = 0;
        elevatorStatus[i] = "OK";
    }

    std::cout << "[Scheduler] Listening on port " << SCHEDULER_PORT << std::endl;
}

Scheduler::~Scheduler() {
    close(sockfd);
}

void Scheduler::start() {
    std::thread(&Scheduler::receiveMessages, this).detach();
    std::thread(&Scheduler::processRequests, this).detach();
    std::thread(&Scheduler::displayStatusLoop, this).join();
}

void Scheduler::receiveMessages() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while (true) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&senderAddr, &len);
        if (n < 0) continue;
        buffer[n] = ' ';

        std::stringstream ss(buffer);
        std::string type;
        ss >> type;

        int id;
        if (type == "STATUS") {
            int floor;
            ss >> id >> floor;
            elevatorFloors[id] = floor;
            elevatorLoad[id] = std::max(0, elevatorLoad[id] - 1);

            auto now = std::chrono::steady_clock::now();
            if (warningTimestamps.count(id) == 0 ||
                std::chrono::duration_cast<std::chrono::seconds>(now - warningTimestamps[id]).count() > 5) {
                elevatorStatus[id] = "OK";
            }
        } else if (type == "FAULT") {
            ss >> id;
            elevatorStatus[id] = "FAULT";
        } else if (type == "WARNING") {
            std::string warning;
            ss >> id >> warning;
            elevatorStatus[id] = "WARNING(" + warning + ")";
            warningTimestamps[id] = std::chrono::steady_clock::now();
        } else {
            handleClientRequest(ss, type);
        }
    }
}

void Scheduler::handleClientRequest(std::stringstream& ss, const std::string& firstToken) {
    int floor = std::stoi(firstToken);
    std::string direction;
    int targetFloor;
    ss >> direction >> targetFloor;

    std::lock_guard<std::mutex> lock(queueMutex);
    requestQueue.emplace(floor, targetFloor, direction);
    cv.notify_one();
}

void Scheduler::processRequests() {

    #ifndef TEST_BUILD
        while (!stop_requested)
    #else
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
    #endif
}

int Scheduler::findBestElevator(const Request& req) {
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

void Scheduler::sendMoveCommand(int elevatorID, int targetFloor) {
    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(BASE_PORT + elevatorID);
    destAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::string cmd = "MOVE " + std::to_string(elevatorID) + " " + std::to_string(targetFloor);
    sendto(sockfd, cmd.c_str(), cmd.length(), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
}

void Scheduler::displayStatusLoop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::lock_guard<std::mutex> lock(queueMutex);

        std::cout << "\n+-----------+--------+------+---------------------------------+" << std::endl;
        std::cout << "| Elevator  | Floor  | Load | Status                          |" << std::endl;
        std::cout << "+-----------+--------+------+---------------------------------+" << std::endl;

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
int main(){
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
