Resource Management and Process Scheduling

This C++ code repository demonstrates resource management algorithms like Banker's algorithm and process scheduling techniques such as Earliest Deadline First (EDF) and Least Laxity First (LLF). It simulates resource requests, usage, and releases in a multi-process environment.

Features:

- Banker's Algorithm: Ensures safe resource allocation by checking for deadlocks.
- EDF Scheduling: Executes processes based on their deadlines.
- LLF Scheduling: Prioritizes processes based on their laxity for efficient execution.
- Resource State Display: Provides insights into resource availability, allocation, and process needs.

Instructions:

1. Compilation: Compile the code using a C++ compiler.

2. Input Files: Provide input files containing resource and process information.

3. Execution: Run the compiled executable to observe resource management and scheduling outcomes.

Usage:

$ g++ main.cpp -o main
$ ./main

Sample Input Files:

- sample_words.txt: Contains information about resource types and instances.
- sample.txt: Contains instructions for processes, including resource requests, usage, and releases.
