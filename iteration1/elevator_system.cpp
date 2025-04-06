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

bool running = true; // Flag to control the running state of the system

// Request class represents a request with time, floor, and button direction
class Request {
private:
    int time;  // Time the request was made
    int floor; // Floor where the request was made
    std::string button; // Direction of the button press (e.g., "UP" or "DOWN")

public:
    // Constructor to initialize the request object
    Request(int t, int f, std::string b) : time(t), floor(f), button(b) {}

    // Getter methods for accessing private members
    std::string get_button() const { return button; }   
    int get_floor() const { return floor; }            
    int get_time() const { return time; }              

    // Setter methods for modifying private members
    void set_button(std::string b) { button = b; }
    void set_floor(int f) { floor = f; }
    void set_time(int t) { time = t; }

    // Method to print the request details
    void Tostring() const {   
        std::cout << "Button: " << button << std::endl;
        std::cout << "Floor: " << floor << std::endl;
        std::cout << "Time: " << time << std::endl;
    }
};

// Global variables used for handling requests and synchronization
std::queue<Request> requestQueue;  // Queue to store incoming requests
std::mutex queueMutex; // Mutex to synchronize access to the request queue
std::condition_variable cond; // Condition variable to notify threads when requests are available

// Elevator function simulates the elevator processing requests from the queue
void Elevator() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cond.wait(lock, [] { return !requestQueue.empty(); }); // Wait until requests are available

        Request req = requestQueue.front();  // Get the front request from the queue
        requestQueue.pop();  // Remove the request from the queue
        lock.unlock();

        std::cout << "[Elevator] Moving to floor " << req.get_floor() << " to handle request.\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));  // Simulate the time taken to move

        std::cout << "[Elevator] Request from floor " << req.get_floor() << " handled.\n";
    }
}

// FloorSubsystem function reads requests from the input file and adds them to the request queue
void FloorSubsystem(const std::string& filename) {
    std::ifstream inputFile(filename);  // Open the file for reading
    if (!inputFile) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(inputFile, line)) {  // Read each line from the file
        std::istringstream iss(line);  // Use a string stream to parse the line
        int time, floor;
        std::string direction;
        iss >> time >> floor >> direction;  // Parse the time, floor, and direction from the line

        // Validate the data before using it
        if (iss.fail()) {
            std::cerr << "Error reading line: " << line << std::endl;
            continue;
        }

        // Simulate button press determining the destination floor
        int destination = (direction == "UP") ? floor + 1 : floor - 1;

        // Create the request object
        Request req(time, floor, direction);

        // Add the request to the shared request queue
        {
            std::lock_guard<std::mutex> lock(queueMutex);  // Lock the queue for thread-safe operation
            requestQueue.push(req);  // Add the request to the queue
            std::cout << "[Floor] Request from floor " << floor << " (" << direction << ") added to queue.\n";
        }

        // Notify the Elevator thread that a new request is in the queue
        cond.notify_one();

        // Simulate a small delay between requests (if needed)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    inputFile.close();  // Close the file when done
}

// Scheduler class for managing the request queue and processing requests
class Scheduler {
public:
    // Add a new request to the scheduler's request queue
    void addRequest(const Request& request) {
        std::unique_lock<std::mutex> lock(mutex_);
        requestQueue_.push(request);  // Add the request to the queue
        std::cout << "Scheduler received request: Time " << request.get_time() 
                  << ", Floor " << request.get_floor() 
                  << ", Button " << request.get_button() << std::endl;
        cv_.notify_one();  // Notify the waiting thread that a request is available
    }

    // Get the next request from the scheduler's request queue
    Request getRequest() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !requestQueue_.empty(); }); // Wait until there is a request in the queue
        Request request = requestQueue_.front(); // Get the front request
        requestQueue_.pop(); // Remove the front request from the queue
        return request;
    }

    // Send response to the scheduler indicating that the elevator has reached the requested floor
    void sendResponse(int elevatorID, int floor) {
        std::unique_lock<std::mutex> lock(mutex_);
        std::cout << "Scheduler: Elevator " << elevatorID 
                  << " has processed request and reached floor " << floor << std::endl;
    }

private:
    std::queue<Request> requestQueue_; // Queue to store the requests
    std::mutex mutex_; // Mutex to synchronize access to the request queue
    std::condition_variable cv_; // Condition variable to notify when requests are available
};

// Main function to start the system and run the elevator and floor subsystem in separate threads
#ifndef TESTING // Only compile main() if not testing
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];  // Get the input file name from the command-line argument

    // Use a lambda to capture the filename correctly and start the FloorSubsystem in a separate thread
    std::thread floorThread([filename]() { FloorSubsystem(filename); });

    // Start the Elevator thread
    std::thread elevatorThread(Elevator);

    // Wait for both threads to finish
    floorThread.join();
    elevatorThread.join();

    return 0;
}
#endif
