#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

#include <sys/wait.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
    char name[50];
    char desc[100];
    int arrival_time;
    int burst_time;
    int priority;
    pid_t pid; 
    int remaining_time; 
}Process;

typedef struct {
    int index;
    int arrival_order;
} QueueItem;


typedef struct {
    QueueItem items[1000];
    int front, rear, count;
}Queue;

void enqueue(Queue *q, int index) {
    q->items[q->rear++] = (QueueItem){index};
}

int dequeue(Queue *q) {
    return q->items[q->front++].index;
}

int is_empty(Queue *q) {
    return q->front == q->rear;
}

void ignoreSignal(int signum){
    signal(signum, ignoreSignal);
}


void print_schedule_header(const char *algorithm){
    printf("══════════════════════════════════════════════\n");
    printf(">> Scheduler Mode : %s\n", algorithm);
    printf(">> Engine Status  : Initialized\n");
    printf("──────────────────────────────────────────────\n\n");
}


void print_schedule_entry(int start, int end, const Process *p){
    if (p) {
        printf("%d → %d: %s Running %s.\n", start, end, p->name, p->desc);
    } else {
        printf("%d → %d: Idle.\n", start, end);
    }
}
void print_summary(double avg_waiting_time){
    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Average Waiting Time : %.2f time units\n", avg_waiting_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════\n");
}
void print_turnaround_summary(double total_time) {
    printf("\n──────────────────────────────────────────────\n");
    printf(">> Engine Status  : Completed\n");
    printf(">> Summary        :\n");
    printf("   └─ Total Turnaround Time : %.0f time units\n\n", total_time);
    printf(">> End of Report\n");
    printf("══════════════════════════════════════════════");
}



// Reads the CSV file and convert it into an array of Process structs.
int load_processes(const char *filename, Process processes[]){
    // Try to open the file for reading if can't prints error.
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen"); 
        return -1;
    }
    
    char line[256];
    int count = 0; 

    // Loop to read each line of file.
    while (fgets(line, sizeof(line), file)) {

         if (strlen(line) < 3) continue; //If line too short (like Nadav) ignores.

         // Process name
        char *token = strtok(line, ",");
        if (!token) continue;  // If token is missing, skip the line
        strncpy(processes[count].name, token, sizeof(processes[count].name));

        // Description
        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(processes[count].desc, token, sizeof(processes[count].desc));

        // Arrival time
        token = strtok(NULL, ",");
        if (!token) continue;
        processes[count].arrival_time = atoi(token); // Needs to be int.

        // Burst time
        token = strtok(NULL, ",");
        if (!token) continue;
        processes[count].burst_time = atoi(token);
        processes[count].remaining_time = processes[count].burst_time;

        // Priority value
        token = strtok(NULL, ",");
        if (!token) continue;
        processes[count].priority = atoi(token);

         count++;
    }
    
    fclose(file);

    return count; // Returns number of Processes.
}

