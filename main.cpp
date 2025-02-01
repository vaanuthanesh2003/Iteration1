#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <conditional_variable>
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

    std::string get_button() { return button; }
    int get_floor() { return floor; }
    int get_time() { return time; }

    void set_button(std::string b) { button = b; }
    void set_floor(int f) { floor = f; }
    void set_time(int t) { time = t; }

    void Tostring() {
        std::cout << "Button: " << button << std::endl;
        std::cout << "Floor: " << floor << std::endl;
        std::cout << "Time: " << time << std::endl;
    }
};

// initializing global variables

std::queue<Request> requestQueue;
std:: mutex queueMutex;
std:: conditional_variable cond;

void Elevator(){
    
    while (true){
        // in this class, the "processing" of a request is what will simulate the car moving
        std::unique_lock<std::mutex> lock(queueMutex); // receive lock to access shared queue
        cond.wait(lock, [] { return !requestQueue.empty(); }); // wait for a request in the queue

        Request req = requestQueue.front(); // this allows us to get the first request from the queue (FIFO)
        requestQueue.pop(); // removes top request from queue as it is being handled
        lock.unlock(); // release lock to allow other threads to access it

        std::cout << "[Elevator] Moving to floor " << req.source << " to handle request.\n";
        std::this_thread::sleep_for(std::chrono::seconds(3)); // this will simulate "moving" to requested floor

        std::cout << "[Elevator] Request from floor " << req.source << " handled.\n" // output that request is handled
    }

}

std::queue<Request> requestQueue;
std::mutex queueMutex;
std::condition_variable cond;

void FloorSubsystem() {
    std::vector<std::string> inputEvents = {
        "1 3 UP",  // Example: Request from floor 3 going UP
        "2 5 DOWN", 
        "3 2 UP"
    };

    for (const auto& event : inputEvents) {
        std::istringstream iss(event);
        int time, floor;
        std::string direction;
        iss >> time >> floor >> direction;

        // Simulate button press determining the destination floor
        int destination = (direction == "UP") ? floor + 1 : floor - 1;

        // Create request
        Request req = {floor, destination};

        // Add to shared queue
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            requestQueue.push(req);
            std::cout << "[Floor] Request from floor " << floor << " (" << direction << ") added to queue.\n";
        }
        
        cond.notify_one(); // Notify Elevator thread
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate delay in new requests
    }
}

// Scheduler class acting as the communication hub
class Scheduler {
public:
    // Add a request from the Floor subsystem
    void addRequest(const Request& request) {
        std::unique_lock<std::mutex> lock(mutex_);
        requestQueue_.push(request);
        std::cout << "Scheduler received request: Time " << request.time 
                  << ", Floor " << request.floor 
                  << ", Button " << request.button << std::endl;
        cv_.notify_one();  // Notify an elevator thread
    }

    // Get a request for an Elevator to process
    Request getRequest() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !requestQueue_.empty(); }); // Wait for a request
        Request request = requestQueue_.front();
        requestQueue_.pop();
        return request;
    }

    // Send the response back to the Floor system (placeholder for now)
    void sendResponse(int elevatorID, int floor) {
        std::unique_lock<std::mutex> lock(mutex_);
        std::cout << "Scheduler: Elevator " << elevatorID 
                  << " has processed request and reached floor " << floor << std::endl;
    }

private:
    std::queue<Request> requestQueue_;  // Queue for storing requests
    std::mutex mutex_;                          // Mutex for thread safety
    std::condition_variable cv_;                // Condition variable for synchronization
};
