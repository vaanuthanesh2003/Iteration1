
# Elevator Control System  Iteration 2- README


## 1. Description

This project simulates an **elevator scheduling system** where multiple elevators respond to floor requests sent from a client. The system ensures efficient elevator assignment using a **centralized scheduler** that dynamically assigns the closest available elevator to each request.

This project iteration consists of three cpp files and one input file. 

1. **Client (Floor Subsystem)** client.cpp
   - Reads requests from an `input.txt` file (or user input).
   - Sends floor requests (e.g., "Floor 2 UP to Floor 5") to the scheduler.

2. **Scheduler (Central Controller)** scheduler.cpp
   - Listens for incoming requests from clients.
   - Tracks the current position of all elevators.
   - Assigns the closest available elevator to handle each request.
   - Sends movement commands (`MOVE <elevator_id> <floor>`) to the selected elevator.

3. **Elevators (Elevator Subsystem)** elevator.cpp
   - Listens for commands from the scheduler.
   - Moves to the assigned floor and updates its position.
   - Simulates doors opening and closing before moving.


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
g++ client.cpp -o client
g++ scheduler.cpp -o scheduler -pthread
g++ elevator.cpp -o elevator

### Compile Tests:
-- -- 


## 5. Running Tests

-- -- 


## 6. Running the System

Run each thread on a separate terminal in this order:
./scheduler
./elevator 1 &
./elevator 2 &
./elevator 3 &
./client

Example input file format (input.txt):
1 3 UP 2
2 5 DOWN 4
3 2 UP 4

Sample output (Depending on terminal):
[Scheduler] Received request: Floor 2 UP to Floor 5
[Scheduler] Assigning Elevator 1 to handle request.
[Scheduler] Elevator 1 moving to Floor 2.

[Elevator 1] Received move command to Floor 2.
[Elevator 1] Doors opening...
[Elevator 1] Doors closing...
[Elevator 1] Moving to Floor 5.

[Client] Sent request: Floor 2 UP to Floor 5


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

## Breakdown of Responsibilities:

- Folahanmi: Class Diagram
- Hassan: Code for client.cpp
- Tobi Ola: Unit Testing
- Vaanathy: Code for scheduler.cpp
- Vaasu: Code for elevator.cpp

