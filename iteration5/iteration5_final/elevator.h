#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <netinet/in.h>
#include <string>

#define BASE_PORT 5100
#define SCHEDULER_PORT 5002
#define SCHEDULER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

class Elevator {
private:
    int id;
    int currentFloor;
    int movementCount;
    int capacity;
    int load;
    int sockfd;
    bool stuck;
    bool doorStuck;
    struct sockaddr_in schedulerAddr;

public:
    Elevator(int elevatorID);
    ~Elevator();

    virtual void receiveCommand();
    void moveTo(int floor);
    virtual void sendStatus();
    void reportHardFault();
    void sendFaultMessage(const std::string& message);
    
    int getID() const;
    int getCurrentFloor() const;
    int getMovementCount() const;
    int getLoad() const;
};

#endif // ELEVATOR_H

