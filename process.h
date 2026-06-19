#pragma once
#include <string>
#include <vector>
#include <stdio.h>
using namespace std;
enum State
{
    READY,
    RUNNING,
    FINISHED
};
class Process
{
public:

    string process_name;
    string start_time;
    State state;
    int current_instruction;
    int total_instructions;
    vector<string> print_commands;
    Process(string name);
    int core_id = -1;
};