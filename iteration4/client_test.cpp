#include <gtest/gtest.h>
#include "client.h"

class ClientTest : public ::testing::Test {
protected:
    Client client;
};

TEST_F(ClientTest, SendRequestFormat) {
    testing::internal::CaptureStdout();
    client.sendRequest(3, "up", 7);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Floor 3 -> Floor 7 (up)"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
