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


enum class ElevatorState { IDLE, MOVING, DOORS_OPEN, DOORS_CLOSED, ARRIVED };
enum class SchedulerState { WAITING, ASSIGNING, PROCESSING };


class Request {
private:
    int time;
    int floor;
    std::string button;

public:
    Request(int t, int f, std::string b);
    std::string get_button() const;
    int get_floor() const;
    int get_time() const;
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
public:
    void processRequests(const std::string& filename);
};


void SchedulerFunction(Scheduler& scheduler, int maxRequests);
void Elevator();
void AddRequest(int time, int floor, std::string direction);

#endif