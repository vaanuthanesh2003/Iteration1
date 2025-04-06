#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SCHEDULER_IP "127.0.0.1"  // IP address of the scheduler (localhost)
#define SCHEDULER_PORT 5002  // Matches the scheduler's port
#define BUFFER_SIZE 1024  // Size of the buffer used for receiving messages
#define INPUT_FILE "input.txt"  // File containing the elevator requests

// Function to send a request to the scheduler
void sendRequest(int floor, std::string direction, int targetFloor) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Create a UDP socket
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in schedulerAddr{};  // Initialize address structure for the scheduler
    memset(&schedulerAddr, 0, sizeof(schedulerAddr));  // Clear the structure
    schedulerAddr.sin_family = AF_INET;  // Set the address family to AF_INET (IPv4)
    schedulerAddr.sin_port = htons(SCHEDULER_PORT);  // Set the port to the scheduler's port
   
    // Convert the IP address string to binary format
    if (inet_pton(AF_INET, SCHEDULER_IP, &schedulerAddr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);  // Close the socket if there was an error with the address
        exit(EXIT_FAILURE);
    }

    // Prepare the message and send it to the scheduler
    std::string message = std::to_string(floor) + " " + direction + " " + std::to_string(targetFloor);
    sendto(sockfd, message.c_str(), message.size(), 0,
           (struct sockaddr*)&schedulerAddr, sizeof(schedulerAddr));  // Send the request

    std::cout << "[Client] Sent request: " << message << std::endl;  // Log the sent request
    close(sockfd);  // Close the socket after sending the request
}

// Function to process requests read from a file
void processRequestsFromFile() {
    std::ifstream inputFile(INPUT_FILE);  // Open the input file
    if (!inputFile) {
        std::cerr << "[Client] Error: Could not open input file: " << INPUT_FILE << std::endl;
        return;  // Exit if the file cannot be opened
    }

    std::string line;
    while (std::getline(inputFile, line)) {  // Read each line from the file
        std::istringstream iss(line);  // Create a string stream for parsing the line
        int time, floor, targetFloor;
        std::string direction;
       
        // Parse the line into the variables: time, floor, direction, and targetFloor
        if (!(iss >> time >> floor >> direction >> targetFloor)) {
            std::cerr << "[Client] Error: Invalid line format -> " << line << std::endl;
            continue;  // Skip the line if it is incorrectly formatted
        }

        sendRequest(floor, direction, targetFloor);  // Send the parsed request
        sleep(1);  // Small delay to simulate time between requests
    }

    inputFile.close();  // Close the input file after processing all lines
}

// Main function to continuously process requests from the file
int main() {
    while (true) {
        processRequestsFromFile();  // Process the requests from the input file
        sleep(5);  // Wait before reading the file again (optional)
    }
    return 0;
}
