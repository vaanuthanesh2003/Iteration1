#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include "client.h"  // Include the header, not the .cpp file

class MockClient {
public:
    std::string lastMessage;

    void sendRequest(int floor, std::string direction, int targetFloor) {
        lastMessage = std::to_string(floor) + " " + direction + " " + std::to_string(targetFloor);
    }
};

TEST(ClientTest, SendsCorrectRequestFormat) {
    MockClient mockClient;
    mockClient.sendRequest(3, "UP", 6);
   
    ASSERT_EQ(mockClient.lastMessage, "3 UP 6");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}