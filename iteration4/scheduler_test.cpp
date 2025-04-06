#include <gtest/gtest.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "scheduler.h"

// Mock Request
Request makeRequest(int floor, int targetFloor, const std::string& dir) {
    return Request(floor, targetFloor, dir);
}

TEST(SchedulerTest, FindBestElevator_Basic) {
    Scheduler scheduler(3, 10);

    // Set up elevators
    scheduler.elevatorFloors[1] = 0;
    scheduler.elevatorFloors[2] = 3;
    scheduler.elevatorFloors[3] = 5;

    scheduler.elevatorStatus[1] = "OK";
    scheduler.elevatorStatus[2] = "OK";
    scheduler.elevatorStatus[3] = "OK";

    scheduler.elevatorLoad[1] = 0;
    scheduler.elevatorLoad[2] = 0;
    scheduler.elevatorLoad[3] = 0;

    Request r = makeRequest(2, 7, "up");
    int best = scheduler.findBestElevator(r);

    EXPECT_EQ(best, 2);  // Closest elevator to floor 2
}

TEST(SchedulerTest, FindBestElevator_CapacityLimit) {
    Scheduler scheduler(2, 10);
    scheduler.elevatorFloors[1] = 1;
    scheduler.elevatorFloors[2] = 2;

    scheduler.elevatorLoad[1] = MAX_CAPACITY;  // Full
    scheduler.elevatorLoad[2] = 2;

    scheduler.elevatorStatus[1] = "OK";
    scheduler.elevatorStatus[2] = "OK";

    Request r = makeRequest(2, 5, "up");
    int best = scheduler.findBestElevator(r);

    EXPECT_EQ(best, 2);  // Only elevator 2 is under capacity
}

TEST(SchedulerTest, FindBestElevator_AllFaulty) {
    Scheduler scheduler(2, 10);
    scheduler.elevatorFloors[1] = 1;
    scheduler.elevatorFloors[2] = 5;

    scheduler.elevatorLoad[1] = 0;
    scheduler.elevatorLoad[2] = 0;

    scheduler.elevatorStatus[1] = "FAULT";
    scheduler.elevatorStatus[2] = "FAULT";

    Request r = makeRequest(3, 6, "up");
    int best = scheduler.findBestElevator(r);

    EXPECT_EQ(best, -1);  // No elevator is available
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
