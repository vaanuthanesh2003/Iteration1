#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <string>
#include <netinet/in.h>

class Elevator {
public:
    explicit Elevator(int elevatorID);
    virtual ~Elevator();

    void receiveCommand();
    virtual void moveTo(int floor);
    virtual void sendStatus();
    virtual void sendFaultMessage(const std::string& message);
    virtual void reportHardFault();

    int getCurrentFloor() const { return currentFloor; }

protected:
    int id;
    int currentFloor;
    int sockfd;
    struct sockaddr_in schedulerAddr;

    // Wrappers to allow mocking
    virtual void sleepSec(int sec);
    virtual void exitProcess(int code);
    virtual int sendUDP(const std::string& msg);
};

#endif // ELEVATOR_H
