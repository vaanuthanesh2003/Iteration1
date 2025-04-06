#include "elevator_system.h"


std::queue<Request> requestQueue; // Stores incoming requests
std::mutex queueMutex; // To synchronize access to requestQueue
std::condition_variable cond; // Notifies when requests are available
ElevatorState elevatorState = ElevatorState::IDLE; // Tracks the current state of the elevator
SchedulerState schedulerState = SchedulerState::WAITING; // Tracks the current state of the scheduler
bool running = true; // Flag indicating whether the system is executing

// Constructor for Elevator class
Elevator::Elevator() : currentFloor(0), state(ElevatorState::IDLE) {} // Initialize the current floor to 0, and state to IDLE

// Move the elevator to a specific floor, simulating movement and door operations
void Elevator::moveToFloor(int floor) {
    std::cout << "[Elevator] Moving from " << currentFloor << " to floor " << floor << "\n";
    state = ElevatorState::MOVING; // Set the state to MOVING
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate elevator movement with a delay

    currentFloor = floor; // Update the current floor
    state = ElevatorState::ARRIVED; // Set the state to ARRIVED
    std::cout << "[Elevator] Arrived at floor " << currentFloor << "\n";

    // Simulate door opening
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "[Elevator] Doors opening\n";
    state = ElevatorState::DOORS_OPEN;

    // Simulate door closing
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "[Elevator] Doors closing\n";
    state = ElevatorState::DOORS_CLOSED;
}

// Start the elevator's main loop to process requests
void Elevator::start(FloorSubsystem& floorSubsystem) {
    std::cout << "[Elevator] Elevator started ... \n";

    while (running) {
        std::unique_lock <std::mutex> lock(queueMutex);
        cond.wait(lock, [] { return !requestQueue.empty(); }); // Wait until requests are available

        if (requestQueue.empty()) continue;

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        std::cout << "[Elevator] Moving from " << currentFloor << " to Floor " << req.get_floor() << "\n";

        moveToFloor(req.get_floor()); // Move to the requested floor
        std:: cout << "[Elevator] Picked up passenger at Floor " << req.get_floor() << " and moving to requested Floor " << req.get_car() << "\n";

        moveToFloor(req.get_car()); // Move to the target floor (destination)
        std::cout << "[Elevator] Passenger dropped off at Floor " << req.get_car() << "\n";

        // Handle any additional requests at the current floor
        while(true) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!requestQueue.empty() && requestQueue.front().get_floor() == currentFloor){
                Request nextReq = requestQueue.front();
                requestQueue.pop();
                std::cout << "[Elevator] Servicing additional request at floor " << currentFloor << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
                moveToFloor(nextReq.get_car());
            } else {
                break;
            }
        }
    }
}

// Constructor for Request class to initialize request properties
Request::Request(int t, int f, std::string b, int c) : time(t), floor(f), button(b), car(c) {}

// Getters for request properties
std::string Request::get_button() const { return button; }
int Request::get_floor() const { return floor; }
int Request::get_time() const { return time; }
int Request::get_car() const { return car; }

// Add a request to the scheduler's request queue
void Scheduler::addRequest(const Request& request) { 
    std::unique_lock<std::mutex> lock(queueMutex); // Locking the request queue
    requestQueue.push(request); // Add the request to the queue
    cond.notify_one(); // Notify that a request is available
}

// Get a request from the scheduler's request queue
Request Scheduler::getRequest() {
    std::unique_lock<std::mutex> lock(queueMutex); // Locking the request queue
    cond.wait(lock, [] { return !requestQueue.empty(); }); // Wait until there is a request in the queue
    Request request = requestQueue.front(); // Get the front request
    requestQueue.pop(); // Remove the front request from the queue
    return request;
}

// Send a response to the elevator indicating it has reached a floor
void Scheduler::sendResponse(int elevatorID, int floor) {
    std::cout << "[Scheduler] Elevator " << elevatorID << " reached floor " << floor << std::endl;
}

