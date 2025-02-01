#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <sstream>

// Struct to define an elevator request
struct Request {
    int source;      // Origin floor
    int destination; // Target floor
};

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
