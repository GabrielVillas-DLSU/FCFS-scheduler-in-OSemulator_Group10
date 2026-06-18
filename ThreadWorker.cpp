#include "ThreadWorker.h"
#include <chrono>
#include <iostream>


ThreadWorker::ThreadWorker(int i, Process ProcesstoExecute) : id(i), ProcesstoExecute(ProcesstoExecute)  {}

//Executes the function
void ThreadWorker::run_async() {
               
}