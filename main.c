#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define UNIT_TIME() { volatile unsigned long unit; for(unit=0;unit<1000000UL;unit++); } 
#define FIFO 0
#define RR 1 
#define SJF 2
#define PSJF 3

// #define DEBUG

int queue[30];
int number = 0, front = 0, end = 0;

typedef struct proc{
    char name[64];
    int ready_t, exec_t, ready, finish;
    pid_t pid;
}process_info;

int cmp(const void *a, const void *b) {
    return ((process_info *)a)->ready_t - ((process_info *)b)->ready_t;
}
int pop(){
    if (number == 0)
        return -1;
    int value;
    value = queue[front];
    front = (front + 1) % 30;
    number--;
    return value;
}
int insert(int value){
    if (number >= 30){
        fprintf(stderr, "queue full\n");
        return -1;
    }
    number++;
    queue[end] = value;
    end = (end + 1) % 30;
    //fprintf(stderr, "queue number %d\n", number);
    return 1;
}
pid_t create_process(process_info *process){
    pid_t pid = fork();
    int time = 0;
    if (pid == 0){
        
        cpu_set_t child_set;        
        CPU_ZERO(&child_set);       /* Initialize */
        CPU_SET(1, &child_set);     /* set the core 1 for child */
        sched_setaffinity(getpid(), sizeof(cpu_set_t), &child_set);
        
        while(time < process->exec_t){
            UNIT_TIME();
            time++;    
        }
        
        exit(0);
    }
    return pid;
}

int stop_process(pid_t pid){
    struct sched_param param;
    param.sched_priority = 0;
    return sched_setscheduler(pid, SCHED_IDLE, &param);
}

int start_process(pid_t pid){
    struct sched_param param;
    param.sched_priority = 0;
    return sched_setscheduler(pid, SCHED_OTHER, &param);
}

