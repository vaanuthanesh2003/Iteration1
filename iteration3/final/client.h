#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class Client {
public:
    Client();
    virtual void sendRequest(int floor, std::string direction, int targetFloor);
    void processRequestsFromFile();
    
};

#endif // CLIENT_H
