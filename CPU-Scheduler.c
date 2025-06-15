#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


typedef struct process {
    char name[50];
    char description[100];
    int arrivalTime;
    int burstTime;
    int priority;
    int pid;
} process;


typedef struct queue {
    process* processes[1000];
    int count;
} queue;


void processHandler(int signum)
{
    if (signum == SIGTERM)
    {
        exit(0);
    }
}

void ignoreSignal(int signum){
    signal(signum, ignoreSignal);
}

process* popQueue(queue* processQueue);
process* topQueue(queue *processQueue);
int pushQueue(queue* processQueue, process* newProcess);
int isProcessInQueue(queue* processQueue, process* targetProcess);
int updateQueue(queue* processQueue, process processes[], int processCount, int currentTime);
int checkIfAllProcessesCompleted(process processes[], int processCount);
void initializeAllProcesses(process processes[], int processCount);
void resetProcesses(process processes[], process originalProcesses[], int processCount);
void roundRobin(process processes[], int processCount, int timeQuantum);
void sortProcesses(int flag, process processes[], int processCount);
void printModeMessage(int flag);
void printSummary(int timeUnits, double averageWaitingTime, int flag);
void printLine(const char* line);
int AllButRoundRobin(process processes[], int processCount, int flag);
int getNextProcessIndex(process processes[], int processCount, int currentTime, int flag);

