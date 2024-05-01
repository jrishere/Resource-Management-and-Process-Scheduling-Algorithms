#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <queue>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <chrono>
#include <thread>

using namespace std;

struct ResourceType {
    std::string name;
    std::vector<std::string> instances;
};

struct Process {
    int pid;
    int deadline;
    int computationTime;
    std::vector<std::string> instructions;
};

std::vector<ResourceType> resources;
std::map<int, Process> processes;
sem_t resourceMutex; // Semaphore for resource access
std::vector<int> Available; // Available resources
std::vector<std::vector<int>> Max; // Maximum demand
std::vector<std::vector<int>> Allocation; // Current allocation
std::vector<std::vector<int>> Need; // Remaining needs

sem_t mutex; // Semaphore for synchronization

void parseResources(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (getline(file, line)) {
        std::istringstream iss(line);
        ResourceType resource;
        std::getline(iss, resource.name, ':');
        resource.name = resource.name.substr(resource.name.find_first_not_of(" R"), resource.name.npos);
        std::string instance;
        while (getline(iss, instance, ',')) {
            resource.instances.push_back(instance.substr(instance.find_first_not_of(" "), instance.npos));
        }
        resources.push_back(resource);
    }
}

void parseProcesses(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    Process currentProcess;
    int processId = 0;
    while (getline(file, line)) {
        if (line.find("process_") != std::string::npos) {
            if (currentProcess.pid != 0) {
                processes[currentProcess.pid] = currentProcess;
                currentProcess.instructions.clear();
            }
            currentProcess.pid = ++processId;
        } else if (!line.empty()) {
            currentProcess.instructions.push_back(line);
        }
    }
    if (currentProcess.pid != 0) {
        processes[currentProcess.pid] = currentProcess;
    }
}

bool isSafe() {
    std::vector<bool> Finish(Max.size(), false);
    std::vector<int> Work = Available;

    while (true) {
        bool found = false;
        for (int i = 0; i < Max.size(); ++i) {
            if (!Finish[i]) {
                bool possible = true;
                for (int j = 0; j < Available.size(); ++j) {
                    if (Need[i][j] > Work[j]) {
                        possible = false;
                        break;
                    }
                }
                if (possible) {
                    for (int k = 0; k < Available.size(); ++k)
                        Work[k] += Allocation[i][k];
                    Finish[i] = true;
                    found = true;
                }
            }
        }
        if (!found) break; // No allocation was possible in this loop iteration
    }

    // If all processes can finish, the system is in a safe state
    for (bool f : Finish) if (!f) return false;
    return true;
}

bool requestResources(int processId, const std::vector<int>& request) {
    sem_wait(&mutex); // Begin critical section

    // Check if request can be satisfied
    for (int i = 0; i < request.size(); ++i) {
        if (request[i] > Need[processId][i] || request[i] > Available[i]) {
            sem_post(&mutex); // End critical section
            return false; // Request cannot be satisfied immediately
        }
    }

    // Temporarily allocate requested resources
    for (int i = 0; i < request.size(); ++i) {
        Available[i] -= request[i];
        Allocation[processId][i] += request[i];
        Need[processId][i] -= request[i];
    }

    // Check for safe state
    if (!isSafe()) {
        // Rollback resources
        for (int i = 0; i < request.size(); ++i) {
            Available[i] += request[i];
            Allocation[processId][i] -= request[i];
            Need[processId][i] += request[i];
        }
        sem_post(&mutex); // End critical section
        return false; // Request cannot be granted due to unsafe state
    }

    sem_post(&mutex); // End critical section
    return true; // Request granted
}

void useResources(int processId, const std::vector<int>& use) {
    std::cout << "Using resources for Process " << processId << ": ";
    for (size_t i = 0; i < resources.size(); ++i) {
        const auto& res = resources[i];
        for (int j = 0; j < use[i]; ++j) {
            std::cout << res.instances[j] << " ";
        }
    }
    std::cout << std::endl;
}

void releaseResources(int processId, const std::vector<int>& release) {
    sem_wait(&mutex); // Begin critical section

    // Release resources
    for (int i = 0; i < release.size(); ++i) {
        Available[i] += release[i];
        Allocation[processId][i] -= release[i];
        Need[processId][i] += release[i];
    }

    sem_post(&mutex); // End critical section
}

void calculate(int processId, int computationTime) {
    sleep(computationTime); // Simulate computation time
}

