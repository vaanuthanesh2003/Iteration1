#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

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
        iss >> time >> floor >> direction;

        // Validate the data before using it
        if (iss.fail()) {
            std::cerr << "Error reading line: " << line << std::endl;
            continue;
        }

        // Simulate button press determining the destination floor
        int destination = (direction == "UP") ? floor + 1 : floor - 1;

        // Create the request
        Request req(time, floor, direction);

        // Add the request to the shared request queue
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            requestQueue.push(req);
            std::cout << "[Floor] Request from floor " << floor << " (" << direction << ") added to queue.\n";
        }

        // Notify the Elevator thread that a new request is in the queue
        cond.notify_one();

        // Simulate a small delay between requests (if needed)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    inputFile.close();  // Close the file when done
}
