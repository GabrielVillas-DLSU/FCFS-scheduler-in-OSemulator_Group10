#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
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
void getCurrTimeString(char* output, size_t outputSize) {
    std::time_t timestamp = std::time(nullptr);
    std::tm datetime;
    localtime_s(&datetime, &timestamp);
       
    std::strftime(output, outputSize, "%m/%d/%y %I:%M:%S%p", &datetime);
}

void printProcess(const Process* process)
{
    int nameWidth = 13;
    int timeWidth = 23;
    int coreWidth = 8;

    cout << left << setw(nameWidth) << process->process_name;

    if (process->core_id != -1)
    {
        cout << "( " << process->start_time << " )";
    }
    else
    {
        cout << setw(timeWidth) << "";
    }
        
    cout << "    Core: ";

    if (process->core_id == -1)
    {
        cout << left << setw(coreWidth) << "N/A";
    }
    else
    {
        cout << left << setw(coreWidth) << process->core_id;
    }

    cout << process->current_instruction
         << " / "
         << process->total_instructions
         << endl;
}

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

                        getCurrTimeString(nextProcess->start_time, sizeof(nextProcess->start_time));

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

        vector<Process*> runningProcesses;
        vector<Process*> finishedProcesses;

        for (auto& process : processList)
        {
            if (process->current_instruction == process->total_instructions)
            {
                finishedProcesses.push_back(process.get());
            }
            else
            {
                runningProcesses.push_back(process.get());
            }
        }

        cout << "Running processes:" << endl;

        for (const auto* process : runningProcesses)
        {
            printProcess(process);
        }

        cout << endl << "Finished processes:" << endl;

        for (const auto* process : finishedProcesses)
        {
            printProcess(process);
        }
        cout << endl;
    }

    else if(command == "exit")
    {
        break;
    }
}

    // Clean up before exiting
    isEmulatorRunning = false;
    schedulerCV.notify_all();
    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }
    
    return 0;
}