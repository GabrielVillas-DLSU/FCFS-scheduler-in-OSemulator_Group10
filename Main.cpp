#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include "Process.h"
#include "ThreadWorker.h"

using namespace std;

queue<Process*> readyQueue;
mutex queueMutex;
condition_variable schedulerCV;
atomic<bool> isEmulatorRunning(true);

// Track CPU cores (true = free, false = busy)
const int NUM_CORES = 4;
atomic<bool> cpuFree[NUM_CORES];

void fcfs_scheduler()
{
    while (isEmulatorRunning)
    {
        Process* nextProcess = nullptr;

        // Extract the next process from the queue (FCFS)
        {
            unique_lock<mutex> lock(queueMutex);
            
            // Wait until a process arrives in the queue OR the emulator shuts down
            schedulerCV.wait(lock, [] { return !readyQueue.empty() || !isEmulatorRunning; });

            if (!isEmulatorRunning && readyQueue.empty()) {
                break;
            }

            nextProcess = readyQueue.front();
            readyQueue.pop();
        }

        // Wait for a CPU core to become free, then assign the process
        bool assigned = false;
        while (!assigned && isEmulatorRunning)
        {
            for (int i = 0; i < NUM_CORES; ++i)
            {
                bool expectedToBeFree = true;
                
                // Check if CPU is free, set to false if so
                if (cpuFree[i].compare_exchange_strong(expectedToBeFree, false))
                {
                    nextProcess->state = RUNNING;
                    
                    // Assign the process to the ThreadWorker and run it
                    thread([i, nextProcess]() {
                        ThreadWorker worker(i, *nextProcess); 
                        worker.run_async();
                        
                        // Free up the CPU core for the next process
                        nextProcess->state = FINISHED;
                        cpuFree[i] = true;
                    }).detach();
                    
                    assigned = true;
                    break; // Successfully assigned, break out of the for-loop
                }
            }
            
            // Precent CPU resource hogging if all the CPUs are busy
            if (!assigned) {
                this_thread::sleep_for(chrono::milliseconds(10));
            }
        }
    }
}

int main()
{
    // Initialize CPU cores as free
    for (int i = 0; i < NUM_CORES; i++) {
        cpuFree[i] = true;
    }

    thread schedulerThread(fcfs_scheduler);

    // The other stuff goes here

    // Clean up before exiting
    isEmulatorRunning = false;
    schedulerCV.notify_all();
    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }
    
    return 0;
}