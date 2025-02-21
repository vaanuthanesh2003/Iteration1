#include "elevator_system.h"

std::queue<Request> requestQueue;
std::mutex queueMutex;
std::condition_variable cond;
ElevatorState elevatorState = ElevatorState::IDLE;
SchedulerState schedulerState = SchedulerState::WAITING;
bool running = true;

Elevator::Elevator() : currentFloor(0), state(ElevatorState::IDLE) {}

void Elevator::moveToFloor(int floor) {
    std::cout << "[Elevator] Moving from " << currentFloor << " to floor " << floor << "\n";
    state = ElevatorState::MOVING;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    currentFloor = floor;
    state = ElevatorState::ARRIVED;
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


void Elevator::start() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        cond.wait(lock, [] { return !requestQueue.empty(); });

        if (requestQueue.empty()) continue;

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        moveToFloor(req.get_floor());
    }
}

Request::Request(int t, int f, std::string b) : time(t), floor(f), button(b) {}
std::string Request::get_button() const { return button; }
int Request::get_floor() const { return floor; }
int Request::get_time() const { return time; }

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
    while (running && processed < maxRequests) {
        Request req = scheduler.getRequest();
        std::cout << "[Scheduler] Processing request for floor " << req.get_floor() << "\n";
        scheduler.sendResponse(1, req.get_floor());
        elevator.moveToFloor(req.get_floor());
        processed++;
    }
    running = false;
}

void AddRequest(int time, int floor, std::string direction){
    std::lock_guard<std::mutex> lock(queueMutex);
    requestQueue.push(Request(time, floor, direction));
    cond.notify_one();
}


void FloorSubsystem::processRequests(const std::string& filename) {
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


