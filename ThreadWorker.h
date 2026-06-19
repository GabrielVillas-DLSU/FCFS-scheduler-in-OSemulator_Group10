#pragma once

#include <thread>
#include "Process.h"


class ThreadWorker {
public:
    inline static const int N_THREADS = 4;              // Declared here so it can be used below

private:
    inline static const int DELAY = 100;                // Sleep duration
    int id;
    Process& ProcesstoExecute;

public:
    ThreadWorker(int i, Process& ProcesstoExecute);
    void run_async();
};
