#define TEST_BUILD
#include <gtest/gtest.h>
#include <iostream>
#include "elevator.h"

class MockElevator : public Elevator {
public:
    explicit MockElevator(int id) : Elevator(id) {}
    void sendStatus() override {
        std::cout << "[MOCK] Elevator " << getID() << " at floor " << getCurrentFloor() << std::endl;
    }
    void receiveCommand() override {
        std::cout << "[MOCK] Elevator " << getID() << " received command." << std::endl;
        moveTo(2);
    }
};

TEST(ElevatorTest, InitializesCorrectly) {
    MockElevator elevator(1);
    EXPECT_EQ(elevator.getCurrentFloor(), 0);
}

TEST(ElevatorTest, SendsStatus) {
    MockElevator elevator(1);
    testing::internal::CaptureStdout();
    elevator.sendStatus();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("Elevator 1 at floor"), std::string::npos);
}

TEST(ElevatorTest, ReceivesCommandAndMoves) {
    MockElevator elevator(2);
    testing::internal::CaptureStdout();
    elevator.receiveCommand();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("received command"), std::string::npos);
    EXPECT_EQ(elevator.getCurrentFloor(), 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
