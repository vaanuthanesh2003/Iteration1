#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define SCHEDULER_IP "127.0.0.1"
#define SCHEDULER_PORT 5003
#define BASE_PORT 5000 // Elevator ports start from 5001, 5002, etc.
#define BUFFER_SIZE 1024

class ElevatorClient {
private:
    int sockfd;
    struct sockaddr_in schedulerAddr, selfAddr;
    char buffer[BUFFER_SIZE];
    int elevatorID;
    int currentFloor;

public:
    ElevatorClient(int id) : elevatorID(id), currentFloor(0) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Bind to a unique port for each elevator
        memset(&selfAddr, 0, sizeof(selfAddr));
        selfAddr.sin_family = AF_INET;
        selfAddr.sin_addr.s_addr = INADDR_ANY;
        selfAddr.sin_port = htons(BASE_PORT + elevatorID);

        if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        // Setup Scheduler Address
        memset(&schedulerAddr, 0, sizeof(schedulerAddr));
        schedulerAddr.sin_family = AF_INET;
        schedulerAddr.sin_port = htons(SCHEDULER_PORT);
        schedulerAddr.sin_addr.s_addr = inet_addr(SCHEDULER_IP);

        std::cout << "Elevator " << elevatorID << " initialized on port " << BASE_PORT + elevatorID << std::endl;
    }

    void sendStatus() {
        std::string message = "STATUS " + std::to_string(elevatorID) + " " + std::to_string(currentFloor);
        sendto(sockfd, message.c_str(), message.size(), 0, 
               (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));
    }

    void receiveCommand() {
        socklen_t len = sizeof(schedulerAddr);
        memset(buffer, 0, BUFFER_SIZE);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                         (struct sockaddr*)&schedulerAddr, &len);
        buffer[n] = '\0';

        int targetFloor, receivedID;
        if (sscanf(buffer, "MOVE %d %d", &receivedID, &targetFloor) == 2 && receivedID == elevatorID) {
            moveTo(targetFloor);
        }
    }

    void moveTo(int targetFloor) {
        std::cout << "Elevator " << elevatorID << " moving from " << currentFloor << " to " << targetFloor << std::endl;
        currentFloor = targetFloor;
        sendStatus();
    }

    void run() {
        while (true) {
            sendStatus();
            receiveCommand();
            sleep(2);
        }
    }

    ~ElevatorClient() {
        close(sockfd);
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./client <elevator_id>\n";
        return 1;
    }

    int elevatorID = atoi(argv[1]);
    ElevatorClient client(elevatorID);
    client.run();

    return 0;
}
