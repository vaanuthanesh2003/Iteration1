#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SCHEDULER_IP "127.0.0.1"
#define SCHEDULER_PORT 5002  // Matches the scheduler's port
#define BASE_PORT 5100 // Elevators bind from 5101, 5102, etc.
#define BUFFER_SIZE 1024
#define DOOR_WAIT_TIME 2  // Simulated door open/close time

class Elevator {
private:
    int currentFloor;
    int id;
    int sockfd;
    struct sockaddr_in schedulerAddr, selfAddr;

public:
    Elevator(int elevator_id) {
        id = elevator_id;
        currentFloor = 0;

        // Setup UDP socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("[Elevator] Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Bind to unique port
        memset(&selfAddr, 0, sizeof(selfAddr));
        selfAddr.sin_family = AF_INET;
        selfAddr.sin_addr.s_addr = INADDR_ANY;
        selfAddr.sin_port = htons(BASE_PORT + id);
       
        if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
            perror("[Elevator] Bind failed");
            exit(EXIT_FAILURE);
        }

        // Setup Scheduler Address
        memset(&schedulerAddr, 0, sizeof(schedulerAddr));
        schedulerAddr.sin_family = AF_INET;
        schedulerAddr.sin_port = htons(SCHEDULER_PORT);
       
        if (inet_pton(AF_INET, SCHEDULER_IP, &schedulerAddr.sin_addr) <= 0) {
            perror("[Elevator] Invalid address");
            exit(EXIT_FAILURE);
        }

        std::cout << "[Elevator " << id << "] Initialized on port " << BASE_PORT + id << std::endl << std::flush;
    }

    void sendStatus() {
    std::string message = "STATUS " + std::to_string(id) + " " + std::to_string(currentFloor);
    sendto(sockfd, message.c_str(), message.size(), 0,
           (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));

    std::cout << "[DEBUG] [Elevator " << id << "] Sent status update: " << message << std::endl << std::flush;
}

    void receiveCommand() {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in srcAddr;
        socklen_t addrLen = sizeof(srcAddr);

        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&srcAddr, &addrLen);
        if (n < 0) {
            perror("[Elevator] recvfrom() failed");
            return;
        }

        buffer[n] = '\0';

        std::cout << "[DEBUG] [Elevator " << id << "] Raw message received: " << buffer << std::endl << std::flush;

        int receivedID, targetFloor;
        if (sscanf(buffer, "MOVE %d %d", &receivedID, &targetFloor) == 2 && receivedID == id) {
            std::cout << "[Elevator " << id << "] Received move command  to Floor " << targetFloor << std::endl << std::flush;
            moveTo(targetFloor);
        } else {
            std::cerr << "[Elevator " << id << "] Error: Invalid command format or ID mismatch -> " << buffer << std::endl << std::flush;
        }
    }

    void moveTo(int targetFloor) {
        std::cout << "[Elevator " << id << "] Arrived at Floor " << currentFloor << " - Doors Opening" << std::endl << std::flush;
        sleep(DOOR_WAIT_TIME);
        std::cout << "[Elevator " << id << "] Doors Closing" << std::endl << std::flush;

        std::cout << "[Elevator " << id << "] Moving from " << currentFloor << " to " << targetFloor << std::endl << std::flush;
        sleep(2);

        currentFloor = targetFloor;
        sendStatus(); // Notify scheduler of new floor
    }

    void run() {
        while (true) {
            sendStatus();
            receiveCommand();
            sleep(2);
        }
    }

    ~Elevator() {
        close(sockfd);
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./elevator <elevator_id>" << std::endl << std::flush;
        return 1;
    }

    int elevatorID = atoi(argv[1]);
    Elevator elevator(elevatorID);
    elevator.run();

    return 0;
}
