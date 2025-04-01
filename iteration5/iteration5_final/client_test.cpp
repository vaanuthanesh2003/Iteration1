#define TEST_BUILD
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include "client.h"

class MockClient : public Client {
public:
    void sendRequest(int floor, std::string direction, int targetFloor) override {
        std::cout << "[MOCK] Client sent request from Floor " << floor
                  << " to Floor " << targetFloor << " (" << direction << ")" << std::endl;
    }
};

TEST(ClientTest, SendsRequestsToScheduler) {
    MockClient client;
    testing::internal::CaptureStdout();
    client.sendRequest(1, "UP", 5);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("Client sent request"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