void printState() {
    std::cout << "Current State:" << std::endl;

    // Print available resources
    std::cout << "Available Resources: ";
    for (size_t i = 0; i < resources.size(); ++i) {
        ResourceType& res = resources[i];
        std::cout << res.name << ": ";
        for (size_t j = 0; j < res.instances.size(); ++j) {
            if (j != 0) std::cout << ", ";
            std::cout << res.instances[j];
        }
        std::cout << " (" << Available[i] << ")";
        if (i != resources.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;

    // Print allocation matrix
    std::cout << "Allocation Matrix:" << std::endl;
    for (auto& row : Allocation) {
        for (auto elem : row) std::cout << elem << " ";
        std::cout << std::endl;
    }

    // Print need matrix
    std::cout << "Need Matrix:" << std::endl;
    for (size_t i = 0; i < processes.size(); ++i) {
        std::cout << "Process " << i + 1 << ": ";
        for (size_t j = 0; j < resources.size(); ++j) {
            std::cout << Need[i][j];
            if (j != resources.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

void executeProcess(const std::vector<std::string>& instructions, int processId) {
    int totalComputationTime = 0;
    std::vector<std::string> usedResources;

    for (const auto& instruction : instructions) {
        if (instruction.find("calculate") != std::string::npos) {
            int computationTime = std::stoi(instruction.substr(instruction.find("(") + 1));
            calculate(processId, computationTime);
            totalComputationTime += computationTime;
        } else if (instruction.find("request") != std::string::npos) {
            std::vector<int> request;
            std::istringstream iss(instruction.substr(instruction.find("(") + 1));
            std::string num;
            while (std::getline(iss, num, ',')) {
                request.push_back(std::stoi(num));
            }
            if (!requestResources(processId, request)) {
                std::cerr << "Request denied. Deadlock may occur." << std::endl;
                break; // Exit loop if request is denied
            }
            // Add requested resources to usedResources
            for (size_t i = 0; i < resources.size(); ++i) {
                if (request[i] > 0) {
                    usedResources.push_back(resources[i].name + " (" + std::to_string(request[i]) + ")");
                }
            }
        } else if (instruction.find("use_resources") != std::string::npos) {
            // Use resources specified in sample_words.txt
            std::vector<int> use(resources.size(), 0); // Initialize vector to use all instances of each resource
            for (size_t i = 0; i < resources.size(); ++i) {
                for (const auto& instance : resources[i].instances) {
                    // Check if resource instance is available and use it
                    if (Available[i] > 0 && std::find(instructions.begin(), instructions.end(), instance) != instructions.end()) {
                        use[i]++;
                        Available[i]--;
                        Allocation[processId][i]++;
                        Need[processId][i]--;
                    }
                }
            }
            useResources(processId, use); // Call useResources function
        } else if (instruction.find("release") != std::string::npos) {
            std::vector<int> release;
            std::istringstream iss(instruction.substr(instruction.find("(") + 1));
            std::string num;
            while (std::getline(iss, num, ',')) {
                release.push_back(std::stoi(num));
            }
            releaseResources(processId, release);
            // Add released resources to usedResources
            for (size_t i = 0; i < resources.size(); ++i) {
                if (release[i] > 0) {
                    usedResources.push_back(resources[i].name + " (released: " + std::to_string(release[i]) + ")");
                }
            }
            // Update the need matrix after releasing resources
            for (size_t i = 0; i < resources.size(); ++i) {
                Need[processId][i] += release[i];
            }
        } else if (instruction.find("print_resources_used") != std::string::npos) {
            // Print the list of resources used by the process
            std::cout << "Resources used by Process " << processId << ": ";
            for (const auto& resource : usedResources) {
                std::cout << resource << ", ";
            }
            std::cout << std::endl;
        } else if (instruction.find("end") != std::string::npos) {
            releaseResources(processId, Allocation[processId]);
            break;
        }
    }

    // Update Need matrix after processing all instructions
    for (size_t i = 0; i < resources.size(); ++i) {
        Need[processId][i] = Max[processId][i] - Allocation[processId][i];
    }

    printState(); // Print state after processing all instructions

    // Print the total computation time for the process
    std::cout << "Process " << processId << " completed in " << totalComputationTime << " units." << std::endl;
}



void initializeDataStructures() {
    Available.resize(resources.size());
    for (int i = 0; i < resources.size(); ++i) {
        Available[i] = resources[i].instances.size();
    }

    int numProcesses = processes.size();
    int numResources = resources.size();

    // Initialize Max matrix with the maximum resource needs of each process
    Max.resize(numProcesses, std::vector<int>(numResources));
    for (const auto& [pid, proc] : processes) {
        for (size_t i = 0; i < proc.instructions.size(); ++i) {
            if (proc.instructions[i].find("request") != std::string::npos) {
                std::istringstream iss(proc.instructions[i].substr(proc.instructions[i].find("(") + 1));
                std::string num;
                int j = 0;
                while (std::getline(iss, num, ',')) {
                    Max[pid - 1][j++] = std::stoi(num);
                }
            }
        }
    }

    // Initialize Allocation matrix (all zeros initially)
    Allocation.resize(numProcesses, std::vector<int>(numResources, 0));

    // Initialize Need matrix as Max - Allocation
    Need.resize(numProcesses, std::vector<int>(numResources));
    for (int i = 0; i < numProcesses; ++i) {
        for (int j = 0; j < numResources; ++j) {
            Need[i][j] = Max[i][j] - Allocation[i][j];
        }
    }
}


void runBankersAlgorithm() {
    sem_wait(&resourceMutex);

    initializeDataStructures();
    printState();

    for (const auto& procEntry : processes) {
        const auto& proc = procEntry.second;
        executeProcess(proc.instructions, proc.pid - 1);
        printState();
        // Update allocation and need matrices after each process execution
        for (size_t i = 0; i < processes.size(); ++i) {
            for (size_t j = 0; j < resources.size(); ++j) {
                Need[i][j] = Max[i][j] - Allocation[i][j];
            }
        }
    }

    sem_post(&resourceMutex);
}

void runEDFScheduling() {
    std::cout << "Running EDF Scheduling" << std::endl;
    std::vector<std::pair<int, int>> sortedProcesses;
    for (const auto& procEntry : processes) {
        sortedProcesses.emplace_back(procEntry.first, procEntry.second.deadline);
    }
    std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });

    for (const auto& [pid, deadline] : sortedProcesses) {
        const auto& proc = processes[pid];
        std::cout << "Executing Process " << pid << " with deadline " << deadline << std::endl;
        executeProcess(proc.instructions, pid - 1);
        printState();
    }
    std::cout << "EDF Scheduling Completed" << std::endl;
}

void runLLFScheduling() {
    int currentTime = 0;
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<>> pq; // Priority queue to store processes based on laxity

    // Initialize priority queue with processes and their laxities
    for (const auto& [pid, proc] : processes) {
        int laxity = proc.deadline - currentTime - proc.computationTime;
        pq.push({laxity, pid});
    }

    // Execute processes based on LLF scheduling
    while (!pq.empty()) {
        int pid = pq.top().second;
        const auto& proc = processes[pid];
        pq.pop();

        executeProcess(proc.instructions, pid - 1);
        printState();

        // Update current time
        currentTime += proc.computationTime;

        // Update priority queue with remaining processes and their updated laxities
        std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<>> updatedPQ;
        while (!pq.empty()) {
            int nextPid = pq.top().second;
            pq.pop();
            const auto& nextProc = processes[nextPid];
            int nextLaxity = nextProc.deadline - currentTime - nextProc.computationTime;
            updatedPQ.push({nextLaxity, nextPid});
        }
        pq = updatedPQ;
    }
}

int main() {
    if (sem_init(&resourceMutex, 0, 1) != 0) {
        std::cerr << "Failed to initialize resourceMutex semaphore." << std::endl;
        return 1;
    }

    if (sem_init(&mutex, 0, 1) != 0) {
        std::cerr << "Failed to initialize mutex semaphore." << std::endl;
        sem_destroy(&resourceMutex);
        return 1;
    }

    parseResources("sample_words.txt");
    parseProcesses("sample.txt");

    runBankersAlgorithm();
    runEDFScheduling();
    runLLFScheduling();

    if (sem_destroy(&mutex) != 0) {
        std::cerr << "Failed to destroy mutex semaphore." << std::endl;
        sem_destroy(&resourceMutex);
        return 1;
    }

    if (sem_destroy(&resourceMutex) != 0) {
        std::cerr << "Failed to destroy resourceMutex semaphore." << std::endl;
        return 1;
    }

    return 0;
}