void runCPUScheduler(char* processesCsvFilePath, int timeQuantum)
{
    FILE *file = fopen(processesCsvFilePath, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    char line[256];
    int processCount = 0;
    process processes[1000];
    
    while (fgets(line, sizeof(line), file) && processCount < 1000)
    {
        line[strcspn(line, "\n")] = '\0';
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        // Skip comment lines (lines starting with #)
        if (line[0] == '#') {
            continue;
        }
        
        processes[processCount].pid = -1; // Initialize PID to -1
        sscanf(line, "%49[^,],%99[^,],%d,%d,%d", 
                    processes[processCount].name,
                    processes[processCount].description,
                    &processes[processCount].arrivalTime,
                    &processes[processCount].burstTime,
                    &processes[processCount].priority);
        
        processCount++;
    }
    fclose(file);
    if (processCount == 0) {
        printf("No processes found in the file.\n");
        return;
    }
    process originalProcesses[1000];
    for (int i = 0; i < processCount; i++)
    {
        originalProcesses[i] = processes[i];
    }
    for(int i = 0; i < 3; i++)
    {
        AllButRoundRobin(processes, processCount, i);
        resetProcesses(processes, originalProcesses, processCount);
    }
    resetProcesses(processes, originalProcesses, processCount);
    roundRobin(processes, processCount, timeQuantum);
}

int AllButRoundRobin(process processes[], int processCount, int flag)
{
    char buffer[256];
    int startTime = 0;
    int endTime = 0;
    int nextIndex = 0;
    int waitingTime = 0;
    initializeAllProcesses(processes, processCount);
    sortProcesses(flag, processes, processCount);
    printModeMessage(flag);
    signal(SIGALRM, ignoreSignal);
    for (int i = 0; i < processCount; i++)
    {
        nextIndex = getNextProcessIndex(processes, processCount, endTime, flag);
        if(nextIndex == -1)
        {
            while(nextIndex == -1)
            {
                endTime++;
                alarm(1);
                pause();
                nextIndex = getNextProcessIndex(processes, processCount, endTime, flag);
            }
            snprintf(buffer, sizeof(buffer), "%d → %d: Idle.\n", startTime, endTime);
            printLine(buffer);
            startTime = endTime;
        }
        waitingTime += startTime - processes[nextIndex].arrivalTime;
        endTime = startTime + processes[nextIndex].burstTime;
        kill(SIGCONT, processes[nextIndex].pid);
        alarm(processes[nextIndex].burstTime);
        pause();
        kill(processes[nextIndex].pid, SIGTERM);
        wait(NULL);
        snprintf(buffer, sizeof(buffer), "%d → %d: %s Running %s.\n", startTime, endTime, 
                processes[nextIndex].name, processes[nextIndex].description);
        printLine(buffer);
        startTime = endTime;
        processes[nextIndex].pid = -1; // Reset PID after completion
    }
    
    // Add summary print at the end
    double averageWaitingTime = (double)waitingTime / processCount;
    printSummary(0, averageWaitingTime, flag);
    
    return endTime;
}

void roundRobin(process processes[], int processCount, int timeQuantum)
{
    char buffer[256];
    int startTime = 0;
    int endTime = 0;
    int i = 0;
    initializeAllProcesses(processes, processCount);
    queue processQueue;
    processQueue.count = 0;
    updateQueue(&processQueue, processes, processCount, startTime);
    printModeMessage(3);
    process* nextProcess = NULL;
    process* currentProcess = topQueue(&processQueue);
    signal(SIGALRM, ignoreSignal);
    while(checkIfAllProcessesCompleted(processes, processCount))
    {
        for(i = 0; i < timeQuantum; i++)
        {
            endTime++;
            alarm(1);
            pause();
            if(currentProcess == NULL)
            {
                updateQueue(&processQueue, processes, processCount, endTime);
                nextProcess = topQueue(&processQueue);
                if(nextProcess != currentProcess)
                {
                    snprintf(buffer, sizeof(buffer), "%d → %d: Idle.\n", startTime, endTime);
                    printLine(buffer);
                    startTime = endTime;
                    kill(nextProcess->pid, SIGCONT);
                    break;
                }
            }
            else
            {
                currentProcess->burstTime--;
                if(currentProcess->burstTime == 0)
                {
                    snprintf(buffer, sizeof(buffer), "%d → %d: %s Running %s.\n", startTime, endTime, 
                            currentProcess->name, currentProcess->description);
                    printLine(buffer);
                    startTime = endTime;
                    kill(currentProcess->pid, SIGTERM);
                    wait(NULL);
                    currentProcess->pid = -1; // Reset PID after completion
                    nextProcess = topQueue(&processQueue);
                    popQueue(&processQueue);
                    break;
                }
            }
        }
        if(i == timeQuantum)
        {
            if(currentProcess != NULL)
            {
                snprintf(buffer, sizeof(buffer), "%d → %d: %s Running %s.\n", startTime, endTime, 
                        currentProcess->name, currentProcess->description);
                printLine(buffer);
                popQueue(&processQueue);
                updateQueue(&processQueue, processes, processCount, endTime - 1);
                updateQueue(&processQueue, processes, processCount, endTime);
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "%d → %d: Idle.\n", startTime, endTime);
                printLine(buffer);
            }
        }
        else if(currentProcess != NULL)
        {
            if(currentProcess->burstTime > 0)
            {
                pushQueue(&processQueue, currentProcess);
                kill(currentProcess->pid, SIGTSTP);
            }
        }
        updateQueue(&processQueue, processes, processCount, endTime);
        nextProcess = topQueue(&processQueue);
        if(nextProcess != NULL)
        {
            kill(nextProcess->pid, SIGCONT);
        }
        startTime = endTime;
        currentProcess = nextProcess;
    }
    
    printSummary(endTime, 0, 4);
}

int getNextProcessIndex(process processes[], int processCount, int currentTime, int flag)
{
    for (int i = 0; i < processCount; i++)
    {
        if (processes[i].arrivalTime <= currentTime && processes[i].pid != -1)
        {
            return i;
        }
    }
    return -1;
}

