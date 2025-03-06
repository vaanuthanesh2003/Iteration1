#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SCHEDULER_IP "127.0.0.1"
#define BASE_PORT 5000 // Each elevator will have a unique port: 5001, 5002, etc.

class Elevator {
private:
    int floor;
    int id;
    int passengers;
    int port;
    int sockfd;
    struct sockaddr_in scheduler_addr;

public:
    Elevator(int elevator_id) {
        id = elevator_id;
        floor = 0;
        passengers = 0;
        port = BASE_PORT + id; // Unique port for each elevator
        
        // Setup UDP socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Bind to unique port
        struct sockaddr_in self_addr;
        memset(&self_addr, 0, sizeof(self_addr));
        self_addr.sin_family = AF_INET;
        self_addr.sin_addr.s_addr = INADDR_ANY;
        self_addr.sin_port = htons(port);
        if (bind(sockfd, (struct sockaddr*)&self_addr, sizeof(self_addr)) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        // Setup scheduler address
        memset(&scheduler_addr, 0, sizeof(scheduler_addr));
        scheduler_addr.sin_family = AF_INET;
        scheduler_addr.sin_port = htons(6000); // Scheduler port
        scheduler_addr.sin_addr.s_addr = inet_addr(SCHEDULER_IP);
        
        std::cout << "Elevator " << id << " initialized on port " << port << std::endl;
    }

    void sendStatus() {
        char message[50];
        sprintf(message, "Elevator %d at floor %d with %d passengers", id, floor, passengers);
        sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&scheduler_addr, sizeof(scheduler_addr));
    }

    void receiveCommand() {
        char buffer[50];
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);

        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&src_addr, &addr_len);
        buffer[n] = '\0';

        if (strstr(buffer, "MOVE")) {
            int target_floor;
            sscanf(buffer, "MOVE %d", &target_floor);
            moveTo(target_floor);
        }
    }

    void moveTo(int target_floor) {
        std::cout << "Elevator " << id << " moving from " << floor << " to " << target_floor << std::endl;
        floor = target_floor;
        sendStatus();
    }

    void run() {
        while (true) {
            sendStatus();
            receiveCommand();
            sleep(2);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./elevator <elevator_id>\n";
        return 1;
    }

    int elevator_id = atoi(argv[1]);
    Elevator elevator(elevator_id);
    elevator.run();

    return 0;
}