#include "manager.hpp"
#include <vector>
#include <iostream>
#include <sstream>
#include <array>
#include <fstream>

std::array<PCB, NUM_PROCESSES> PCBs;
std::array<RCB, NUM_RESOURCES> RCBs;
std::list<int> readyList;
int running = -1;

void create(int priority) {
    //input handling
    if ( priority < 0 || priority > NUM_PRIORITIES ) {
        std::cout << "error\n";
        return;
    }
    
    //allocate new pcb[j]
    int j = -1;
    for ( int i = 0; i < NUM_PROCESSES; ++i ) {
        if ( PCBs[i].state == ProcessState::FREE ) {
            j = i;
            break;
        }
    }

    //if no spots open (ie all 16 processes filled) break
    if ( j == -1 ) { 
        std::cout << "error\n";
        return;
    }

    //state = ready
    PCBs[j].state = ProcessState::READY;
    PCBs[j].priority = priority;

    //insert j innto list of kids i
    if ( running != -1 ) { 
        PCBs[running].children.push_back(j);
    }

    //parent = i, clean slate 
    PCBs[j].parent = running;
    PCBs[j].children.clear();
    PCBs[j].resources.clear();

    //j is ready, add to list
    auto curr = readyList.begin();
    while (curr != readyList.end() && PCBs[*curr].priority >= PCBs[j].priority) {
        ++curr;
    }
    readyList.insert(curr, j);

    scheduler();
}

void destroy_recursive(int pid) {
    if (pid == running) {
        running = -1;
    }
    
    //make copy to not impact actual processes yet - segfault otherwise
    std::list<int> childrenCopy = PCBs[pid].children;
    for (int child : childrenCopy) {
        destroy_recursive(child);
    }    
    PCBs[pid].children.clear(); //now clear original kids

    if (PCBs[pid].state == ProcessState::RUNNING || PCBs[pid].state == ProcessState::READY) {
        readyList.remove(pid);
    } else if (PCBs[pid].state == ProcessState::BLOCKED) {
        for (auto& rcb : RCBs) {
            for (auto curr = rcb.waitlist.begin(); curr != rcb.waitlist.end(); ) {
                if (curr->first == pid) {
                    curr = rcb.waitlist.erase(curr);
                } else {
                    ++curr;
                }
            }
        }
    }
    
    //release all resources of j
    for (auto [rid, units] : PCBs[pid].resources) {
        RCBs[rid].available += units;

        auto& wl = RCBs[rid].waitlist; 
        auto start = wl.begin();
        while (start != wl.end() && RCBs[rid].available > 0) {
            auto [wpid, wunits] = *start;
            if (wunits <= RCBs[rid].available) {
                RCBs[rid].available -= wunits;
                PCBs[wpid].resources.push_back({rid, wunits}); //give resources to waitlist processes
                PCBs[wpid].state = ProcessState::READY;

                auto curr = readyList.begin();
                while (curr != readyList.end() && PCBs[*curr].priority >= PCBs[wpid].priority) {
                    ++curr;
                }
                readyList.insert(curr, wpid);
                start = wl.erase(start);
            } else {
                ++start;
            }
        }
    }
    PCBs[pid].resources.clear();

    //free PCB[j]
    int parent = PCBs[pid].parent;
    if (parent != -1) {
        PCBs[parent].children.remove(pid);
    }

    PCBs[pid].state = ProcessState::FREE;
    PCBs[pid].parent = -1;
}

void destroy(int pid) {
    //input handling
    if (pid < 0 || pid >= NUM_PROCESSES || PCBs[pid].state == ProcessState::FREE) {
        std::cout << "error\n";
        return;
    }

    int ancestor = pid;
    while (ancestor != -1) {
        if (ancestor == running) { break; }
        ancestor = PCBs[ancestor].parent;
    }
    //if process is not running, can't delete non-running process
    if (ancestor == -1) {
        std::cout << "error\n";
        return;
    }

    destroy_recursive(pid); //call recursive destory
    scheduler();
}

void request(int rid, int units) {
    //input handling
    if (rid < 0 || rid >= NUM_RESOURCES || units <= 0 || units > RCBs[rid].inventory || running == -1) {
        std::cout << "error\n";
        return;
    }

    if (RCBs[rid].available >= units) { //if enough resources available to fulfill request
        RCBs[rid].available -= units;
        PCBs[running].resources.push_back({rid, units});
    } else {
        PCBs[running].state = ProcessState::BLOCKED;
        readyList.remove(running);
        RCBs[rid].waitlist.push_back({running, units});
        running = -1;
    }
    scheduler();
}

