#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

class Process {
public:
    int id;
    bool isForeground;
    string command;
    bool promoted = false;
    int remainingTime = 0; // For wait queue simulation

    Process(int id, bool isForeground, const string& command)
        : id(id), isForeground(isForeground), command(command), promoted(false) {}

    string toString() const {
        stringstream ss;
        ss << "[" << id << (isForeground ? "F" : "B");
        if (promoted) ss << "*";
        ss << "]";
        return ss.str();
    }
};

class DynamicQueue {
private:
    mutex mtx;
    list<shared_ptr<Process>> processes;
    map<int, shared_ptr<Process>> waitQueue;

public:
    void enqueue(shared_ptr<Process> process) {
        lock_guard<mutex> lock(mtx);
        processes.push_back(process);
    }

    void simulateSleep(int pid, int seconds) {
        lock_guard<mutex> lock(mtx);
        auto it = find_if(processes.begin(), processes.end(), [&pid](const shared_ptr<Process>& p) {
            return p->id == pid;
            });
        if (it != processes.end()) {
            (*it)->remainingTime = seconds;
            waitQueue[pid] = *it;
            processes.erase(it);
        }
    }

    void wakeUpProcesses() {
        lock_guard<mutex> lock(mtx);
        for (auto it = waitQueue.begin(); it != waitQueue.end(); ) {
            if (--it->second->remainingTime <= 0) {
                enqueue(it->second);
                it = waitQueue.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void display() {
        lock_guard<mutex> lock(mtx);
        cout << "Running: ";
        if (!processes.empty()) {
            cout << processes.front()->toString() << "\n";
        }
        cout << "DQ: P => ";
        for (const auto& proc : processes) {
            cout << proc->toString() << " ";
        }
        cout << "(bottom/top)\n";
        cout << "---------------------------\n";
        cout << "WQ: ";
        for (const auto& wp : waitQueue) {
            cout << wp.second->toString() << " - remaining time: " << wp.second->remainingTime << "s; ";
        }
        cout << "\n---------------------------\n";
    }
};

void shellProcess(DynamicQueue& dq, int interval) {
    vector<string> commands = { "alarm clock", "todo list", "email check" };
    int processId = 2; // Starting from 2, since 0 and 1 are taken by shell and monitor
    for (const string& cmd : commands) {
        auto proc = make_shared<Process>(processId++, true, cmd);
        dq.enqueue(proc);
        this_thread::sleep_for(chrono::seconds(interval));
        dq.display();
    }
}

void monitorProcess(DynamicQueue& dq, int interval) {
    while (true) {
        this_thread::sleep_for(chrono::seconds(interval));
        dq.wakeUpProcesses();
        dq.display();
    }
}

int main() {
    DynamicQueue dq;
    auto shellProc = make_shared<Process>(0, true, "shell");
    auto monitorProc = make_shared<Process>(1, false, "monitor");

    dq.enqueue(shellProc);
    dq.enqueue(monitorProc);

    thread shellThread([&]() { shellProcess(dq, 5); });
    thread monitorThread([&]() { monitorProcess(dq, 10); });

    shellThread.join();
    monitorThread.detach();

    return 0;
}
