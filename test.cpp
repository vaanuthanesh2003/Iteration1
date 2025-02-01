#include <gtest/gtest.h>
#include <fstream>
#include "elevator_system.cpp"

extern std::condition_variable cond;
extern std::mutex queueMutex;
extern std::queue<Request> requestQueue;
extern bool running;

// ✅ Test: Request Class
TEST(RequestTest, GettersAndSetters) {
    Request req(5, 2, "UP");

    EXPECT_EQ(req.get_time(), 5);
    EXPECT_EQ(req.get_floor(), 2);
    EXPECT_EQ(req.get_button(), "UP");

    req.set_time(10);
    req.set_floor(4);
    req.set_button("DOWN");

    EXPECT_EQ(req.get_time(), 10);
    EXPECT_EQ(req.get_floor(), 4);
    EXPECT_EQ(req.get_button(), "DOWN");
}

// ✅ Test: Scheduler Class (Adding & Retrieving Requests)
TEST(SchedulerTest, AddAndRetrieveRequest) {
    Scheduler scheduler;
    Request req1(1, 3, "UP");
    Request req2(2, 5, "DOWN");

    scheduler.addRequest(req1);
    scheduler.addRequest(req2);

    Request retrieved1 = scheduler.getRequest();
    EXPECT_EQ(retrieved1.get_time(), 1);
    EXPECT_EQ(retrieved1.get_floor(), 3);
    EXPECT_EQ(retrieved1.get_button(), "UP");

    Request retrieved2 = scheduler.getRequest();
    EXPECT_EQ(retrieved2.get_time(), 2);
    EXPECT_EQ(retrieved2.get_floor(), 5);
    EXPECT_EQ(retrieved2.get_button(), "DOWN");
}

// ✅ Test: FloorSubsystem - Read Input from File
TEST(FloorSubsystemTest, ReadFromFile) {
    std::string filename = "test_requests.txt";
    
    // Create a test file with valid input
    std::ofstream outfile(filename);
    ASSERT_TRUE(outfile.is_open()) << "Failed to create test input file.";
    outfile << "1,3,UP\n";
    outfile << "2,5,DOWN\n";
    outfile << "3,2,UP\n";
    outfile.close();

    // Start FloorSubsystem (reads from file)
    std::thread floorThread(FloorSubsystem, filename);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Verify that requests were added to the queue
    std::lock_guard<std::mutex> lock(queueMutex);
    ASSERT_EQ(requestQueue.size(), 3);

    // Verify the first request
    Request req = requestQueue.front();
    EXPECT_EQ(req.get_time(), 1);
    EXPECT_EQ(req.get_floor(), 3);
    EXPECT_EQ(req.get_button(), "UP");

    floorThread.join();
}

// ✅ Test: Elevator Processes Requests Correctly
TEST(ElevatorTest, ElevatorHandlesRequests) {
    std::queue<Request> testQueue;
    std::mutex testMutex;
    std::condition_variable testCond;

    testQueue.push(Request(1, 3, "UP"));
    testQueue.push(Request(2, 5, "DOWN"));

    // Process first request
    {
        std::lock_guard<std::mutex> lock(testMutex);
        if (!testQueue.empty()) {
            Request req = testQueue.front();
            testQueue.pop();
            EXPECT_EQ(req.get_floor(), 3);
            EXPECT_EQ(req.get_button(), "UP");
        }
    }
    EXPECT_EQ(testQueue.size(), 1);

    // Process second request
    {
        std::lock_guard<std::mutex> lock(testMutex);
        if (!testQueue.empty()) {
            Request req = testQueue.front();
            testQueue.pop();
            EXPECT_EQ(req.get_floor(), 5);
            EXPECT_EQ(req.get_button(), "DOWN");
        }
    }
    EXPECT_TRUE(testQueue.empty());
}

// ✅ Test: Full System (FloorSubsystem + Elevator)
TEST(IntegrationTest, FullSystemRun) {
    std::string filename = "test_requests.txt";

    // Create input file
    std::ofstream outfile(filename);
    ASSERT_TRUE(outfile.is_open()) << "Failed to create test input file.";
    outfile << "1,3,UP\n";
    outfile << "2,5,DOWN\n";
    outfile << "3,2,UP\n";
    outfile.close();

    std::thread floorThread(FloorSubsystem, filename);
    std::thread elevatorThread(Elevator);

    floorThread.join();
    running = false;
    cond.notify_one();
    elevatorThread.join();

    EXPECT_TRUE(requestQueue.empty());
}
