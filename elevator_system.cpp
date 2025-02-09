#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>

bool running = true;

enum class ElevatorState { IDLE, MOVING, DOORS_OPEN, DOORS_CLOSED, ARRIVED };
enum class SchedulerState { WAITING, ASSIGNING, PROCESSING };

class Request {
private:
    int time;
    int floor;
    std::string button;

public:
    Request(int t, int f, std::string b) : time(t), floor(f), button(b) {}

    std::string get_button() const { return button; }  
    int get_floor() const { return floor; }            
    int get_time() const { return time; }              

    void set_button(std::string b) { button = b; }
    void set_floor(int f) { floor = f; }
    void set_time(int t) { time = t; }

    void Tostring() const {  
        std::cout << "Button: " << button << std::endl;
        std::cout << "Floor: " << floor << std::endl;
        std::cout << "Time: " << time << std::endl;
    }
};

// Global variables
std::queue<Request> requestQueue;
std::mutex queueMutex;
std::condition_variable cond;
ElevatorState elevatorState = ElevatorState::IDLE;
SchedulerState schedulerState = SchedulerState::WAITING;

class FloorSubsystem {
public:
    void processRequests(const std::string& filename) {
        std::ifstream inputFile(filename);
        if (!inputFile) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        std::string line;
        while (std::getline(inputFile, line)) {
            std::istringstream iss(line);
            int time, floor;
            std::string direction;
            iss >> time >> floor >> direction;

            if (iss.fail()) {
                std::cerr << "Error reading line: " << line << std::endl;
                continue;
            }

            Request req(time, floor, direction);
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                requestQueue.push(req);
                std::cout << "[Floor] Request from floor " << floor << " (" << direction << ") added to queue.\n";
            }
            cond.notify_one();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        inputFile.close();
    }
};

void Scheduler() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cond.wait(lock, [] { return !requestQueue.empty(); });
       
        schedulerState = SchedulerState::ASSIGNING;
        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        std::cout << "[Scheduler] Assigning request for floor " << req.get_floor() << "\n";
        schedulerState = SchedulerState::PROCESSING;
       
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "[Scheduler] Request processed, notifying elevator\n";
        elevatorState = ElevatorState::MOVING;
       
        cond.notify_all();
    }
}

void Elevator() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cond.wait(lock, [] { return elevatorState == ElevatorState::MOVING; });
        lock.unlock();
       
        std::cout << "[Elevator] Moving..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "[Elevator] Arrived at requested floor" << std::endl;
        elevatorState = ElevatorState::ARRIVED;
       
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "[Elevator] Doors opening\n";
        elevatorState = ElevatorState::DOORS_OPEN;
       
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "[Elevator] Doors closing\n";
        elevatorState = ElevatorState::DOORS_CLOSED;
       
        schedulerState = SchedulerState::WAITING;
        elevatorState = ElevatorState::IDLE;
        cond.notify_all();
    }
}

void AddRequest(int time, int floor, std::string direction) {
    std::lock_guard<std::mutex> lock(queueMutex);
    requestQueue.push(Request(time, floor, direction));
    cond.notify_all();
}

#ifndef TESTING
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    FloorSubsystem floorSubsystem;
    std::thread floorThread([&]() { floorSubsystem.processRequests(filename); });
    std::thread schedulerThread(Scheduler);
    std::thread elevatorThread(Elevator);

    floorThread.join();
    schedulerThread.join();
    elevatorThread.join();

    return 0;
}
#endif

