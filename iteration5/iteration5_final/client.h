#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <netinet/in.h>

class Client {

private:
    int sockfd;
    struct sockaddr_in schedulerAddr;
public:
    Client();
    virtual void sendRequest(int floor, std::string direction, int targetFloor);
    void processRequestsFromFile(const std::string& filename);
    ~Client();
    int getSecondsFromTimestamp(const std::string& timestamp);
    
};

#endif // CLIENT_H
