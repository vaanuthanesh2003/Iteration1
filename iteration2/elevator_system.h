#ifndef ELEVATOR_SYSTEM_H
#define ELEVATOR_SYSTEM_H

#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <map>

enum class ElevatorState { IDLE, MOVING, DOORS_OPEN, DOORS_CLOSED, ARRIVED };
enum class SchedulerState { WAITING, ASSIGNING, PROCESSING };


class Request {
private:
    int time;
    int floor;
    std::string button;
    int car;

public:
    Request(int t, int f, std::string b, int c);
    std::string get_button() const;
    int get_floor() const;
    int get_time() const;
    int get_car() const;
};


extern std::queue<Request> requestQueue;
extern std::mutex queueMutex;
extern std::condition_variable cond;
extern ElevatorState elevatorState;
extern SchedulerState schedulerState;
extern bool running;


class Scheduler {
public:
    void addRequest(const Request& request);
    Request getRequest();
    void sendResponse(int elevatorID, int floor);
};


class FloorSubsystem {

private:
    std::map<int, std::queue<Request>> upRequests;
    std::map<int, std::queue<Request>> downRequests;

public:
    void processRequests(const std::string& filename);
    void requestElevator(int time, int floor, std::string direction, int car);
    Request getNextRequest(int floor, std::string direction);
    bool hasRequests(int floor, std::string direction);
};

class Elevator {
private:
    int currentFloor;
    ElevatorState state;

public:
    Elevator();
    void moveToFloor(int floor);
    void start(FloorSubsystem& floorSubsystem);  // Starts the elevator thread
};


void SchedulerFunction(Scheduler& scheduler, int maxRequests, Elevator& elevator);
void AddRequest(int time, int floor, std::string direction, int car);

#endif