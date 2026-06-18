#include "Process.h"
#include <stdio.h>
#include <string>
using namespace std;
Process::Process(string name)
{
    process_name = name;
    state = READY;
    current_instruction = 0;
    total_instructions = 100;
    for (int i =0; i < 100; i++)
    {
        print_commands.push_back(
            "Print Command"
        );
    }
}
