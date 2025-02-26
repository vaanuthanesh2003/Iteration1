#include "elevator_system.h"


std::queue<Request> requestQueue; // stores incoming requests
std::mutex queueMutex; // to sync access to requestQueue
std::condition_variable cond; // notifies when requests are available
ElevatorState elevatorState = ElevatorState::IDLE; // to track current state
SchedulerState schedulerState = SchedulerState::WAITING; // to track current state
bool running = true; // executing condition

Elevator::Elevator() : currentFloor(0), state(ElevatorState::IDLE) {} // initializing current floor to 0, state - IDLE

void Elevator::moveToFloor(int floor) {
    std::cout << "[Elevator] Moving from " << currentFloor << " to floor " << floor << "\n";
    state = ElevatorState::MOVING;
    std::this_thread::sleep_for(std::chrono::seconds(2)); // simulates elevator moving with delay

    currentFloor = floor;
    state = ElevatorState::ARRIVED; // updating state
    std::cout << "[Elevator] Arrived at floor " << currentFloor << "\n";

    // door opening
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "[Elevator] Doors opening\n";
    state = ElevatorState::DOORS_OPEN;

    // door closing
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "[Elevator] Doors closing\n";
    state = ElevatorState::DOORS_CLOSED;
}


void Elevator::start(FloorSubsystem& floorSubsystem) {
    std::cout << "[Elevator] Elevator started ... \n";

    while (running) {
        std::unique_lock <std::mutex> lock(queueMutex);
        cond.wait(lock, [] { return !requestQueue.empty(); }); // wait until requests are available

        if (requestQueue.empty()) continue;

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        std::cout << "[Elevator] Moving from " << currentFloor << " to Floor " << req.get_floor() << "\n";

        moveToFloor(req.get_floor());
        std:: cout << "[Elevator] Picked up passenger at Floor " << req.get_floor() << " and moving to requested Floor " << req.get_car() << "\n";

        moveToFloor(req.get_car());
        std::cout << "[Elevator] Passenger dropped off at Floor " << req.get_car() << "\n";

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


Request::Request(int t, int f, std::string b, int c) : time(t), floor(f), button(b), car(c){}
std::string Request::get_button() const { return button; }
int Request::get_floor() const { return floor; }
int Request::get_time() const { return time; }
int Request::get_car() const { return car;}

void Scheduler::addRequest(const Request& request) { 
    std::unique_lock<std::mutex> lock(queueMutex);
    requestQueue.push(request);
    cond.notify_one();
}

Request Scheduler::getRequest() {
    std::unique_lock<std::mutex> lock(queueMutex);
    cond.wait(lock, [] { return !requestQueue.empty(); });
    Request request = requestQueue.front();
    requestQueue.pop();
    return request;
}

void Scheduler::sendResponse(int elevatorID, int floor) {
    std::cout << "[Scheduler] Elevator " << elevatorID << " reached floor " << floor << std::endl;
}

void SchedulerFunction(Scheduler& scheduler, int maxRequests, Elevator& elevator) {
    int processed = 0;
    std::cout << "[Scheduler] Starting scheduler ... \n";
    while (running && processed < maxRequests) {
        std::unique_lock<std::mutex> lock(queueMutex);
        std::cout << "[Scheduler] Waiting for a request ... Queue size: " << requestQueue.size() << "\n";
        cond.wait(lock, [] { return !requestQueue.empty(); });
        //std::cout << "[Scheduler] Waiting for a request ... Queue size: " << requestQueue.size() << "\n";

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

void AddRequest(int time, int floor, std::string direction, int car){
    std::lock_guard<std::mutex> lock(queueMutex); // obtains lock for requestQueue
    requestQueue.push(Request(time, floor, direction, car));
    cond.notify_one(); // notify a waiting thread that we are done
}


void FloorSubsystem::processRequests(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    // reads requests from file
    std::string line;
    while (std::getline(inputFile, line)) {
        std::istringstream iss(line);
        int time, floor, car;
        std::string direction;
        iss >> time >> floor >> direction >> car;

        if (iss.fail()) {
            std::cerr << "Error reading line: " << line << std::endl;
            continue;
        }

        requestElevator(time, floor, direction, car);
    }
    inputFile.close();
}

void FloorSubsystem::requestElevator(int time, int floor, std::string direction, int car){
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        // obtains lock for requestQueue
        Request req(time, floor, direction, car);
        requestQueue.push(req); // adds a new floor request to the queue
        
        if (direction == "UP"){
            upRequests[floor].push(req);
        } else {
            downRequests[floor].push(req);
        }
        std::cout << "[FloorSubSystem] Request at Floor " << floor << " (" << direction << ") to Floor " << car << " added to queue.\n";
    }
    cond.notify_one();  
}

Request FloorSubsystem::getNextRequest(int floor, std::string direction){
    std::lock_guard<std::mutex> lock(queueMutex);
    // obtains lock for requestQueue
    if (direction == "UP" && !upRequests[floor].empty()){
        Request req = upRequests[floor].front();
        upRequests[floor].pop();
        return req;
    }
    if (direction == "DOWN" && !downRequests[floor].empty()){
        Request req = downRequests[floor].front();
        downRequests[floor].pop();
        return req;
    }
    return Request(0, 0, "", 0); // return empty request

}

bool FloorSubsystem::hasRequests(int floor, std::string direction){
    std::lock_guard<std::mutex> lock(queueMutex);
    // obtains lock for requestQueue
    return (direction == "UP" && !upRequests[floor].empty()) || (direction == "DOWN" && !downRequests[floor].empty());
}

#ifndef TESTING
int main(int argc, char* argv[]){
    if (argc != 3){
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    int maxRequests = std::stoi(argv[2]);

    FloorSubsystem floorSubsystem;
    Scheduler scheduler;
    Elevator elevator;
    
    std::cout << "[Main] Starting system with maxRequests = " << maxRequests << "\n";
    


    std:: thread floorThread([&]() { floorSubsystem.processRequests(filename);});
    std:: thread schedulerThread([&]() { SchedulerFunction(scheduler, maxRequests, elevator);});
    std::thread elevatorThread([&] {elevator.start(floorSubsystem); });

    floorThread.join();
    schedulerThread.join();
    elevatorThread.join();

    return 0;
}

#endif

