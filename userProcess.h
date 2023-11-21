/*
 * user_process.h
 *
 *  Created on: Oct 27, 2022
 *      Author: kihong.park
 */

#ifndef PROJECT_4_USER_PROCESS_H_
#define PROJECT_4_USER_PROCESS_H_

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <time.h>

#define BUFF_SZ	sizeof ( int )
#define TERMINATE_PROBABILITY 1
#define PARENT_QUEUE_ADDRESS 999
#define MAX_NUM_USER_PROC 17
#define TRUE  1
#define FALSE 0

#define CPU_BOUND_INTERRUPT_PCT 5
#define IO_BOUND_INTERRUPT_PCT 60

#define GET_RANDOM_PERCENTAGE 0
#define TIME_SLICE_USAGE_RANDOM 1

#define CPU_BOUND 0
#define IO_BOUND  1

int  isTerminated();
int  generateRandomNumber(int flag, int time_slice);
void timeout();
void interrupt();
void terminate();

typedef struct
{
    long mesg_type;
    int time_slice;
    int is_terminated;
    int is_interrupted;
} message;

typedef struct
{
	unsigned int total_used_cpu_time_sec;
	unsigned int total_used_cpu_time_ns;
	unsigned int total_system_time;
	unsigned int total_last_burst_time;
	unsigned int local_sim_pid;
	unsigned int is_blocked;
	unsigned int event_happen_s;
	unsigned int event_happen_ns;
	unsigned int proc_priority;
} proc_ctrl_blck;

#endif /* PROJECT_4_USER_PROCESS_H_ */