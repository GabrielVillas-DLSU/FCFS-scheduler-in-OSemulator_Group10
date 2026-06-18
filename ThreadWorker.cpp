#include "ThreadWorker.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <ctime>
#include <thread>
#include <chrono>

ThreadWorker::ThreadWorker(int i, Process ProcesstoExecute) : id(i), ProcesstoExecute(ProcesstoExecute)  {}

//Executes the function
void ThreadWorker::run_async() {

       //edit this depending on what directory you want the written text files to be in
       const std::string fileName ="C:\\Users\\river\\Desktop\\C++\\CSOPESY\\FCFS2\\PROCESS_FOLDER\\process_"+ProcesstoExecute.process_name+".txt";
       ofstream writeFile(fileName);

       writeFile << "Process name: " << ProcesstoExecute.process_name << std::endl;
       writeFile << "Logs:" << std::endl << std::endl;

       for(int i = 0; i < ProcesstoExecute.total_instructions; i++){

          std::time_t timestamp = std::time(nullptr);
          std::tm datetime;
          localtime_s(&datetime, &timestamp);
       
          char timeString[50];
          std::strftime(timeString, sizeof(timeString), "%m/%d/%y %I:%M:%S%p", &datetime);
          
          writeFile <<"(" << timeString <<") "<< "Core:" << id << " \"" << "Hello World from "<< ProcesstoExecute.process_name<<"!" << "\"" << std::endl;
          this_thread::sleep_for(chrono::milliseconds(100));
       }

       writeFile.close();
}