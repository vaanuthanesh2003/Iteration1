#define TEST_BUILD
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include "elevator.h"

// Mock Elevator: Overrides networking
class MockElevator : public Elevator {
public:
    explicit MockElevator(int elevator_id) : Elevator(elevator_id) {}

    void sendStatus() override {
        std::cout << "[MOCK] Elevator " << getID() << " sending status at floor " << getCurrentFloor() << std::endl;
    }

    void receiveCommand() override {
        std::cout << "[MOCK] Elevator " << getID() << " received MOVE command to Floor 5" << std::endl;
        moveTo(5);  // Simulate movement
    }
};

// Test Elevator Initialization
TEST(ElevatorTest, InitializesCorrectly) {
    MockElevator elevator(1);
    EXPECT_EQ(elevator.getID(), 1);
    EXPECT_EQ(elevator.getCurrentFloor(), 0);
}

// Test Sending Status
TEST(ElevatorTest, SendsStatus) {
    MockElevator elevator(1);
    testing::internal::CaptureStdout();
    elevator.sendStatus();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("[MOCK] Elevator 1 sending status"), std::string::npos);
}

// Test Receiving Commands
TEST(ElevatorTest, ReceivesCommandsCorrectly) {
    MockElevator elevator(2);
    testing::internal::CaptureStdout();
    elevator.receiveCommand();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("[MOCK] Elevator 2 received MOVE command to Floor 5"), std::string::npos);
    EXPECT_EQ(elevator.getCurrentFloor(), 5);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