void simulate_run(Process *p, int duration){
    pid_t pid = fork();

    if (pid == 0) {
        signal(SIGCONT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        pause();  // Start paused
        // (1) {}  // Infinite loop simulating "running"
    }
    else if (pid > 0) {
        p->pid = pid;
        kill(pid, SIGSTOP);  // Immediately stop the child until scheduled
    } else {
        perror("fork");
    }
}


// For sorting the Processes.
int cmp_arrival(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    return p1->arrival_time - p2->arrival_time;
}

// FCFS scheduling simulation.
void schedule_fcfs(Process processes[], int count){

    print_schedule_header("FCFS");

    // Sort the processes by arrival time.
    qsort(processes, count, sizeof(Process), cmp_arrival);

    int current_time = 0;
    double total_waiting_time = 0;

    for (int i = 0; i < count; i++) {
        Process *p = &processes[i];
        
        // If the current time is before the process arival we insert an idle period.
        if (current_time < p->arrival_time) {
            print_schedule_entry(current_time, p->arrival_time, NULL);  
            current_time = p->arrival_time;
        }

        // Start and end time of the process.
        int start_time = current_time;
        int end_time = start_time + p->burst_time;

        // Calclating the waiting time.
        int waiting_time = start_time - p->arrival_time;
        total_waiting_time += waiting_time;

        print_schedule_entry(start_time, end_time, p);

        simulate_run(p, p->burst_time);

        // Update the current time.
        current_time = end_time;
    }

    // Calculate and print the avg waiting time.
    double avg_waiting_time = total_waiting_time / count;
    print_summary(avg_waiting_time);
    printf("\n");
}

// SJF scheduling simulation.
void schedule_sjf(Process processes[], int count){

    print_schedule_header("SJF");

    int current_time = 0;
    double total_waiting_time = 0;
    int completed = 0;
    int done[1000] = {0};

    // Loop over all the processes.
    while (completed < count) {

        int shortest_index = -1;

        // Find the shortest job that has already arrived and is not done.
        for (int i = 0; i < count; i++) {
            if (!done[i] && processes[i].arrival_time <= current_time) {
                if (shortest_index == -1 || 
                    processes[i].burst_time < processes[shortest_index].burst_time || 
                    (processes[i].burst_time == processes[shortest_index].burst_time &&
                    processes[i].arrival_time < processes[shortest_index].arrival_time)) {
                    shortest_index = i;
}
            }
        }

        // If no process had arrived yet.
        if (shortest_index == -1) {
            int next_arrival = INT_MAX;
            // Looking for the process that is the most soon to arrive and isn't done or arrived yet.
            for (int i = 0; i < count; i++) {
                if (!done[i] && processes[i].arrival_time > current_time) {
                    next_arrival = MIN(next_arrival, processes[i].arrival_time);
                }
            }
            print_schedule_entry(current_time, next_arrival, NULL);
            // Update the time for the next process.
            current_time = next_arrival;
        }
        else {
            // We got a process to run.
            Process *p = &processes[shortest_index];

            int start_time = current_time;
            int end_time = start_time + p->burst_time;

            int waiting_time = start_time - p->arrival_time;
            total_waiting_time += waiting_time;

            print_schedule_entry(start_time, end_time, p);
            simulate_run(p, p->burst_time);

            current_time = end_time;
            done[shortest_index] = 1; // Makr process as done.
            completed++;
        }
    }
    // Calculate and print avg waiting time.
    double avg_waiting_time = total_waiting_time / count;
    print_summary(avg_waiting_time);
    printf("\n");
}

// Priority scheduling simulation.
void schedule_priority(Process processes[], int count){

    print_schedule_header("Priority");

    int current_time = 0;                
    double total_waiting_time = 0;     
    int completed = 0;                  
    int done[1000] = {0};    

    // Loop over all the processes.
    while (completed < count) {
        int best_index = -1;

        // Find the highest priority process that has arrived and is not done.
        for (int i = 0; i < count; i++) {
            if (!done[i] && processes[i].arrival_time <= current_time) {
                if (best_index == -1 || processes[i].priority < processes[best_index].priority) {
                    best_index = i;  
                }
            }
        }
        
        // If not process is ready yet.
        if (best_index == -1) {
            int next_arrival = INT_MAX;

            // Looking for the closest arrival among the unfinished processes.
            for (int i = 0; i < count; i++) {
                if (!done[i] && processes[i].arrival_time > current_time) {
                    next_arrival = MIN(next_arrival, processes[i].arrival_time);
                }
            }
            print_schedule_entry(current_time, next_arrival, NULL);

            // Update time to the next process.
            current_time = next_arrival;
    
        }
        else {
            Process *p = &processes[best_index];

            int start_time = current_time;
            int end_time = start_time + p->burst_time;

            int waiting_time = start_time - p->arrival_time;
            total_waiting_time += waiting_time;

            print_schedule_entry(start_time, end_time, p);
            simulate_run(p, p->burst_time);

            current_time = end_time;
            done[best_index] = 1; // Mark process as done.
            completed++;
        }
    }

    // Calculate and print avg waiting time.
    double avg_waiting_time = total_waiting_time / count;
    print_summary(avg_waiting_time);
    printf("\n");
}
// Round Robin scheduling simulation.

void schedule_rr(Process processes[], int count, int quantum) {
    print_schedule_header("Round Robin");

    int time = 0;
    int completed = 0;
    int remaining[1000];
    int arrived[1000] = {0};
    int queue[1000], front = 0, rear = 0;
    int finish_time[1000] = {0};

    for (int i = 0; i < count; i++) {
        remaining[i] = processes[i].burst_time;
    }

    // Enqueue all processes that have arrived at time 0
    for (int i = 0; i < count; i++) {
        if (processes[i].arrival_time == 0) {
            queue[rear++] = i;
            arrived[i] = 1;
        }
    }

    while (completed < count) {
        // If queue is empty, jump to next arrival
        if (front == rear) {
            int next_arrival_time = -1;
            for (int i = 0; i < count; i++) {
                if (!arrived[i]) {
                    if (next_arrival_time == -1 || processes[i].arrival_time < next_arrival_time)
                        next_arrival_time = processes[i].arrival_time;
                }
            }
            if (next_arrival_time != -1 && next_arrival_time > time) {
                print_schedule_entry(time, next_arrival_time, NULL);
                time = next_arrival_time;
                // Enqueue all processes that arrive at this time
                for (int i = 0; i < count; i++) {
                    if (!arrived[i] && processes[i].arrival_time == time) {
                        queue[rear++] = i;
                        arrived[i] = 1;
                    }
                }
            }
            continue;
        }

        int idx = queue[front++];
        Process *p = &processes[idx];

        // If the process arrives after the current time, CPU should be idle
        if (p->arrival_time > time) {
            print_schedule_entry(time, p->arrival_time, NULL);
            time = p->arrival_time;
        }

        int slice = (remaining[idx] < quantum) ? remaining[idx] : quantum;
        int start = time;
        int end = time + slice;

        print_schedule_entry(start, end, p);

        time = end;
        remaining[idx] -= slice;

        // Enqueue any new arrivals during this time slice, in arrival order
        for (int i = 0; i < count; i++) {
            if (!arrived[i] && processes[i].arrival_time > start && processes[i].arrival_time <= end) {
                queue[rear++] = i;
                arrived[i] = 1;
            }
        }

        // Only after enqueuing new arrivals, re-queue the current process if not finished
        if (remaining[idx] > 0) {
            queue[rear++] = idx;
        } else {
            completed++;
            finish_time[idx] = end;
        }
    }

    // Calculate total turnaround time (from first arrival to last finish)
    int last_finish = 0;
    for (int i = 0; i < count; i++) {
        if (finish_time[i] > last_finish)
            last_finish = finish_time[i];
    }
    print_turnaround_summary((double)last_finish);
}


void runCPUScheduler(char* processesCsvFilePath, int timeQuantum) {

    Process processes[1000];
    //printf("Time Quantum: %d\n", timeQuantum);
    int count = load_processes(processesCsvFilePath, processes);
    if (count <= 0) {
        fprintf(stderr, "Error: Failed to load processes from file: %s\n", processesCsvFilePath);
        return;
    }

    // Create copies of the processes for each scheduler.
    Process fcfs_list[1000], sjf_list[1000], prio_list[1000], rr_list[1000];
    memcpy(fcfs_list, processes, sizeof(Process) * count);
    memcpy(sjf_list, processes, sizeof(Process) * count);
    memcpy(prio_list, processes, sizeof(Process) * count);
    memcpy(rr_list, processes, sizeof(Process) * count);

    // Run each scheduling simulation.
    schedule_fcfs(fcfs_list, count);
    schedule_sjf(sjf_list, count);
    schedule_priority(prio_list, count);
    schedule_rr(rr_list, count, timeQuantum);
}