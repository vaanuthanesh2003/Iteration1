#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <iostream>
#include <netinet/in.h>
#include <unistd.h>  // For sleep()
#include <cstring>   // For memset()
#include <cstdlib>   // For exit()
#include <arpa/inet.h>  // For inet_pton()

#define BASE_PORT 5100
#define SCHEDULER_PORT 5002
#define SCHEDULER_IP "127.0.0.1"
#define DOOR_WAIT_TIME 2
#define BUFFER_SIZE 1024

class Elevator {
public:
    int id;
    int currentFloor;
    int sockfd;
    struct sockaddr_in schedulerAddr, selfAddr;

public:
    explicit Elevator(int elevator_id);
    virtual ~Elevator();

    virtual void sendStatus();
    virtual void receiveCommand();
    void moveTo(int targetFloor);
    void run();

    int getCurrentFloor() const { return currentFloor; }
    int getID() const { return id; }
    int main(int argc, char* argv[]);
};

#endif // ELEVATOR_H

