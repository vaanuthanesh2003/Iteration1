#include <gtest/gtest.h>
#include "../scheduler.cpp"  // Ensure correct path if needed
#include <thread>
#include <chrono>

class SchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        scheduler = new Scheduler(2, 10); // 2 elevators, 10 floors
    }

    void TearDown() override {
        delete scheduler;
    }

    Scheduler* scheduler;
};

// Test: Scheduler Initialization
TEST_F(SchedulerTest, Initialization) {
    EXPECT_EQ(scheduler->getElevatorCount(), 2);
    EXPECT_EQ(scheduler->getFloorCount(), 10);
}

// Test: Adding Requests
TEST_F(SchedulerTest, HandleClientRequest) {
    std::stringstream ss("3 UP 7");
    scheduler->handleClientRequest(ss, "3");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_FALSE(scheduler->isRequestQueueEmpty());
}

// Test: Assigning Elevators
TEST_F(SchedulerTest, FindBestElevator) {
    Request req(3, 7, "UP");
    int bestElevator = scheduler->findBestElevator(req);

    EXPECT_NE(bestElevator, -1);
}

// Test: Processing Requests
TEST_F(SchedulerTest, ProcessRequests) {
    std::stringstream ss("2 DOWN 0");
    scheduler->handleClientRequest(ss, "2");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_EQ(scheduler->getTotalRequestsHandled(), 1);
}

// Test: Status Update
TEST_F(SchedulerTest, StatusUpdate) {
    std::stringstream ss("STATUS 1 5");
    scheduler->receiveMessages(ss);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(scheduler->getElevatorFloor(1), 5);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
