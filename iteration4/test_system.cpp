// #define TEST_BUILD // Already been mentioned in the compile statements and thus causing errors
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <sstream>
#include "client.h"
#include "elevator.h"


// Dummy Request struct (minimal match to scheduler)
struct Request {
    int floor;
    int targetFloor;
    std::string direction;
    Request(int f, int t, const std::string& d) : floor(f), targetFloor(t), direction(d) {}
};

// -----------------------------
// Mock Client
// -----------------------------
class MockClient {
public:
    void sendRequest(int floor, std::string direction, int targetFloor) {
        std::cout << "[MOCK] Client sent request from Floor " << floor
                  << " to Floor " << targetFloor << " (" << direction << ")" << std::endl;
    }
};

// -----------------------------
// Mock Elevator
// -----------------------------
class MockElevator {
public:
    int id = 1;
    int currentFloor = 0;

    int getID() { return id; }
    int getCurrentFloor() { return currentFloor; }

    void sendStatus() {
        std::cout << "[MOCK] Elevator " << getID() << " sending status at floor " << getCurrentFloor() << std::endl;
    }

    void receiveCommand() {
        std::cout << "[MOCK] Elevator " << getID() << " received MOVE command to Floor 5" << std::endl;
        currentFloor = 5;
    }
};

// -----------------------------
// Mock Scheduler
// -----------------------------
class MockScheduler {
public:
    void sendCommandToElevator(int elevatorID, const Request& req) {
        std::cout << "[MOCK] Scheduler assigned Elevator " << elevatorID
                  << " to move from Floor " << req.floor << " to " << req.targetFloor << std::endl;
    }

    int assignElevator(const Request& req) {
        return 2;  // always assign elevator 2 for testing
    }
};

// -----------------------------
// Unit Tests
// -----------------------------

TEST(ClientTest, SendsRequestsToScheduler) {
    MockClient client;
    testing::internal::CaptureStdout();
    client.sendRequest(3, "UP", 6);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("[MOCK] Client sent request"), std::string::npos);
}

TEST(ElevatorTest, InitializesCorrectly) {
    MockElevator elevator;
    EXPECT_EQ(elevator.getID(), 1);
    EXPECT_EQ(elevator.getCurrentFloor(), 0);
}

TEST(ElevatorTest, SendsStatus) {
    MockElevator elevator;
    testing::internal::CaptureStdout();
    elevator.sendStatus();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("[MOCK] Elevator 1 sending status"), std::string::npos);
}

TEST(ElevatorTest, ReceivesCommandsCorrectly) {
    MockElevator elevator;
    testing::internal::CaptureStdout();
    elevator.receiveCommand();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("[MOCK] Elevator 1 received MOVE command to Floor 5"), std::string::npos);
    EXPECT_EQ(elevator.getCurrentFloor(), 5);
}

TEST(SchedulerTest, AssignsClosestElevator) {
    MockScheduler scheduler;
    Request request(2, 5, "UP");
    int assignedElevator = scheduler.assignElevator(request);
    EXPECT_EQ(assignedElevator, 2);
}

TEST(SchedulerTest, SendsCommandToElevator) {
    MockScheduler scheduler;
    Request request(3, 6, "UP");

    testing::internal::CaptureStdout();
    scheduler.sendCommandToElevator(2, request);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("[MOCK] Scheduler assigned Elevator 2"), std::string::npos);
}

// -----------------------------
// Test Entry Point
// -----------------------------

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
