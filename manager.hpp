
#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <array>
#include <list>
#include <vector>
#include <iostream>

//constant ints
static const int NUM_PROCESSES = 16;
static const int NUM_RESOURCES = 4;
static const int NUM_PRIORITIES = 3;
static const int REOURCE_ALLOCATOR[NUM_RESOURCES] = {1, 1, 2, 3};

enum class ProcessState { FREE, RUNNING, READY, BLOCKED };

struct PCB {
    ProcessState state = ProcessState::FREE;
    int parent = -1;
    std::list<int> children;
    std::list<std::pair<int, int>> resources; 
    //pair = what resource (index), how many of it
    int priority = 0;    
    int pid;
};

struct RCB {
    int available = 0;
    int rid;
    int inventory; //inital number of resources

    std::list<std::pair<int, int>> waitlist; //waitlist on resource, processes that want to use it
    //pair = what process, how many of units
};

//constant data strucrtures
extern std::array<PCB, NUM_PROCESSES> PCBs;
extern std::array<RCB, NUM_RESOURCES> RCBs;

//ready list - 3 tiered linked list of PCB indicies
extern std::list<int> readyList;
extern int running; //nothing currently running

void create(int priority);
void destroy_recursive(int pid);
void destroy(int pid);
void request(int rid, int units);
void release(int rid, int units);
void timeout();
void scheduler();
void init(); 

void run_shell(std::istream& in);

#endif
