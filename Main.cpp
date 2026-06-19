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
atomic<bool> cpuFree[ThreadWorker::N_THREADS];

vector<unique_ptr<Process>> processList;



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
            for (int i = 0; i < ThreadWorker::N_THREADS; ++i)
            {
                bool expectedToBeFree = true;
                
                // Check if CPU is free, set to false if so
                if (cpuFree[i].compare_exchange_strong(expectedToBeFree, false))
                {
                    nextProcess->state = RUNNING;
                    nextProcess->core_id = i;
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
    for (int i = 0; i < ThreadWorker::N_THREADS; i++) {
        cpuFree[i] = true;
    }


    for(int i = 1; i < 11; i++){
        processList.push_back(std::make_unique<Process>("screen_"+std::to_string(i)));
    }


    for(int i = 0; i < 10; i++)
    {
        lock_guard<mutex> lock(queueMutex);
        readyQueue.push(processList[i].get());
    }

    thread schedulerThread(fcfs_scheduler);

    schedulerCV.notify_one();

    // Keep the program alive long enough for the detached worker to print.
    string command;

 while(true)
{
    cout << "Enter a command: ";

    getline(cin, command);

    if(command == "screen -ls")
    {
        cout << endl;

        cout << "Running processes:" << endl;

        for(auto& process : processList)
        {
                cout
                    << "Name: "
                    << process->process_name

                    << " | Core: ";

                    if(process->core_id == -1)
                            {
                                cout << "N/A";
                            }
                            else
                            {
                                cout << process->core_id;
                            }
                    cout << " | "

                    << process->current_instruction

                    << " / "

                    << process->total_instructions

                    << endl;
        }

        cout << endl;
    }

    else if(command == "exit")
    {
        break;
    }
}

    // The other stuff goes here

    // Clean up before exiting
    isEmulatorRunning = false;
    schedulerCV.notify_all();
    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }
    
    return 0;
}