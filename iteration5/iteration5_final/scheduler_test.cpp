#define TEST_BUILD
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <iostream>
#include <climits>

// Constants matching scheduler.cpp
#define MAX_CAPACITY 4

// Re-declare Request struct
struct Request {
    int floor;
    int targetFloor;
    std::string direction;
    Request(int f, int t, const std::string& d) : floor(f), targetFloor(t), direction(d) {}
};

// === Minimal Scheduler declaration with virtual method ===
class Scheduler {
public:
    Scheduler(int, int) {}
    virtual ~Scheduler() {}
    virtual void sendMoveCommand(int, int) {}
};

// === MockScheduler for testing ===
class MockScheduler : public Scheduler {
public:
    std::string capturedCommand;
    std::unordered_map<int, int> elevatorFloors;
    std::unordered_map<int, std::string> elevatorStatus;
    std::queue<Request> requestQueue;
    std::mutex queueMutex;
    std::condition_variable cv;

    MockScheduler() : Scheduler(3, 10) {}

    void injectRequest(const Request& req) {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(req);
        cv.notify_one();
    }

    void runOnce() {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (requestQueue.empty()) return;

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        int best = -1, minDist = INT_MAX;
        for (int i = 1; i <= 3; ++i) {
            if (elevatorStatus[i] != "OK") continue;
            int dist = std::abs(elevatorFloors[i] - req.floor);
            if (dist < minDist) {
                minDist = dist;
                best = i;
            }
        }

        if (best != -1) {
            sendMoveCommand(best, req.targetFloor);
        }
    }

    void sendMoveCommand(int elevatorID, int targetFloor) override {
        std::stringstream ss;
        ss << "MOVE to Elevator " << elevatorID << " to Floor " << targetFloor;
        capturedCommand = ss.str();
        std::cout << "[MOCK] " << capturedCommand << std::endl;
    }
};

// === Unit Test ===
TEST(SchedulerTest, AssignsClosestElevator) {
    MockScheduler scheduler;
    scheduler.elevatorFloors[1] = 0;
    scheduler.elevatorFloors[2] = 4;
    scheduler.elevatorFloors[3] = 2;

    scheduler.elevatorStatus[1] = "OK";
    scheduler.elevatorStatus[2] = "OK";
    scheduler.elevatorStatus[3] = "OK";

    scheduler.injectRequest(Request(3, 7, "UP"));
    scheduler.runOnce();

    EXPECT_NE(scheduler.capturedCommand.find("MOVE to Elevator 2 to Floor 7"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
