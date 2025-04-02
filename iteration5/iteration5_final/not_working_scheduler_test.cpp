// === scheduler_test.cpp ===
#define TEST_BUILD
#include "scheduler.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <sstream>
#include <iostream>
#include <streambuf>

class MockScheduler : public Scheduler {
public:
    MockScheduler() : Scheduler(3, 10) {
        elevatorFloors[1] = 2;
        elevatorFloors[2] = 5;
        elevatorFloors[3] = 8;

        elevatorStatus[1] = "OK";
        elevatorStatus[2] = "OK";
        elevatorStatus[3] = "OK";
    }

    void sendMoveCommand(int elevatorID, int targetFloor) override {
        std::cout << "[MOCK] Scheduler would send MOVE to Elevator "
                  << elevatorID << " to Floor " << targetFloor << std::endl;
    }
};

TEST(SchedulerTest, AssignsClosestElevator) {
    MockScheduler scheduler;

    std::thread schedulerThread(&MockScheduler::processRequests, &scheduler);

    // Capture stdout
    testing::internal::CaptureStdout();

    // Inject test request (floor 6 should go to elevator 2 at floor 5)
    scheduler.testInjectRequest(Request(6, 10, "UP"));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::string output = testing::internal::GetCapturedStdout();
    std::cout << "[TEST OUTPUT]\n" << output << std::endl;

    EXPECT_NE(output.find("MOCK"), std::string::npos);
    EXPECT_NE(output.find("Elevator 2"), std::string::npos);
    EXPECT_NE(output.find("to Floor 10"), std::string::npos);

    scheduler.stop();
    schedulerThread.join();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