void release(int rid, int units) {
    //input handling
    if (rid < 0 || rid >= NUM_RESOURCES || units <= 0 || units > RCBs[rid].inventory || running == -1) {
        std::cout << "error\n";
        return;
    }

    int held = 0;
    for (auto& [rrid, runits] : PCBs[running].resources) {
        if (rid == rrid) {
            held += runits;
        }
    }
    if (held < units) { //holding less units than release-able
        std::cout << "-1 ";
        return;
    }

    int toRelease = 0;
    auto& resources = PCBs[running].resources;
    for (auto curr = resources.begin(); curr != resources.end() && units > 0; ) {
        if (curr->first == rid) {
            if (curr->second > units) { //running's held resources > toRelease resources
                curr->second -= units; //release resources
                toRelease += units; //increment toRelease
                units = 0; //desired units = 0 (fulfilled), loop will not continue because 0 !> 0
                ++curr;
            } else {
                toRelease += curr->second;
                units -= curr->second;
                curr = resources.erase(curr); //can't increment curr when deleting an entry, will already shift elements
            }
        } else {
            ++curr;
        }
    }

    RCBs[rid].available += toRelease; //released units back to list of available

    auto& waitlist = RCBs[rid].waitlist; //give highest priority process on waitlist the resources
    for (auto curr = waitlist.begin(); curr != waitlist.end() && RCBs[rid].available > 0; ) {
        auto [wpid, wunits] = *curr;
        if (wunits <= RCBs[rid].available) { //if we have enough units to fulfill request
            RCBs[rid].available -= wunits; //give units to wpid
            PCBs[wpid].resources.push_back({rid, wunits});
            PCBs[wpid].state = ProcessState::READY;

            auto pos = readyList.begin();
            while ( pos != readyList.end() && PCBs[*pos].priority >= PCBs[wpid].priority ) {
                ++pos; //insert process into readyList in order of priority
            }
            readyList.insert(pos, wpid);
            curr = waitlist.erase(curr);
        } else {
            ++curr;
        }
    }
    scheduler();
}

void timeout() {
    //if no process running, can't timeout
    if (running == -1) {
        std::cout << "error\n";
        return;
    }

    readyList.remove(running);
    PCBs[running].state = ProcessState::READY;

    auto curr = readyList.begin(); //grab next item from readyList
    while (curr != readyList.end() && PCBs[*curr].priority >= PCBs[running].priority) {
        ++curr;
    }
    readyList.insert(curr, running);

    running = -1;
    scheduler();
}

void scheduler() {
    //if readyList empty, no more processes to run
    if ( readyList.empty() ) { 
        std::cout << "-1 ";
        return;
    }

    int candidateProcess = readyList.front();
    if (candidateProcess != running) {
        if (running != -1 && PCBs[running].state == ProcessState::RUNNING) {
            PCBs[running].state = ProcessState::READY;
        }
        running = candidateProcess;
    }
    PCBs[running].state = ProcessState::RUNNING;

    std::cout << running << " ";
}

void init() {
    if (running != -1 || !readyList.empty()) {
        std::cout << "\n";
    }

    for ( auto& pcb : PCBs ) {
        pcb.state = ProcessState::FREE;
        pcb.parent = -1;
        pcb.priority = 0;
        pcb.children.clear();
        pcb.resources.clear();
    }

    readyList.clear();
    running = -1;

    for ( int i = 0; i < NUM_RESOURCES; ++i ) {
        RCBs[i].inventory = REOURCE_ALLOCATOR[i];
        RCBs[i].available = REOURCE_ALLOCATOR[i];
        RCBs[i].waitlist.clear();
    }

    PCBs[0].state = ProcessState::RUNNING;
    PCBs[0].priority = 0;
    PCBs[0].parent = -1;
    PCBs[0].children.clear();
    PCBs[0].resources.clear();

    readyList.push_back(0);
    scheduler();
}

void run_shell(std::istream& in) {
    std::string input;
    while (std::getline(in, input)) {
        if (!input.empty() && input.back() == '\r') {
            input.pop_back();
        }

        if (input.empty()) {
            init(); 
            continue;
        }

        std::istringstream ss(input);
        std::string cmd;
        ss >> cmd;

        try {
            if (cmd == "in") {
                init();
            } else if (cmd == "cr") {
                int priority;
                ss >> priority;
                create(priority);
            } else if (cmd == "de") {
                int pid;
                ss >> pid;
                destroy(pid);
            } else if (cmd == "rq") {
                int rid, units;
                ss >> rid >> units;
                request(rid, units);
            } else if (cmd == "rl") {
                int rid, units;
                ss >> rid >> units;
                release(rid, units); 
            } else if (cmd == "to") {
                timeout();
            } else {
                std::cout << "error\n";
            }
        } catch (...) {
            std::cout << "error\n";
        }
    }
}

int main(int arg, char* argv[]) {
    if (arg == 2) {
        std::ifstream file(argv[1]);
        if (!file) {
            std::cerr << "Cannot open file: " << argv[1] << '\n';
            return 1;
        }
        run_shell(file);
    } else {
        run_shell(std::cin);
    }
    return 0;
}