void sortProcesses(int flag, process processes[], int processCount)
{
    int i, j;
    int swapped;
    for (i = 0; i < processCount - 1; i++)
    {
        swapped = 0;
        for (j = 0; j < processCount - 1 - i; j++)
        {
            int shouldSwap = 0;
            if (flag == 0)
            {
                if (processes[j].arrivalTime > processes[j+1].arrivalTime)
                {
                    shouldSwap = 1;
                }
            }
            else if (flag == 1)
            {
                if (processes[j].burstTime > processes[j+1].burstTime)
                {
                    shouldSwap = 1;
                }
                else if (processes[j].burstTime == processes[j+1].burstTime)
                {
                    if (processes[j].arrivalTime > processes[j+1].arrivalTime)
                    {
                        shouldSwap = 1;
                    }
                }
            }
            else if (flag == 2)
            {
                if (processes[j].priority > processes[j+1].priority)
                {
                    shouldSwap = 1;
                }
                else if (processes[j].priority == processes[j+1].priority)
                {
                    if (processes[j].arrivalTime > processes[j+1].arrivalTime)
                    {
                        shouldSwap = 1;
                    }
                }
            }

            if (shouldSwap)
            {
                process temp = processes[j];
                processes[j] = processes[j+1];
                processes[j+1] = temp;
                swapped = 1;
            }
        }

        // If no two elements were swapped by inner loop, then break
        if (swapped == 0)
            break;
    }
}
void printModeMessage(int flag)
{
    printLine("══════════════════════════════════════════════\n");
    if (flag == 0)
    {
        printLine(">> Scheduler Mode : FCFS\n");
    }
    else if (flag == 1)
    {
        printLine(">> Scheduler Mode : SJF\n");
    }
    else if (flag == 2)
    {
        printLine(">> Scheduler Mode : Priority\n");
    }
    else if (flag == 3)
    {
        printLine(">> Scheduler Mode : Round Robin\n");
    }
    else
    {
        printLine(">> Scheduler Mode : Unknown\n");
    }
    printLine(">> Engine Status  : Initialized\n");
    printLine("──────────────────────────────────────────────\n\n");
}

void printLine(const char* line) {
    write(STDOUT_FILENO, line, strlen(line));
}

void printSummary(int timeUnits,double averageWaitingTime, int flag)
{
    char buffer[256];
    printLine("\n──────────────────────────────────────────────\n");
    printLine(">> Engine Status  : Completed\n");
    printLine(">> Summary        :\n");
    if(flag != 4)
    {
        snprintf(buffer, sizeof(buffer), "   └─ Average Waiting Time : %.2f time units\n", averageWaitingTime);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "   └─ Total Turnaround Time : %d time units\n\n", timeUnits);
    }
    printLine(buffer);
    printLine(">> End of Report\n");
    printLine("══════════════════════════════════════════════\n");
}

process* popQueue(queue* processQueue)
{
    if (processQueue->count <= 0) {
        return NULL;
    }
    process* topProcess = processQueue->processes[0];
    for (int i = 0; i < processQueue->count - 1; i++) {
        processQueue->processes[i] = processQueue->processes[i + 1];
    }
    
    processQueue->count--;
    return topProcess;
}

process* topQueue(queue *processQueue)
{
    if(processQueue->count <= 0) {
        return NULL;
    }
    return processQueue->processes[0];
}

int pushQueue(queue* processQueue, process* newProcess)
{
    if (processQueue->count >= 1000) {
        return -1;
    }
    
    processQueue->processes[processQueue->count] = newProcess;
    processQueue->count++;
    return 0;
}

int isProcessInQueue(queue* processQueue, process* targetProcess)
{
    for (int i = 0; i < processQueue->count; i++) {
        if (processQueue->processes[i] == targetProcess) {
            return 1;
        }
    }
    return 0;
}

int updateQueue(queue* processQueue, process processes[], int processCount, int currentTime)
{
    if (processCount <= 0 || processCount > 1000) {
        return -1;
    }
    
    for (int i = 0; i < processCount && i < 1000; i++) 
    {
        if(processes[i].arrivalTime <= currentTime && 
           processes[i].burstTime > 0 && 
           !isProcessInQueue(processQueue, &processes[i])) // Check for duplicates
        {
            pushQueue(processQueue, &processes[i]);
        }
    }
    return 0;
}

int checkIfAllProcessesCompleted(process processes[], int processCount)
{
    for (int i = 0; i < processCount; i++)
    {
        if (processes[i].burstTime > 0) // If any process still has burst time left
        {
            return 1; // Not all processes are completed
        }
    }
    return 0; // All processes are completed
}

void initializeAllProcesses(process processes[], int processCount)
{
    for(int i = 0; i < processCount; i++)
    {
        int pid = fork();
        if(pid < 0)
        {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        else if(pid == 0)
        {
            pause();
            while(1){}
        }
        else
        {
            processes[i].pid = pid; // Assign the PID to the process
        }
    }
}

void resetProcesses(process processes[], process originalProcesses[], int processCount)
{
    for(int i = 0; i < processCount; i++)
    {
        processes[i] = originalProcesses[i]; // Reset to original state
        processes[i].pid = -1; // Reset PID to -1
    }
}