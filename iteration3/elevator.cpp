#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SCHEDULER_IP "127.0.0.1"  // IP address of the scheduler (localhost)
#define SCHEDULER_PORT 5002  // Matches the scheduler's port
#define BASE_PORT 5100  // Elevators bind from 5101, 5102, etc.
#define BUFFER_SIZE 1024  // Buffer size for receiving messages
#define DOOR_WAIT_TIME 2  // Simulated door open/close time (in seconds)

class Elevator {
private:
    int currentFloor;  // Current floor the elevator is on
    int id;  // Unique identifier for the elevator
    int sockfd;  // Socket descriptor for communication
    struct sockaddr_in schedulerAddr, selfAddr;  // Address structures for the scheduler and the elevator

public:
    // Constructor to initialize the elevator with a given ID
    Elevator(int elevator_id) {
        id = elevator_id;
        currentFloor = 0;  // Initialize the elevator on floor 0

        // Setup UDP socket for communication
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Create a UDP socket
        if (sockfd < 0) {
            perror("[Elevator] Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Bind the elevator to a unique port based on the elevator ID
        memset(&selfAddr, 0, sizeof(selfAddr));  // Clear the address structure
        selfAddr.sin_family = AF_INET;  // Set address family to IPv4
        selfAddr.sin_addr.s_addr = INADDR_ANY;  // Accept any incoming connections
        selfAddr.sin_port = htons(BASE_PORT + id);  // Set port to 5101, 5102, etc.

        if (bind(sockfd, (struct sockaddr*)&selfAddr, sizeof(selfAddr)) < 0) {
            perror("[Elevator] Bind failed");
            exit(EXIT_FAILURE);
        }

        // Setup Scheduler Address
        memset(&schedulerAddr, 0, sizeof(schedulerAddr));  // Clear the address structure
        schedulerAddr.sin_family = AF_INET;  // Set address family to IPv4
        schedulerAddr.sin_port = htons(SCHEDULER_PORT);  // Set the scheduler's port
        
        // Convert the scheduler IP address to binary format
        if (inet_pton(AF_INET, SCHEDULER_IP, &schedulerAddr.sin_addr) <= 0) {
            perror("[Elevator] Invalid address");
            exit(EXIT_FAILURE);
        }

        std::cout << "[Elevator " << id << "] Initialized on port " << BASE_PORT + id << std::endl << std::flush;
    }

    // Send the current status (floor) of the elevator to the scheduler
    void sendStatus() {
        std::string message = "STATUS " + std::to_string(id) + " " + std::to_string(currentFloor);
        sendto(sockfd, message.c_str(), message.size(), 0,
               (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));

        std::cout << "[DEBUG] [Elevator " << id << "] Sent status update: " << message << std::endl << std::flush;
    }

    // Receive commands from the scheduler (move command to a target floor)
    void receiveCommand() {
        char buffer[BUFFER_SIZE];  // Buffer to store received messages
        struct sockaddr_in srcAddr;  // Address of the sender
        socklen_t addrLen = sizeof(srcAddr);  // Length of the sender's address

        // Receive the message from the scheduler
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&srcAddr, &addrLen);
        if (n < 0) {
            perror("[Elevator] recvfrom() failed");
            return;
        }

        buffer[n] = '\0';  // Null-terminate the received message

        std::cout << "[DEBUG] [Elevator " << id << "] Raw message received: " << buffer << std::endl << std::flush;

        int receivedID, targetFloor;
        // Parse the "MOVE" command (e.g., "MOVE 1 3") and check if the ID matches
        if (sscanf(buffer, "MOVE %d %d", &receivedID, &targetFloor) == 2 && receivedID == id) {
            std::cout << "[Elevator " << id << "] Received move command to Floor " << targetFloor << std::endl << std::flush;
            moveTo(targetFloor);  // Move the elevator to the target floor
        } else {
            std::cerr << "[Elevator " << id << "] Error: Invalid command format or ID mismatch -> " << buffer << std::endl << std::flush;
        }
    }

    // Simulate the elevator moving to the target floor and opening/closing doors
    void moveTo(int targetFloor) {
        std::cout << "[Elevator " << id << "] Arrived at Floor " << currentFloor << " - Doors Opening" << std::endl << std::flush;
        sleep(DOOR_WAIT_TIME);  // Simulate door opening time
        std::cout << "[Elevator " << id << "] Doors Closing" << std::endl << std::flush;

        std::cout << "[Elevator " << id << "] Moving from " << currentFloor << " to " << targetFloor << std::endl << std::flush;
        sleep(2);  // Simulate the elevator moving to the target floor

        currentFloor = targetFloor;  // Update the current floor
        sendStatus();  // Notify the scheduler of the new floor
    }

    // Run the elevator continuously (send status and receive commands)
    void run() {
        while (true) {
            sendStatus();  // Periodically send status updates
            receiveCommand();  // Receive and process commands from the scheduler
            sleep(2);  // Wait before checking for commands again
        }
    }

    // Destructor to close the socket when the elevator is done
    ~Elevator() {
        close(sockfd);  // Close the socket to clean up
    }
};

int main(int argc, char* argv[]) {
    // Check if the elevator ID is provided as a command line argument
    if (argc != 2) {
        std::cerr << "Usage: ./elevator <elevator_id>" << std::endl << std::flush;
        return 1;
    }

    int elevatorID = atoi(argv[1]);  // Convert the command line argument to an integer
    Elevator elevator(elevatorID);  // Create an Elevator object with the provided ID
    elevator.run();  // Start the elevator's main loop

    return 0;
}
