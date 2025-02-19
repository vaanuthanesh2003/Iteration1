#include <gtest/gtest.h>
#include <sstream>
#include "elevator_system.h"

extern std::queue<Request> requestQueue;
extern std::mutex queueMutex;
extern std::condition_variable cond;
extern ElevatorState elevatorState;
extern SchedulerState schedulerState;
extern bool running;

TEST(RequestTest, GettersAndSetters) {
    Request req(5, 2, "UP");
    EXPECT_EQ(req.get_time(), 5);
    EXPECT_EQ(req.get_floor(), 2);
    EXPECT_EQ(req.get_button(), "UP");
}

TEST(SchedulerTest, AssignAndProcessRequests) {
    std::lock_guard<std::mutex> lock(queueMutex);
    requestQueue = std::queue<Request>();

    AddRequest(3, 7, "UP");
    AddRequest(4, 2, "DOWN");

    Scheduler scheduler;
    std::thread schedulerThread([&] { SchedulerFunction(scheduler, 2); });

    schedulerThread.join();
 
    EXPECT_EQ(schedulerState, SchedulerState::PROCESSING);

}

TEST(ElevatorTest, MovingBetweenStates) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        elevatorState = ElevatorState::MOVING;
    }
    cond.notify_all();

    std::this_thread::sleep_for(std::chrono::seconds(3));
    EXPECT_EQ(elevatorState, ElevatorState::ARRIVED);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
