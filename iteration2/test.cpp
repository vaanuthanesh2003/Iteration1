#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <sstream>
#include "elevator_system.h"

// Reset shared state before each test
void resetGlobals() {
    std::lock_guard<std::mutex> lock(queueMutex);
    requestQueue = std::queue<Request>();
    running = true;
    elevatorState = ElevatorState::IDLE;
    schedulerState = SchedulerState::WAITING;
}

TEST(RequestTest, GettersWorkCorrectly) {
    Request req(10, 3, "UP", 5);
    EXPECT_EQ(req.get_time(), 10);
    EXPECT_EQ(req.get_floor(), 3);
    EXPECT_EQ(req.get_button(), "UP");
    EXPECT_EQ(req.get_car(), 5);
}

TEST(FloorSubsystemTest, RequestIsAddedCorrectly) {
    resetGlobals();

    FloorSubsystem floor;
    floor.requestElevator(0, 2, "UP", 5);

    std::lock_guard<std::mutex> lock(queueMutex);
    ASSERT_FALSE(requestQueue.empty());

    Request r = requestQueue.front();
    EXPECT_EQ(r.get_floor(), 2);
    EXPECT_EQ(r.get_car(), 5);
    EXPECT_EQ(r.get_button(), "UP");
}

TEST(SchedulerElevatorIntegration, ProcessesTwoRequests) {
    resetGlobals();

    // Add two requests
    AddRequest(0, 1, "UP", 3);
    AddRequest(1, 4, "DOWN", 2);

    Scheduler scheduler;
    Elevator elevator;

    std::thread schedulerThread([&] {
        SchedulerFunction(scheduler, 2, elevator); // process 2 requests
    });

    std::thread elevatorThread([&] {
        elevator.start(*(new FloorSubsystem())); // dummy
    });

    schedulerThread.join();
    elevatorThread.join();

    // After 2 requests processed, queue should be empty
    std::lock_guard<std::mutex> lock(queueMutex);
    EXPECT_TRUE(requestQueue.empty());
    EXPECT_EQ(running, false);
}

TEST(ElevatorTest, MovesAndChangesState) {
    resetGlobals();

    Elevator elevator;
    std::thread t([&] {
        elevator.moveToFloor(3);
    });

    // Wait enough time for movement
    std::this_thread::sleep_for(std::chrono::seconds(3));

    t.join();

    EXPECT_EQ(elevatorState, ElevatorState::DOORS_CLOSED);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
