#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>

// Scheduler class acting as the communication hub
class Scheduler {
public:
    // Add a request from the Floor subsystem
    void addRequest(const ElevatorRequest& request) {
        std::unique_lock<std::mutex> lock(mutex_);
        requestQueue_.push(request);
        std::cout << "Scheduler received request: Time " << request.time 
                  << ", Floor " << request.floor 
                  << ", Button " << request.button << std::endl;
        cv_.notify_one();  // Notify an elevator thread
    }

    // Get a request for an Elevator to process
    ElevatorRequest getRequest() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !requestQueue_.empty(); }); // Wait for a request
        ElevatorRequest request = requestQueue_.front();
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
    std::queue<ElevatorRequest> requestQueue_;  // Queue for storing requests
    std::mutex mutex_;                          // Mutex for thread safety
    std::condition_variable cv_;                // Condition variable for synchronization
};
