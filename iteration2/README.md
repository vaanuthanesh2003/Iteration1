
# Elevator Control System  Iteration 2- README


## 1. Description

This project simulates an elevator control system with multithreading the main goal of this project is to add the state machines for the scheduler and elevator subsystems. Key components:
- Request: Represents a user request (time, floor, direction).
- Scheduler: Manages incoming requests and dispatches them.
- FloorSubsystem: Reads requests from a file and adds them to the queue.
- Elevator: Processes requests from the queue and simulates movement.
- Scheduler function: processes requests


## 2. Dependencies

- C++ 
- Google Test (gtest) for testing
- CMake (optional, for build automation)
- pthread (for multithreading)


## 3. Installation

### Install GTest & CMake:
sudo apt update
sudo apt install cmake

sudo apt update
sudo apt install libgtest-dev cmake g++ make
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib


## 4. Building the Project

### Compile Main System:
g++ -std=c++11 elevator_system.cpp -o elevator_system -pthread

### Compile Tests:
g++ -std=c++14 test.cpp -o test -I/usr/include -L/usr/lib -lgtest -lgtest_main -pthread -no-pie


## 5. Running Tests

Execute the test binary:
./test


## 6. Running the System

Run the program with an input file:
./elevator_system input.txt

Example input file format (test_requests.txt):
1 3 UP
2 5 DOWN
3 2 UP

Sample output:
[Floor] Request from floor 3 (UP) added to queue.
[Scheduler]  Processing request for floor 3 
[Scheduler] Elevator 10 reached floor 3.


## 7. Input File Format

Each line contains:
<time_in_seconds> <floor_number> <direction>

Example:
1 3 UP    # At 1 second, floor 3 UP button pressed
2 5 DOWN  # At 2 seconds, floor 5 DOWN button pressed


## 8. Notes

- System uses condition variables for thread synchronization
- Elevator simulates 3-second movement between floors
- Tests include:
  * Request class validation
  * Elevator class validation
  * Scheduler request handling
  * File input processing
  * Full system integration
- Use Ctrl+C to terminate the running system
