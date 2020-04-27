#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define UNIT_TIME() { volatile unsigned long i; for(i=0;i<1000000UL;i++); } 
#define FIFO 0
#define RR 1 
#define SJF 2
#define PSJF 3


typedef struct proc{
    char name[64];
    int ready_t, exec_t;
    pid_t pid;
}process_info;

int cmp(const void *a, const void *b) {
    return ((process_info *)a)->ready_t - ((process_info *)b)->exec_t;
}

pid_t create_process(process_info *process){
    pid_t pid = fork();
    int time = 0;
    if (pid == 0){
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
    int time = 0;
    int running_proc = -1;
    int prev_start_t = 0;
    int finished_proc = 0;
    while(finished_proc < n){
        for (int i = 0; i < n; i++){
            if (process[i].ready_t == time){
                //Start a process at its ready time
                process[i].pid = create_process(&process[i]);
                //Block the process immediately
                stop_process(process[i].pid);
            }
        }
        if (process[running_proc].exec_t == 0){
            //wait for the child process if it has finished execution
            fprintf(stderr, "Wait for process %d with pid = %d...\n", running_proc, process[running_proc].pid);
            waitpid(process[running_proc].pid, NULL, 0);
            fprintf(stderr, "Reap the process\n");
            process[running_proc].pid = -1;
            running_proc = -1;
            finished_proc++;
        }

        if (policy == PSJF){
            int shortest = -1;
            for (int i = 0; i < n; i++){
                if (process[i].pid != -1){
                    if (shortest != -1){
                        if(process[i].exec_t < process[shortest].exec_t)
                            shortest = i;
                    }
                    else
                        shortest = i;
                }
            }
            
            if (shortest != -1){ // found shortest job
                if (running_proc != -1) // stop the current process
                    stop_process(process[running_proc].pid);
                start_process(process[shortest].pid);    
            }
            
            running_proc = shortest;
            
        }
        else{
            if (running_proc == -1){ //No running process
            //choose a job to run by the policy
                if (policy == FIFO || policy == RR){
                    for (int i = 0; i < n; i++){
                        if (process[i].pid != -1){
                            start_process(process[i].pid);
                            running_proc = i;
                            break;
                        }   
                    }
                }
                else if (policy == SJF){
                    int shortest = -1;
                    for (int i = 0; i < n; i++){
                        if (process[i].pid != -1){
                            if (shortest != -1){
                                if(process[i].exec_t < process[shortest].exec_t)
                                    shortest = i;
                            }
                            else
                                shortest = i;
                        }
                    }
                    if (shortest != -1)
                        start_process(process[shortest].pid);
                    running_proc = shortest;
                }
            }
            else{
                if (policy == RR){ // Round Robin policy check for the time quantum
                    if ((time - prev_start_t) == 500){
                        int i = (running_proc + 1) % n;
                        while (i != running_proc && process[i].pid == -1) // circular find a process that has been created
                            i = (i + 1) % n;
                        if (i != running_proc){
                            stop_process(process[running_proc].pid);
                            start_process(process[i].pid);
                        }
                        running_proc = i;
                        prev_start_t = time;
                    }
                }
            }
    
        }

        UNIT_TIME();
        time++;
        if (running_proc != -1){
            process[running_proc].exec_t--;
        }
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
	fprintf(stderr, "%s %d %d\n", process[i].name, process[i].ready_t, process[i].exec_t);
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
    schedule(n, process, policy);

    return 0;
}
