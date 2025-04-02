
# Elevator Control System  Iteration 5 - README


## 1. Description

This iteration focuses on adding a display console showing where each of the elevators is in real time and
displaying any faults (if any) as well as implementing capacity limits (if not done so previously) so that people waiting to board an
elevator will have to wait for one which is empty.

This project iteration consists of three cpp files, three header files, and one input file. 

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
- sudo apt update
- sudo apt install cmake

- sudo apt update
- sudo apt install libgtest-dev cmake g++ make

- cd /usr/src/gtest
- sudo cmake CMakeLists.txt
- sudo make
- sudo cp lib/*.a /usr/lib
- After this, return to the direcotry of the project and run the rest of the commands

## 4. Building the Project

### Compile Main System:
- g++ client.cpp -o client
- g++ scheduler.cpp -o scheduler -pthread
- g++ elevator.cpp -o elevator

### Compile Tests:
- g++ -std=c++17 -DTEST_BUILD -o client_test client_test.cpp client.cpp -lgtest -lpthread
- g++ -std=c++17 -DTEST_BUILD -o elevator_test elevator_test.cpp elevator.cpp -lgtest -lpthread
- g++ -std=c++17 -DTEST_BUILD -o scheduler_test scheduler_test.cpp scheduler.cpp -lgtest -lpthread

## 5. Running Tests

- ./client_test
- ./elevator_test
- ./scheduler_test

## 6. Running the System

Run each thread on a separate terminal in this order:
- ./scheduler
- ./elevator 1 &
- ./elevator 2 &
- ./elevator 3 &
- ./client

Example input file format (input.txt):
- 00::00::11 3 UP 2
- 00::00::14 5 DOWN 4
- 00::00::19 2 UP 4

Sample output (Depending on terminal):
=== Simulation Stats ===
Simulation Time: 30 seconds
Total Moves: 14
Requests Handled: 12

---------------------------------------------
| Elevator | Floor | Load | Status          |
---------------------------------------------
|      1   |     0  |  0  |                              OK |
|      2   |     0  |  0  |                              OK |
|      3   |     0  |  0  |                              OK |
---------------------------------------------

[Elevator 1] Received move command to Floor 2.\
[Elevator 1] Doors opening...\
[Elevator 1] Doors closing...\
[Elevator 1] Moving to Floor 5.\

[Client] Sent request: Floor 2 UP to Floor 5\


## 7. Input File Format

Each line contains:\
<time_in_seconds> <floor_number> <direction> <target_floor>

Example:\
00::00::11 1 UP 2   # At 11 seconds, floor 1 UP button pressed to go to floor 2\


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

- Folahanmi: Class Diagram, State Diagram, Comments in program files
- Hassan: Code for scheduler_test.cpp
- Tobi Ola: Comments in program files, Timing diagram, Sequence Diagram
- Vaanathy: Code files, update input.txt file
- Vaasu: Code files, Comments in program files



