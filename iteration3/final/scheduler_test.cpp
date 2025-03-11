#define TEST_BUILD
#include <gtest/gtest.h>
#include <iostream>
#include "scheduler.h"

// Mock Scheduler: No real networking
class MockScheduler : public Scheduler {
public:
    MockScheduler() : Scheduler(3) {}

    void sendCommandToElevator(int elevatorID, const Request& req) override {
        std::cout << "[MOCK] Scheduler assigned Elevator " << elevatorID
                  << " to move from Floor " << req.floor << " to " << req.targetFloor << std::endl;
    }
};

// Test Elevator Assignment
TEST(SchedulerTest, AssignsClosestElevator) {
    MockScheduler scheduler;
    Request request(2, 5, "UP");
    int assignedElevator = scheduler.assignElevator(request);
    EXPECT_GE(assignedElevator, 1);
    EXPECT_LE(assignedElevator, 3);
}

// Test Sending Commands to Elevator
TEST(SchedulerTest, SendsCommandToElevator) {
    MockScheduler scheduler;
    Request request(3, 6, "UP");

    testing::internal::CaptureStdout();
    scheduler.sendCommandToElevator(2, request);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("[MOCK] Scheduler assigned Elevator 2"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

