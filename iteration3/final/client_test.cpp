#define TEST_BUILD
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include "client.h"

// Mock Client: No real networking
class MockClient : public Client {
public:
    void sendRequest(int floor, std::string direction, int targetFloor) override {
        std::cout << "[MOCK] Client sent request from Floor " << floor
                  << " to Floor " << targetFloor << " (" << direction << ")" << std::endl;
    }
};

// Test Sending Requests
TEST(ClientTest, SendsRequestsToScheduler) {
    MockClient client;
    testing::internal::CaptureStdout();
    client.sendRequest(3, "UP", 6);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("[MOCK] Client sent request"), std::string::npos);
}

int main(int argc, char **argv__) {
    ::testing::InitGoogleTest(&argc, argv__);
    return RUN_ALL_TESTS();
}

