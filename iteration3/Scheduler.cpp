#include <iostream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <climits>
#include <memory>

#define FLOOR_PORT 5001       // Port for FloorSubsystem
#define ELEVATOR_PORT 5002     // Port for ElevatorSubsystem
#define SCHEDULER_PORT 5003    // Scheduler's listening port

using namespace std;

// Request structure
struct Request {
    int time;
    int floor;
    int targetFloor;
    string direction;

    Request(int t, int f, int tf, string d) : time(t), floor(f), targetFloor(tf), direction(d) {}
};

// Elevator structure
struct Elevator {
    int id;
    int currentFloor;
    bool isMoving;
    string direction;
    queue<Request> taskQueue;

    Elevator(int id, int cf) : id(id), currentFloor(cf), isMoving(false), direction("IDLE") {}
};

// Scheduler Class
class Scheduler {
private:
    unordered_map<int, Elevator> elevators;  // Tracks all elevators
    queue<Request> requestQueue;  // Stores incoming elevator requests
    mutex queueMutex;
    condition_variable cond;
    bool running;
    int sockfd;
    struct sockaddr_in floorAddr, elevatorAddr;

public:
    Scheduler(int numElevators);
    void addRequest(const Request& req);
    void processRequests();
    int selectBestElevator(const Request& req);
    void sendCommandToElevator(int elevatorID, const Request& req);
    void receiveRequests();
    void sendArrivalSignal(int floor);
    void run();
};

// Constructor: Initializes elevators and UDP socket
Scheduler::Scheduler(int numElevators) : running(true) {
    for (int i = 1; i <= numElevators; i++) {
        elevators[i] = Elevator(i, 0);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SCHEDULER_PORT);

    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    floorAddr.sin_family = AF_INET;
    floorAddr.sin_port = htons(FLOOR_PORT);
    floorAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    elevatorAddr.sin_family = AF_INET;
    elevatorAddr.sin_port = htons(ELEVATOR_PORT);
    elevatorAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
}

// Adds request to queue and notifies the processing thread
void Scheduler::addRequest(const Request& req) {
    unique_lock<mutex> lock(queueMutex);
    requestQueue.push(req);
    cout << "[Scheduler] New request from Floor " << req.floor << " to " << req.targetFloor << " (" << req.direction << ")\n";
    cond.notify_one();
}

// Processes requests and assigns elevators
void Scheduler::processRequests() {
    while (running) {
        unique_lock<mutex> lock(queueMutex);
        cond.wait(lock, [this]() { return !requestQueue.empty(); });

        Request req = requestQueue.front();
        requestQueue.pop();
        lock.unlock();

        int chosenElevator = selectBestElevator(req);
        if (chosenElevator != -1) {
            elevators[chosenElevator].taskQueue.push(req);
            sendCommandToElevator(chosenElevator, req);
        }
    }
}

// Selects the best elevator based on distance and direction
int Scheduler::selectBestElevator(const Request& req) {
    int bestElevator = -1;
    int minDistance = INT_MAX;

    for (auto& [id, elevator] : elevators) {
        int distance = abs(elevator.currentFloor - req.floor);
        if (!elevator.isMoving || (elevator.direction == req.direction && distance < minDistance)) {
            bestElevator = id;
            minDistance = distance;
        }
    }

    return bestElevator;
}

// Sends a movement command to the chosen elevator
void Scheduler::sendCommandToElevator(int elevatorID, const Request& req) {
    string message = "MOVE " + to_string(elevatorID) + " " + to_string(req.floor);
    sendto(sockfd, message.c_str(), message.length(), MSG_CONFIRM,
           (const struct sockaddr*)&elevatorAddr, sizeof(elevatorAddr));

    cout << "[Scheduler] Assigned Elevator " << elevatorID << " to Floor " << req.floor << "\n";
}

// Listens for requests from the FloorSubsystem
void Scheduler::receiveRequests() {
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    char buffer[1024];

    while (true) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer), MSG_WAITALL,
                         (struct sockaddr*)&clientAddr, &len);
        buffer[n] = '\0';

        int time, floor, targetFloor;
        string direction;
        sscanf(buffer, "%d %d %d %s", &time, &floor, &targetFloor, &direction[0]);

        addRequest(Request(time, floor, targetFloor, direction));
    }
}

// Sends an arrival signal to FloorSubsystem
void Scheduler::sendArrivalSignal(int floor) {
    string message = "ARRIVED " + to_string(floor);
    sendto(sockfd, message.c_str(), message.length(), MSG_CONFIRM,
           (const struct sockaddr*)&floorAddr, sizeof(floorAddr));

    cout << "[Scheduler] Elevator arrived at Floor " << floor << ". Sending signal to FloorSubsystem.\n";
}

// Runs the Scheduler in multiple threads
void Scheduler::run() {
    thread schedulerThread(&Scheduler::processRequests, this);
    thread requestReceiver(&Scheduler::receiveRequests, this);

    schedulerThread.join();
    requestReceiver.join();
}

// Main function
int main() {
    int numElevators = 3;
    Scheduler scheduler(numElevators);
    scheduler.run();
    return 0;
}
