#include <gtest/gtest.h>
#include "elevator.h"

// Subclass to expose currentFloor for setup purposes
class TestableElevator : public Elevator {
public:
    using Elevator::currentFloor;

    TestableElevator(int id) : Elevator(id) {}
};

// =====================
// Unit Tests
// =====================

TEST(ElevatorTest, NoMovementIfAlreadyAtTarget) {
    Elevator elevator(1);
    EXPECT_EQ(elevator.getCurrentFloor(), 0);  // should start at 0

    elevator.moveTo(0);  // No movement expected
    EXPECT_EQ(elevator.getCurrentFloor(), 0);
}

TEST(ElevatorTest, MovesUpToTargetFloor) {
    Elevator elevator(2);
    elevator.moveTo(3);
    EXPECT_EQ(elevator.getCurrentFloor(), 3);
}

TEST(ElevatorTest, MovesDownToTargetFloor) {
    TestableElevator elevator(3);
    elevator.currentFloor = 5;
    elevator.moveTo(2);
    EXPECT_EQ(elevator.getCurrentFloor(), 2);
}

TEST(ElevatorTest, SendsStatusAfterMove) {
    Elevator elevator(4);
    elevator.moveTo(1);
    EXPECT_EQ(elevator.getCurrentFloor(), 1);
    elevator.sendStatus();  // Should output to console, we can't assert this, but it should not crash
    SUCCEED();  // Test passes as long as sendStatus() doesn't crash
}

TEST(ElevatorTest, SendsFaultMessageManually) {
    Elevator elevator(5);
    elevator.sendFaultMessage("FAULT 5");  // Should output to console
    SUCCEED();  // Just testing that it doesn't crash
}

// Optional: if you want to simulate timeout and ensure it logs something, comment out for now
// TEST(ElevatorTest, MovementTimeoutSimulation) {
//     Elevator elevator(6);
//     elevator.moveTo(100);  // May take a while depending on MOVE_TIMEOUT logic
//     // This would normally exit the program, so can't be tested without mocks
// }

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