// Main function that handles the scheduler's execution loop
void SchedulerFunction(Scheduler& scheduler, int maxRequests, Elevator& elevator) {
    int processed = 0;
    std::cout << "[Scheduler] Starting scheduler ... \n";
    while (running && processed < maxRequests) {
        std::unique_lock<std::mutex> lock(queueMutex);
        std::cout << "[Scheduler] Waiting for a request ... Queue size: " << requestQueue.size() << "\n";
        cond.wait(lock, [] { return !requestQueue.empty(); });

        if (requestQueue.empty()) {
            std:: cout << "[Scheduler] No requests available, continuing ... \n";
            continue;
        }
        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        std::cout << "[Scheduler] Processing request for floor " << req.get_floor() <<  " (Destination: " << req.get_car() << ")\n";
        scheduler.sendResponse(1, req.get_floor());
        elevator.moveToFloor(req.get_floor());
        std::cout << "[Elevator] Passenger picked up at floor " << req.get_floor() << " , going to Floor " << req.get_car() << "\n";
        elevator.moveToFloor(req.get_car());
        std::cout << "[Elevator] Passenger dropped off at floor " << req.get_car() << "\n";

        processed++;

        if (processed >= maxRequests && !requestQueue.empty()){
            std::cout << "[Scheduler] Processed maxRequests, but queue still has items! Continuing ...\n";
            processed = 0;
        }
    }
    std::cout << "[Scheduler] All requests processed, exiting ... \n";
    running = false;
}

// Function to add a request to the global request queue
void AddRequest(int time, int floor, std::string direction, int car){
    std::lock_guard<std::mutex> lock(queueMutex); // Obtaining lock for requestQueue
    requestQueue.push(Request(time, floor, direction, car)); // Add request to the queue
    cond.notify_one(); // Notify a waiting thread that a new request has been added
}

// FloorSubsystem class handles floor-specific operations like processing requests
void FloorSubsystem::processRequests(const std::string& filename) {
    std::ifstream inputFile(filename); // Open the file containing floor requests
    if (!inputFile) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    // Reads requests from file and processes them
    std::string line;
    while (std::getline(inputFile, line)) {
        std::istringstream iss(line); // Read each line as an input stream
        int time, floor, car;
        std::string direction;
        iss >> time >> floor >> direction >> car;

        if (iss.fail()) {
            std::cerr << "Error reading line: " << line << std::endl;
            continue; // Skip invalid lines
        }

        requestElevator(time, floor, direction, car); // Request an elevator
    }
    inputFile.close(); // Close the file after processing all requests
}

// Request an elevator for a specific floor
void FloorSubsystem::requestElevator(int time, int floor, std::string direction, int car){
    {
        std::lock_guard<std::mutex> lock(queueMutex); // Locking the queue for request submission
        Request req(time, floor, direction, car);
        requestQueue.push(req); // Add the new request to the global queue
        
        // Separate requests by direction for each floor
        if (direction == "UP"){
            upRequests[floor].push(req);
        } else {
            downRequests[floor].push(req);
        }
        std::cout << "[FloorSubSystem] Request at Floor " << floor << " (" << direction << ") to Floor " << car << " added to queue.\n";
    }
    cond.notify_one(); // Notify that the new request has been added
}

// Retrieve the next request for a given floor and direction
Request FloorSubsystem::getNextRequest(int floor, std::string direction){
    std::lock_guard<std::mutex> lock(queueMutex); // Locking the queue for retrieval
    if (direction == "UP" && !upRequests[floor].empty()){
        Request req = upRequests[floor].front();
        upRequests[floor].pop();
        return req; // Return the request and remove it from the queue
    }
    if (direction == "DOWN" && !downRequests[floor].empty()){
        Request req = downRequests[floor].front();
        downRequests[floor].pop();
        return req; // Return the request and remove it from the queue
    }
    return Request(0, 0, "", 0); // Return an empty request if none is available
}

// Check if there are any requests for a given floor and direction
bool FloorSubsystem::hasRequests(int floor, std::string direction){
    std::lock_guard<std::mutex> lock(queueMutex); // Locking the queue for checking
    return (direction == "UP" && !upRequests[floor].empty()) || (direction == "DOWN" && !downRequests[floor].empty());
}

#ifndef TESTING
// Main function to start the system and handle threads
int main(int argc, char* argv[]){
    if (argc != 3){
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1]; // Input file containing requests
    int maxRequests = std::stoi(argv[2]); // Maximum number of requests to process

    FloorSubsystem floorSubsystem;  // Create a FloorSubsystem object
    Scheduler scheduler;  // Create a Scheduler object
    Elevator elevator;  // Create an Elevator object
    
    std::cout << "[Main] Starting system with maxRequests = " << maxRequests << "\n";
    
    // Start threads for each subsystem
    std::thread floorThread([&]() { floorSubsystem.processRequests(filename);});
    std::thread schedulerThread([&]() { SchedulerFunction(scheduler, maxRequests, elevator);});
    std::thread elevatorThread([&] {elevator.start(floorSubsystem); });

    // Wait for all threads to complete
    floorThread.join();
    schedulerThread.join();
    elevatorThread.join();

    return 0;
}
#endif
