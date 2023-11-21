/*
 * user_process.c
 *
 *  Created on: Oct 27 2022
 *      Author: Daniel Park
 */
#include <signal.h>
#include "userProcess.h"

int isTerminated()
{
	return generateRandomNumber(GET_RANDOM_PERCENTAGE, 0) <= TERMINATE_PROBABILITY ? TRUE : FALSE;
}

int generateRandomNumber(int flag, int time_slice)
{
	switch (flag)
	{
		case GET_RANDOM_PERCENTAGE:
			return rand() % 100 + 1;
			break;
		case TIME_SLICE_USAGE_RANDOM:
			return rand() % time_slice + 1;
			break;
		default:
			return 0;
			break;
	}
}

void timeout()
{
	printf("\nTime out, so killing all the processes, message queues, and shm\n");
	terminate();
	return;
}

void interrupt()
{
	printf("\nInterrupt, so killing all the processes, message queues, and shm\n");
	terminate();
}

void terminate()
{
	time_t current_time;
	struct tm * time_info;
	key_t oss_msgqueue_key;
	int oss_msgqueue_id;
	key_t user_msgqueue_key;
	int user_msgqueue_id;
	key_t sysclock_key;
	int sysclock_id;
	key_t process_table_key;
	int process_table_id;

	sysclock_key = ftok("/tmp", 'A');
	oss_msgqueue_key = ftok("/tmp", 'B');
	user_msgqueue_key = ftok("/tmp", 'C');
	process_table_key = ftok("/tmp", 'D');


    sysclock_id = shmget ( sysclock_key, sizeof(unsigned int), 0777 | IPC_CREAT );
    oss_msgqueue_id = msgget(oss_msgqueue_key, 0666 | IPC_CREAT);
    user_msgqueue_id = msgget(user_msgqueue_key, 0666 | IPC_CREAT);
	process_table_id = shmget (process_table_key , sizeof(proc_ctrl_blck) * MAX_NUM_USER_PROC, 0777 | IPC_CREAT );

	msgctl(oss_msgqueue_id, IPC_RMID,NULL);
	msgctl(user_msgqueue_id, IPC_RMID,NULL);
    shmctl(sysclock_id, IPC_RMID,NULL);
    shmctl(process_table_id, IPC_RMID,NULL);
    kill(0, SIGKILL);
}

int main(int argc, char** argv)
{
	srand(time(NULL) + getpid());
	char perrorOutput[100];
	int current_process_id = atoi(argv[1]);
	int cpu_or_io = generateRandomNumber(GET_RANDOM_PERCENTAGE, 0) <= 50 ? CPU_BOUND : IO_BOUND;

	printf("current_process_id : %d\n", atoi(argv[1]));
	key_t oss_msgqueue_key;
	int oss_msgqueue_id;
	key_t user_msgqueue_key;
	int user_msgqueue_id;
	message msg;
	key_t process_table_key;
	int process_table_id;
	proc_ctrl_blck *proc_ctrl_table;
	proc_ctrl_blck proc_ctrl_block;

	oss_msgqueue_key  = ftok("/tmp", 'B');
	user_msgqueue_key = ftok("/tmp", 'C');
	process_table_key = ftok("/tmp", 'D');

	if (oss_msgqueue_key == (key_t) -1 || user_msgqueue_key == (key_t) -1 || process_table_key == (key_t) -1)
	{
		printf("Error during ftok\n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
	}

	oss_msgqueue_id  = msgget(oss_msgqueue_key, 0666);
	user_msgqueue_id = msgget(user_msgqueue_key, 0666);
	process_table_id = shmget (process_table_key , sizeof(proc_ctrl_blck) * MAX_NUM_USER_PROC, 0777 | IPC_CREAT );

	if ( oss_msgqueue_id == -1 || user_msgqueue_id == -1 || process_table_id == -1 )
	{
		printf("Error during get\n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
	}

	proc_ctrl_table = shmat( process_table_id, 0, 0 );
	proc_ctrl_block = proc_ctrl_table[current_process_id];

	strcpy(perrorOutput, argv[0]);
	strcat(perrorOutput, ": Error: ");

	while (1)
	{
		printf("user waiting for message\n");
		int msg_rcv = msgrcv(user_msgqueue_id, &msg, (sizeof(message)-sizeof(long)), current_process_id, 0);
		if (msg_rcv != -1)
		{
			msg.mesg_type = PARENT_QUEUE_ADDRESS;
			if (isTerminated())
			{
				msg.is_terminated  = TRUE;
				msg.is_interrupted = FALSE;
				msg.time_slice = generateRandomNumber(TIME_SLICE_USAGE_RANDOM, msg.time_slice);
				msgsnd(oss_msgqueue_id, &msg, sizeof(message), 0);
			}
			else
			{
				int interrupted_random_pct = generateRandomNumber(GET_RANDOM_PERCENTAGE, 0);
				// cpu-bound processes should be very unlikely to get interrupted (so they will usually use up their entire timeslice without getting interrupted).
				// On the other hand, i/o-bound processes should more likely than not get interrupted before finishing their time slices.
				int interrupt_percentage = cpu_or_io == CPU_BOUND ? CPU_BOUND_INTERRUPT_PCT : IO_BOUND_INTERRUPT_PCT;
				int is_interrupted = interrupted_random_pct <= interrupt_percentage ? TRUE : FALSE;

				msg.is_terminated = FALSE;
				// If not interrupted, use all time slice, if interrupted randomize the number less than time slice
				msg.is_interrupted = is_interrupted;
				msg.time_slice = is_interrupted ? generateRandomNumber(TIME_SLICE_USAGE_RANDOM, msg.time_slice) : msg.time_slice;

				msgsnd(oss_msgqueue_id, &msg, sizeof(message), 0);
			}
		}
	}

	return EXIT_SUCCESS;
}