#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class Client {
public:
    Client();
    void sendRequest(int floor, std::string direction, int targetFloor);
};

#endif // CLIENT_H
