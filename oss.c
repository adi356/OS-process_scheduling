/*
 *  Author: Daniel Park
 *  Class: CMP_SCI_4760_002
 *  Project 4
 *  10 / 26 / 2022
 *  The goal of this homework is to become familiar with semaphores in Linux.
 *
 */

#include <signal.h>
#include "oss.h"
#include "queue.h"

void help()
{
	printf("The inputs for this program are: \n");
	printf("-h   : To see help message \n");
	printf("-s t : t is the maximum time in seconds before the system terminates (default 100 seconds)\n");
	printf("-l f : f is the name for the log file\n");
}

int checkToCreateNewProcess(struct Queue* pid_queue, system_clock* sysclock, int last_proc_created_sec, int last_proc_created_ns)
{
	printf("checkToCreateNewProcess\n");
	int ns_to_seconds;

	int current_sec = sysclock->sec;
	int current_ns  = sysclock->nano_sec;
	// A new process should be generated every 1 second, on an average.
	int create_process_at_sec = last_proc_created_sec + generateRandomNumber(PROC_CREATE_SEC_RANDOM);
	int create_process_at_ns  = last_proc_created_ns + generateRandomNumber(PROC_CREATE_NS_RANDOM);
	// Creating a new process should not exceed maxTimeBetweenNewProc
	int max_create_process_at_sec = last_proc_created_sec + maxTimeBetweenNewProcsSecs;
	int max_create_process_at_ns  = last_proc_created_ns + maxTimeBetweenNewProcsNS;

	if (create_process_at_ns >= ONE_SEC_IN_NANO)
	{
		ns_to_seconds = 0;
		ns_to_seconds += create_process_at_ns / ONE_SEC_IN_NANO;
		create_process_at_ns -= ns_to_seconds * ONE_SEC_IN_NANO;
		create_process_at_sec += ns_to_seconds;
	}

	if (max_create_process_at_ns >= ONE_SEC_IN_NANO)
	{
		ns_to_seconds = 0;
		ns_to_seconds += max_create_process_at_ns / ONE_SEC_IN_NANO;
		max_create_process_at_ns -= ns_to_seconds * ONE_SEC_IN_NANO;
		max_create_process_at_sec += ns_to_seconds;
	}

	if (isEmpty(pid_queue))
	{
		return FALSE;
	}
	else if (current_sec == 0 && current_ns == 0)
	{
		return TRUE;
	}
	else if (isAfter(current_sec, current_ns, create_process_at_sec, create_process_at_ns)
		     || isAfter(current_sec, current_ns, max_create_process_at_sec, max_create_process_at_ns))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

int isAfter(int current_sec, int current_ns, int compare_sec, int compare_ns)
{
	return current_sec > compare_sec || (current_sec == compare_sec && current_ns > compare_ns);
}

int generateRandomNumber(int flag)
{
//	printf("Generating flag %d\n", flag);
	switch (flag)
	{
		case TOTAL_DISPATCH_RANDOM:
			return rand() % (10000 + 1 - 100) + 100;
			break;
		case REAL_OR_USER_RANDOM:
			return rand() % 100 + 1;
			break;
		case LOOP_ADVANCE_RANDOM:
			return rand() % 1000;
			break;
		case PROC_CREATE_SEC_RANDOM:
			return rand() % 3; // generate between 0 to 2
			break;
		case PROC_CREATE_NS_RANDOM:
			return rand() % INT_MAX; // generate between 0 to 2
			break;
		case EVENT_HAPPEN_SEC_RANDOM:
			return rand() % 5 + 1;  // range [0,5]
			break;
		case EVENT_HAPPEN_NS_RANDOM:
			return rand() % 1000 + 1; // range [0,1000]
			break;
		default:
			return 0;
			break;
	}
}

void incrementSysClock(system_clock* sysclock, int seconds, int nano_seconds)
{
	int ns_to_seconds = 0;

	// Handling incoming nano seconds bigger than 1 sec in nano seconds
	if (nano_seconds >= ONE_SEC_IN_NANO)
	{
		ns_to_seconds += nano_seconds / ONE_SEC_IN_NANO;
		nano_seconds  -= ns_to_seconds * ONE_SEC_IN_NANO;
	}
	// Handling sysclock nano seconds bigger than 1 sec in nano seconds
	sysclock->sec += seconds + ns_to_seconds;
	sysclock->nano_sec += sysclock->nano_sec + nano_seconds;

	// recalculate current time if nano second is bigger than one sec
	ns_to_seconds = 0;
	if (sysclock->nano_sec >= ONE_SEC_IN_NANO)
	{
		ns_to_seconds += sysclock->nano_sec / ONE_SEC_IN_NANO;
		sysclock->sec += ns_to_seconds;
		sysclock->nano_sec -= (ns_to_seconds * ONE_SEC_IN_NANO);
	}
}

struct Queue* initPidQueue()
{
	struct Queue* pids = createQueue(MAX_NUM_USER_PROC);
	int i;
	for (i = 1; i <= MAX_NUM_USER_PROC; i++)
	{
		enqueue(pids, i);
	}
	return pids;
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
	system("killall user");
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
	srand(time(NULL));
	int maxTimeToRunProcess = 100;
	char perrorOutput[100];
	char* fileName = "logfile";
	key_t oss_msgqueue_key;
	int oss_msgqueue_id;
	key_t user_msgqueue_key;
	int user_msgqueue_id;
	key_t sysclock_key;
	int sysclock_id;
	system_clock* sysclock;
	key_t process_table_key;
	int process_table_id;
	proc_ctrl_blck *proc_ctrl_table;
	int pid_in_use[MAX_NUM_USER_PROC];
	int line_number = 0;
	struct Queue* available_pids         = initPidQueue();
	struct Queue* highest_priority_queue = createQueue(MAX_NUM_USER_PROC);
	struct Queue* second_priority_queue  = createQueue(MAX_NUM_USER_PROC);
	struct Queue* third_priority_queue   = createQueue(MAX_NUM_USER_PROC);
	struct Queue* blocked_queue     = createQueue(MAX_NUM_USER_PROC);

	int last_proc_created_sec = 0;
	int last_proc_created_ns  = 0;

	strcpy(perrorOutput, argv[0]);
	strcat(perrorOutput, ": Error: ");

	int option;
	while ( (option = getopt(argc, argv, "hs:l:")) != -1 )
	{
		switch(option)
		{
			case 'h':
				help();
				return EXIT_SUCCESS;
				break;
			case 's':
				maxTimeToRunProcess = atoi(optarg);
				printf("Maximum time in seconds: %d\n", maxTimeToRunProcess);
				break;
			case 'l':
				fileName = optarg;
				printf("Logfile name is : %s\n", fileName);
				break;
			case '?':
				if (isprint (optopt))
				   fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
				   fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return EXIT_FAILURE;
			default:
				help();
				return EXIT_SUCCESS;
		}
	}

	FILE* logfile = fopen(fileName, "w");

	sysclock_key = ftok("/tmp", 'A');
	oss_msgqueue_key = ftok("/tmp", 'B');
	user_msgqueue_key = ftok("/tmp", 'C');
	process_table_key = ftok("/tmp", 'D');

	if (sysclock_key == (key_t) -1 || oss_msgqueue_key == (key_t) -1 || user_msgqueue_key == (key_t) -1 || process_table_key == (key_t) -1)
	{
	    printf("Error during ftok\n");
	    perror(perrorOutput);
	    exit(EXIT_FAILURE);
	}

    sysclock_id = shmget (sysclock_key , sizeof(system_clock), 0777 | IPC_CREAT );
    oss_msgqueue_id = msgget(oss_msgqueue_key, 0666 | IPC_CREAT);
    user_msgqueue_id = msgget(user_msgqueue_key, 0666 | IPC_CREAT);
    process_table_id = shmget (process_table_key , sizeof(proc_ctrl_blck) * MAX_NUM_USER_PROC, 0777 | IPC_CREAT );
    if ( sysclock_id == -1 || oss_msgqueue_id == -1 || user_msgqueue_id == -1 || process_table_id == -1 )
    {
		printf("master: Error in shmget \n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
    }

    // Initialize system clock
    sysclock = shmat( sysclock_id, 0, 0 );
    if ( sysclock == (void *)-1  )
    {
		printf("master: Error in shmat sysclock \n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
    }

    sysclock -> sec = 0;
    sysclock -> nano_sec = 0;

    if ( sysclock == (void *)-1  )
    {
		printf("master: Error in shmat sysclock \n");
		perror(perrorOutput);
		exit(EXIT_FAILURE);
    }

    proc_ctrl_table = shmat( process_table_id, 0, 0 );

    signal(SIGINT, interrupt);
    signal(SIGALRM, timeout);
    alarm(maxTimeToRunProcess); // specified number of seconds (default: 100).

	while (line_number < 10000) // while line number is less than 10000
	{
		if (checkToCreateNewProcess(available_pids, sysclock, last_proc_created_sec, last_proc_created_ns))
		{
			int pid_to_use = dequeue(available_pids);
			int real_or_normal = generateRandomNumber(REAL_OR_USER_RANDOM) <= REAL_TIME_PROC_PERCENTAGE ? REAL_TIME : USER_PROCESS;
			pid_t childPid = fork();
			if (childPid == 0)
			{
				char arg1[10];
				char arg2[10];
				sprintf(arg1, "%d", pid_to_use);
				execl("./user", "./user", arg1, (char*)0);
			}
			else
			{
				proc_ctrl_blck proc_ctrl_block;
				proc_ctrl_block.total_used_cpu_time_sec = 0;
				proc_ctrl_block.total_used_cpu_time_ns = 0;
				proc_ctrl_block.total_last_burst_time = 0;
				proc_ctrl_block.total_system_time = 0;
				proc_ctrl_block.local_sim_pid = pid_to_use;
				proc_ctrl_block.is_blocked = FALSE;
				proc_ctrl_block.event_happen_s = 0;
				proc_ctrl_block.event_happen_ns = 0;
				proc_ctrl_block.proc_priority = real_or_normal;
				proc_ctrl_table[pid_to_use] = proc_ctrl_block;

				if (real_or_normal == REAL_TIME)
				{
					// Schedule real-time processes at highest priority using FIFO*because of short CPU bursts
					enqueue(highest_priority_queue, pid_to_use);
				}
				else if (real_or_normal == USER_PROCESS)
				{
					// Schedule batch and interactive processes using multilevel feedback queue
					enqueue(second_priority_queue, pid_to_use);

				}
				fprintf(logfile, "OSS: Generating process with PID %d and putting it in queue %d at time %d:%d\n",
						pid_to_use, real_or_normal, sysclock->sec, sysclock->nano_sec);
				fflush(logfile);
				// Keeps the record of when the lastest process was created
				last_proc_created_sec = sysclock->sec;
				last_proc_created_ns  = sysclock->nano_sec;
				line_number++;
			}

		}
		else if (!isEmpty(highest_priority_queue) || !isEmpty(second_priority_queue) || !isEmpty(third_priority_queue))
		{
			int total_dispatch_time = generateRandomNumber(TOTAL_DISPATCH_RANDOM);
			int pid_to_process = 0;
			int queue_id = 0;
			int time_slice = 0;
			int checked_blocked_queue = FALSE;
			int still_blocked_pid_cnt = 0;
			int blocked_pids[MAX_NUM_USER_PROC];
			incrementSysClock(sysclock, 0, total_dispatch_time);

			// Starvation prevention
			// Every minute, go through each high priority, if a process's total cpu time is
			// bigger than 25% of the total sysclock, then increment its priority
			if (sysclock->sec % 60 == 0)
			{
				fprintf(logfile, "OSS: Starvation Prevention run at time %d:%d\n",
						sysclock->sec, sysclock->nano_sec);
				line_number++;
				int total_time = 0;
				int num_of_proc = 0;

				int pid_to_highest_queue[MAX_NUM_USER_PROC];
				int pid_to_second_queue[MAX_NUM_USER_PROC];
				int pid_to_third_queue[MAX_NUM_USER_PROC];

				int highest_queue_cnt = 0;
				int second_queue_cnt = 0;
				int third_queue_cnt = 0;

				while (!isEmpty(highest_priority_queue))
				{
					int pid_to_check = dequeue(highest_priority_queue);
					proc_ctrl_blck proc_ctrl_block = proc_ctrl_table[pid_to_check];

					// If a process used more than 25% of the total sysclock then decrease the priority
					if (proc_ctrl_block.total_used_cpu_time_sec >= (sysclock->sec / 4))
					{
						proc_ctrl_block.proc_priority += 1;
						pid_to_second_queue[second_queue_cnt] = pid_to_check;
						second_queue_cnt++;
					}
					else
					{
						pid_to_highest_queue[highest_queue_cnt] = pid_to_check;
						highest_queue_cnt++;
					}
				}

				while (!isEmpty(second_priority_queue))
				{
					int pid_to_check = dequeue(second_priority_queue);
					proc_ctrl_blck proc_ctrl_block = proc_ctrl_table[pid_to_check];

					// If a process used more than 25% of the total sysclock then decrease the priority
					if (proc_ctrl_block.total_used_cpu_time_sec >= (sysclock->sec / 4))
					{
						proc_ctrl_block.proc_priority += 1;
						pid_to_third_queue[third_queue_cnt] = pid_to_check;
						third_queue_cnt++;
					}
					else
					{
						pid_to_second_queue[second_queue_cnt] = pid_to_check;
						second_queue_cnt++;
					}
				}

				// Put all the process into where they belong
				int i;
				for (i = 0; i < highest_queue_cnt; i++)
				{
					enqueue(highest_priority_queue, pid_to_highest_queue[i]);
				}

				for (i = 0; i < second_queue_cnt; i++)
				{
					enqueue(second_priority_queue, pid_to_second_queue[i]);
				}

				for (i = 0; i < third_queue_cnt; i++)
				{
					enqueue(third_priority_queue, pid_to_third_queue[i]);
				}
			}

			// Hanling blocked queue
			while (!checked_blocked_queue)
			{
				if (isEmpty(blocked_queue))
				{
					checked_blocked_queue = TRUE;

					// Putting back the processes not pass event time back to blocked queue
					if (still_blocked_pid_cnt > 0)
					{
						int i;
						for (i = 0; i < still_blocked_pid_cnt; i++)
						{
							enqueue(blocked_queue, blocked_pids[i]);
						}
					}
				}
				else
				{
					int blocked_pid = dequeue(blocked_queue);
					proc_ctrl_blck proc_ctrl_block = proc_ctrl_table[blocked_pid];

					// Check if the event time is after
					if (isAfter(sysclock->sec, sysclock->nano_sec, proc_ctrl_block.event_happen_s, proc_ctrl_block.event_happen_ns))
					{
						// If a process comes out of a blocked queue, it should go to the highest priority queue.
						enqueue(highest_priority_queue, blocked_pid);
						// would take more time than a normal scheduling decision so it would make sense
						// to increment the system clock by a different amount to indicate this.
						incrementSysClock(sysclock, 0, generateRandomNumber(TOTAL_DISPATCH_RANDOM) + 10000);
						fprintf(logfile, "OSS: Moving %d from Blocked queue to ready queue at the time %d:%d\n",
								blocked_pid, sysclock->sec, sysclock->nano_sec);
					}
					else
					{
						// Keeps the pids so we can put this back into the blocked queue
						blocked_pids[still_blocked_pid_cnt] = blocked_pid;
						still_blocked_pid_cnt++;
					}
				}
			}

			if (!isEmpty(highest_priority_queue))
			{
				pid_to_process = dequeue(highest_priority_queue);
				queue_id = HIGHEST_PRIORITY_QID;
				time_slice = HIGHEST_PRIORITY_QUANTUM;
			}
			else if (!isEmpty(second_priority_queue))
			{
				pid_to_process = dequeue(second_priority_queue);
				queue_id = SECOND_PRIORITY_QID;
				time_slice = SECOND_PRIORITY_QUANTUM;
			}
			else if (!isEmpty(third_priority_queue))
			{
				pid_to_process = dequeue(third_priority_queue);
				queue_id = THIRD_PRIORITY_QID;
				time_slice = THIRD_PRIORITY_QUANTUM;
			}
			else
			{
				printf("No process to process. Terminating the program\n");
				int pid_to_process = 0;
				terminate();
			}

			proc_ctrl_blck proc_ctrl_block = proc_ctrl_table[pid_to_process];
			printf("proc_ctrl_block id %d\n", proc_ctrl_block.local_sim_pid);
			proc_ctrl_block.is_blocked = FALSE;

			fprintf(logfile, "OSS: Dispatching process with PID %d from queue %d at time %d:%d\n",
					pid_to_process, queue_id, sysclock->sec, sysclock->nano_sec);
			line_number++;

			fprintf(logfile, "OSS: total time this dispatch was %d nanoseconds\n", total_dispatch_time);
			line_number++;
			fflush(logfile);

			// Sending a message to the process using specific pid and how much of a time slice it has to run
			message msg;
			msg.mesg_type = pid_to_process;
			msg.time_slice = time_slice;

			printf("OSS: total time this dispatch was %d nanoseconds\n", total_dispatch_time);
			msgsnd(user_msgqueue_id, &msg, (sizeof(message)-sizeof(long)), 0);

			// receive from parent's
			msgrcv(oss_msgqueue_id, &msg, sizeof(message), PARENT_QUEUE_ADDRESS, 0);

			printf("OSS: Receiving that process with PID %d ran for %d nanoseconds\n",
					pid_to_process, msg.time_slice);
			fprintf(logfile, "OSS: Receiving that process with PID %d ran for %d nanoseconds\n",
					pid_to_process, msg.time_slice);
			line_number++;
			fflush(logfile);

			incrementSysClock(sysclock, 0, msg.time_slice);

			// Updating statistcis in process control block
			int cpu_usage_sec = 0;
			int cpu_usage_ns = msg.time_slice;

			if (cpu_usage_ns >= ONE_SEC_IN_NANO)
			{
				cpu_usage_sec = cpu_usage_ns / ONE_SEC_IN_NANO;
				cpu_usage_ns -= cpu_usage_sec * ONE_SEC_IN_NANO;
			}

			proc_ctrl_block.total_last_burst_time = msg.time_slice;
			proc_ctrl_block.total_used_cpu_time_sec += cpu_usage_sec;
			proc_ctrl_block.total_used_cpu_time_ns  += cpu_usage_ns;

			if (msg.is_terminated)
			{
				// If done, enqueue id back to pid queuce
				printf("OSS: pid %d is terminated\n", pid_to_process);
				enqueue(available_pids, pid_to_process);
			}
			else
			{
				if (msg.is_interrupted)
				{
					proc_ctrl_block.is_blocked = TRUE;
					proc_ctrl_block.event_happen_s  = sysclock->sec + generateRandomNumber(EVENT_HAPPEN_SEC_RANDOM);
					proc_ctrl_block.event_happen_ns = sysclock->nano_sec + generateRandomNumber(EVENT_HAPPEN_NS_RANDOM);
					proc_ctrl_block.proc_priority = 0;
					// Enqueue to blocked_queue
					enqueue(blocked_queue, pid_to_process);
				}
				else
				{
					// if current priority is less than 2, increase the priority by 1
					proc_ctrl_block.proc_priority += proc_ctrl_block.proc_priority < 2 ? 1 : 0;

					if (proc_ctrl_block.proc_priority == SECOND_PRIORITY_QID)
					{
						enqueue(second_priority_queue, pid_to_process);
					}
					else if (proc_ctrl_block.proc_priority == THIRD_PRIORITY_QID)
					{
						enqueue(second_priority_queue, pid_to_process);
					}
				}

			}

		}

		// Advance the logical clock by 1.xx seconds in each iteration of the loop where xx is the number of nanoseconds.
		incrementSysClock(sysclock, 1, generateRandomNumber(LOOP_ADVANCE_RANDOM));
	}

	// Deallocate semaphore, shared memory and terminate.
	terminate();

	return EXIT_SUCCESS;
}