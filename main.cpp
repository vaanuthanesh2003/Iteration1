#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <sstream>

class Request {
private:
    int time;
    int floor;
    std::string button;

public:
    Request(int t, int f, std::string b) : time(t), floor(f), button(b) {}

    std::string get_button() const { return button; }   // Add "const" here
    int get_floor() const { return floor; }            // Add "const" here
    int get_time() const { return time; }              // Add "const" here

    void set_button(std::string b) { button = b; }
    void set_floor(int f) { floor = f; }
    void set_time(int t) { time = t; }

    void Tostring() const {   // Mark this as const too
        std::cout << "Button: " << button << std::endl;
        std::cout << "Floor: " << floor << std::endl;
        std::cout << "Time: " << time << std::endl;
    }
};

// Global variables
std::queue<Request> requestQueue;
std::mutex queueMutex;
std::condition_variable cond;

void Elevator() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cond.wait(lock, [] { return !requestQueue.empty(); });

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        std::cout << "[Elevator] Moving to floor " << req.get_floor() << " to handle request.\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::cout << "[Elevator] Request from floor " << req.get_floor() << " handled.\n";
    }
}

void FloorSubsystem() {
    std::vector<std::string> inputEvents = {
        "1 3 UP",
        "2 5 DOWN",
        "3 2 UP"
    };

    for (const auto& event : inputEvents) {
        std::istringstream iss(event);
        int time, floor;
        std::string direction;
        iss >> time >> floor >> direction;

        Request req(time, floor, direction);

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            requestQueue.push(req);
            std::cout << "[Floor] Request from floor " << floor << " (" << direction << ") added to queue.\n";
        }

        cond.notify_one();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

class Scheduler {
public:
    void addRequest(const Request& request) {
        std::unique_lock<std::mutex> lock(mutex_);
        requestQueue_.push(request);
        std::cout << "Scheduler received request: Time " << request.get_time() 
                  << ", Floor " << request.get_floor() 
                  << ", Button " << request.get_button() << std::endl;
        cv_.notify_one();
    }

    Request getRequest() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !requestQueue_.empty(); });
        Request request = requestQueue_.front();
        requestQueue_.pop();
        return request;
    }

    void sendResponse(int elevatorID, int floor) {
        std::unique_lock<std::mutex> lock(mutex_);
        std::cout << "Scheduler: Elevator " << elevatorID 
                  << " has processed request and reached floor " << floor << std::endl;
    }

private:
    std::queue<Request> requestQueue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

int main() {
    std::thread floorThread(FloorSubsystem);
    std::thread elevatorThread(Elevator);

    floorThread.join();
    elevatorThread.join();

    return 0;
}
