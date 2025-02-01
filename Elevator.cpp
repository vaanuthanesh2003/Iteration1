#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <conditional_variable>
#include <vector>

// initializing global variables

class Request {
private:
    int time;
    int floor;
    std::string button;

public:
    Request(int t, int f, std::string b) : time(t), floor(f), button(b) {}

    std::string get_button() { return button; }
    int get_floor() { return floor; }
    int get_time() { return time; }

    void set_button(std::string b) { button = b; }
    void set_floor(int f) { floor = f; }
    void set_time(int t) { time = t; }

    void Tostring() {
        std::cout << "Button: " << button << std::endl;
        std::cout << "Floor: " << floor << std::endl;
        std::cout << "Time: " << time << std::endl;
    }
};

std::queue<Request> requestQueue;
std:: mutex queueMutex;
std:: conditional_variable cond;

void Elevator(){
    
    while (true){
        // in this class, the "processing" of a request is what will simulate the car moving
        std::unique_lock<std::mutex> lock(queueMutex); // receive lock to access shared queue
        cond.wait(lock, [] { return !requestQueue.empty(); }); // wait for a request in the queue

        Request req = requestQueue.front(); // this allows us to get the first request from the queue (FIFO)
        requestQueue.pop(); // removes top request from queue as it is being handled
        lock.unlock(); // release lock to allow other threads to access it

        std::cout << "[Elevator] Moving to floor " << req.source << " to handle request.\n";
        std::this_thread::sleep_for(std::chrono::seconds(3)); // this will simulate "moving" to requested floor

        std::cout << "[Elevator] Request from floor " << req.source << " handled.\n" // output that request is handled
    }

}