void schedule(int n, process_info *process, int policy){
    //sort the processes by ready time
    qsort(process, n, sizeof(process_info), cmp);
#ifdef DEBUG    
    fprintf(stderr, "Sort the processes by ready time\n" );
    for (int i = 0; i < n; i++)
        fprintf(stderr, "%s %d %d\n", process[i].name, process[i].ready_t, process[i].exec_t);
#endif
    int time = 0;
    int running_proc = -1;
    int prev_start_t = 0;
    int finished_proc = 0;
    number = 0, front = 0, end = 0;
    unsigned long start_sec[30], start_nsec[30], end_sec[30], end_nsec[30];
    char proc_time[256];
    start_process(getpid());
    while(finished_proc < n){
        for (int i = 0; i < n; i++){
            if (process[i].ready_t == time){
                process[i].ready = 1;
                insert(i);
                //Start a process at its ready time
                //process[i].pid = create_process(&process[i]);

                //Block the process immediately
                //stop_process(process[i].pid);
#ifdef DEBUG
                //fprintf(stderr, "process %s start at %d pid = %d\n", process[i].name, time, process[i].pid);
#endif
            }
        }
        if (running_proc != -1 && process[running_proc].exec_t == 0){
            //wait for the child process if it has finished execution
#ifdef DEBUG
            fprintf(stderr, "Wait for process %s with pid = %d......", process[running_proc].name, process[running_proc].pid);
#endif
            waitpid(process[running_proc].pid, NULL, 0);
            syscall(333, &end_sec[running_proc], &end_nsec[running_proc]);
            sprintf(proc_time, "[Project1] %d %lu.%09lu %lu.%09lu\n", process[running_proc].pid, start_sec[running_proc], start_nsec[running_proc], end_sec[running_proc], end_nsec[running_proc]);
            syscall(334, proc_time);
#ifdef DEBUG
            fprintf(stderr, "Reap the process at time %d\n", time);
#endif
            process[running_proc].pid = -1;
            process[running_proc].ready = 0;
            running_proc = -1;
            finished_proc++;
        }

        if (policy == PSJF){
            int shortest = -1;
            for (int i = 0; i < n; i++){
                //if (process[i].pid != -1){
                if (process[i].ready == 1){
                    if (shortest != -1){
                        if(process[i].exec_t < process[shortest].exec_t)
                            shortest = i;
                    }
                    else
                        shortest = i;
                }
            }
            
            if (shortest != -1){ // found shortest job
                if (running_proc != -1){ // stop the current process
                    stop_process(process[running_proc].pid);
#ifdef DEBUG
                    if (running_proc != shortest)
                        fprintf(stderr, "context switch from process %s to process %s at time %d\n", process[running_proc].name, process[shortest].name, time);
#endif
                }
#ifdef DEBUG
                else
                    fprintf(stderr, "Run process %s at time %d\n", process[shortest].name, time);
#endif                
                if (process[shortest].pid == -1){
                    
                    process[shortest].pid = create_process(&process[shortest]);
                    syscall(333, &start_sec[shortest], &start_nsec[shortest]);
                    printf("%s %d\n", process[shortest].name, process[shortest].pid);
                    fflush(stdout);
                }
                start_process(process[shortest].pid);    
            }
            
            running_proc = shortest;
            
        }
        else if (policy == RR){
            if (running_proc == -1 || (time - prev_start_t) == 500){
                if (running_proc != -1){
#ifdef DEBUG
                    fprintf(stderr, "process %s expired at time %d\n", process[running_proc].name, time);
#endif                                        
                    stop_process(process[running_proc].pid);
                    insert(running_proc);
                    running_proc = -1;
                }
                if (number > 0){
                    // for (int i = front; i < end; i++)
                    //     fprintf(stderr, "%d ", queue[i]);
                    // fprintf(stderr, "\n");
                    // fprintf(stderr, "front = %d end = %d\n", front, end);
                    running_proc = pop();
                    if (process[running_proc].pid == -1){
                        
                        process[running_proc].pid = create_process(&process[running_proc]);
                        syscall(333, &start_sec[running_proc], &start_nsec[running_proc]);
                        printf("%s %d\n", process[running_proc].name, process[running_proc].pid);
                        fflush(stdout);
                    }
                    start_process(process[running_proc].pid);
                    prev_start_t = time;
#ifdef DEBUG
                    fprintf(stderr, "Run process %s at time %d\n", process[running_proc].name, time);
#endif                                        
                }

            }
        }
        else{
            if (running_proc == -1){ //No running process
            //choose a job to run by the policy
                if (policy == FIFO){
                    for (int i = 0; i < n; i++){
                        //if (process[i].pid != -1){
                        if (process[i].ready == 1){
                            if (process[i].pid == -1){
                                
                                process[i].pid = create_process(&process[i]);
                                syscall(333, &start_sec[i], &start_nsec[i]);
                                printf("%s %d\n", process[i].name, process[i].pid);
                                fflush(stdout);
                            }
                            start_process(process[i].pid);
                            running_proc = i;
                            prev_start_t = time;
#ifdef DEBUG                            
                            fprintf(stderr, "Run process %s at time %d\n", process[i].name, time);
#endif
                            break;
                        }   
                    }
                }
                else if (policy == SJF){
                    int shortest = -1;
                    for (int i = 0; i < n; i++){
                        // if (process[i].pid != -1){
                        if (process[i].ready == 1){
                            if (shortest != -1){
                                if(process[i].exec_t < process[shortest].exec_t)
                                    shortest = i;
                            }
                            else
                                shortest = i;
                        }
                    }
                    if (shortest != -1){
                        if (process[shortest].pid == -1){
                            
                            process[shortest].pid = create_process(&process[shortest]);
                            syscall(333, &start_sec[shortest], &start_nsec[shortest]);
                            printf("%s %d\n", process[shortest].name, process[shortest].pid);
                            fflush(stdout);
                        }
                        start_process(process[shortest].pid);
#ifdef DEBUG                        
                        fprintf(stderr, "Run process %s at time %d\n", process[shortest].name, time);
#endif
                    }
                    running_proc = shortest;
                }
            }
            /*else{
                if (policy == RR){ // Round Robin policy check for the time quantum
                    if ((time - prev_start_t) == 500){

                        int i = (running_proc + 1) % n;
                        while (i != running_proc && process[i].ready != 1) // circular find a process that has been created
                            i = (i + 1) % n;
                        if (i != running_proc){
#ifdef DEBUG
                            fprintf(stderr, "Process %s expired, switch to process %s at time %d\n", process[running_proc].name, process[i].name, time);
#endif
                            stop_process(process[running_proc].pid);
                            if (process[i].pid == -1)
                                process[i].pid = create_process(&process[i]);
                            start_process(process[i].pid);
                        }
                        running_proc = i;
                        prev_start_t = time;
                    }
                }
            }*/
    
        }

        UNIT_TIME();
        time++;
        if (running_proc != -1){
            process[running_proc].exec_t--;
        }
#ifdef DEBUG
        if(time % 5000 == 0)
            fprintf(stderr, "time %d\n", time);
#endif        
    }
    
}

int main(int argc, char const *argv[])
{
    char policy_name[64]; // scheduling policy
    int n; //the number of processes
    int policy; //specify the schedule policy
    process_info *process;

    scanf("%s", policy_name);
    scanf("%d", &n);
    process = (process_info*)malloc(n*sizeof(process_info));
    for (int i = 0; i < n; i++){
        scanf("%s%d%d", process[i].name, &process[i].ready_t, &process[i].exec_t);
        process[i].pid = -1;
        process[i].ready = 0;
        process[i].finish = 0;
#ifdef DEBUG
	    fprintf(stderr, "%s %d %d\n", process[i].name, process[i].ready_t, process[i].exec_t);
#endif
    }
    if (strcmp(policy_name, "FIFO") == 0)
        policy = FIFO;
    else if (strcmp(policy_name, "RR") == 0)
        policy = RR;
    else if (strcmp(policy_name, "SJF") == 0)
        policy = SJF;
    else if (strcmp(policy_name, "PSJF") == 0)
        policy = PSJF;
    else
        fprintf(stderr, "No such scheduling policy\n");
    cpu_set_t scheduler_set;        
    CPU_ZERO(&scheduler_set);       /* Initialize */
    CPU_SET(0, &scheduler_set);     /* set the core 0 for scheduler */
    sched_setaffinity(getpid(), sizeof(cpu_set_t), &scheduler_set); 
    schedule(n, process, policy);

    return 0;
}
