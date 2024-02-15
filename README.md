# OS-process_scheduling

# How to run

1. Use 'make clean' command to clean any old compiled files
2. Run 'make'
3. ./oss [-h] [-s t] [-l f]
   
**-h** flag is to get help menu
**-s t** indicate how many maximum seconds before system terminates
**-l f** specify a particular name for the log file
   
## Purpose

The goal of this homework is to learn about process scheduling inside an operating system. You will work on the specified scheduling algorithm and simulate its performance.

### Task

In this project, you will simulate the process of scheduling part of an operating system. You will implement time-based scheduling, ignoring almost every other aspect of the OS. In particular, this project will involve a main executable managing the execution of concurrent processes just like a scheduler/dispatcher does in an OS. You may us message queues or semaphores for synchronization, depending on your choice.

### Operating System Simulator

The operating system simulator, or OSS, will be your main program and serve as the master process. You will start the simulator (call the executable **oss**) as one main process who will fork multiple children at random times. The randomness will be simulated by a logical clock that will also be updated by **oss**. As we are simulating a scheduler, only one process (either **oss** or one of the children/user process) will be in running state at any given time, while all other processes wait to receive a message, or for a semaphore, for them to be scheduled.

In the beginning, **oss** will allocate shared memory for system data structures, including a process table with a process control block for each user process. The process control block is a fixed size structure and contains informationto manage the child process scheduling. Notice that since it is a simulator, you will NOT need to allocate space to save the context of child processes. However, you must allocate space for scheduling-related items such as total CPU time used, total time in the system, time used during the last burst, your local simulated pid, and process priority if any. The process control block residees in shared memory and is accesible to the children. Since we are limiting ourselves to 20 processes, you should allocate space for up to 18 process control blocks. Also create a bit vector, local to **oss**, that will help you keep track of the process control blocks (or process IDs) that are currently in use.

**oss** simulates the passing of time by using a simulated system clock. The clock is stored in two unsigned integers in shared memory, one which stores seconds the other nanoseconds. **oss** will create user processes at random intervals, say every second on an average of teh simulated clock. While the simulated clock (the two integers) is viewable by the child processess, it should only be advanced (changed) by **oss**, and only in strict ways as described in the rest of this document.

Note that **oss** simulates the passing of time in the system by adding time to the clock and it is the only process that would change the clock. At various times, a user process will simulate doing some work by running. When this happens, after it cededs control back to **oss**, **oss** would update the simulated system clock by appropriate amount. This is achieved by generating a random number within the child that will be sent back to **oss** through either shared memory or a message queue. In the same fashion, if **oss** does something that should take some time if it was a real operating system, it should increment the clock by a small amount to indicate the time it spent.

There will be two types of user processes: normal user processes that run on a normal priority and realtime processes that are time sensitive. While they can be simulated by the same executable, the type will be randomly determined at launch process.

**oss** will create user processes at random intervals (of simulated time), so you will have two constants; let us call them **maxTimeBetweenNewProcsNS** and **maxTimeBetweenNewProcsSecs**. oss will launch a new user process based on a random time interval from 0 to those constants. It generates a new process by allocating and initializing the process control block for the process and then, **forks** the process. The child process will **execl** the binary. I would suggest setting these constants initially to spawn a new process about every 1-3 seconds, but you can experiment with this later to keep the system busy. There should be a constant representing the percentage of time a process is launched as a normal user process or a real-time process. While this constant is specified by you, it should be heavily weighted to generate mainly user processes.

**oss** will be in control of all concurrency. In the beginning, there will be no processes in the system but it will have a time in the future where it will launch a process. If there are no processes currently ready to run in the system, it should increment the clock until it is time when it should launch a process. It should then set up that process, generate a new time where it will launch a process and then using a message queue, schedule a process to run by sending it a message. It should then wait for a message back from that process that it has finished its task. If your process table is already full when you go to generate a process, just skip that generation, but do determine another time in the future to try and generate a new process. Make a note of the process table being full in the log file.

Advance the logical clock by 1.xx seconds in each iteration of the loop where xx is the number of nanoseconds. xx will be a random number in the interval [0, 1000] to simulate some overhead activity for each iteration.

A new process should be generated every 1 second, on an average. So, it should generate a random number between 0 and 2 assigning it to time to create new process. If your clock has passed this time since the creation of the last process, generate a new process (and **execl** it). The time can be generated by generating two random numbers in the interval [0,2) for sec and [0,MAXINT) for ns.

**oss** acts as the scheduler and so will schedule a process by sending it a message using a message queue or semaphore. When initially started, there will be no processses in the system but it will have a time in the future when it will launch a process, generated by the random process mentioned previously. If there are no processes currently ready to run in the system, it should increment the clock until it is the time when it should launch a process. It should then set up that process, generate a new time when it will create a new process and then using a message queue or semaphore, schedule a process to run by sending it a message. It should then wait for a message back from that process that is has finished its task. If your process table is already full when you go to generate a process, just skip that generation, but do determine another time in the future to try and generate a new process.

#### Scheduling Algorithm

Assuming you have more than one process in your simulated system, **oss** will select a process to run and schedule it for execution. It will select the process by using a scheduling algorithm with the following features:

Implement a multi-level feedback queue. There are three scheduling queues, each having an associated time quantam. The base time quantam is determined by you as a constant, let us say something like 10 milliseconds, but certainly could be experimented with. The highest priority queue has this base time quantam as the amount of time it would schedule a child process if it scheduled it out of that queue. The second highest priority queue would have twice that, the third highest priority queue would have four times the base queue and so on, as per a normal multi-level feedback queue. If a process finishes using its entire timeslice, it should be moved to a queue one lower in priority. If a process comes out of a blocked queue, it should go to the highest priority queue.

In addition to the naive multi-level feedback queue, you should implement some form of aging to prevent processes from starving. This should be based on some function of a processes wait time compared to how much time it has spent on the CPU.

When **oss** has to pick a process to schedule, it will look for the higheest priority occupied queue and schedule the process at the head of this queue. The process will be dispatched by sending the process a message queue indicating how much of a timeslice it has to run. Note this scheduling itself takes time, so before launching the process the **oss** should increment the clock for the amount of work that it did, let us say from 100 to 100000 nanoseconds.

#### User Processes

All user processes simulate the system by performing some work that will take some random time. The user processess will wait on receiving a message giving them a time slice and then it will simulate running. They do not do any actual work but instead send a message to oss saying how much time they used and if they had to use i/o or had to terminate.

You should **#define** a constant in your system to indicate the probability that a process will terminate when scheduled. This probability should be fairly small to start. Processes will then, using a random number, use this to determine if they should terminate. Note that when creating this random number you must be careful that the seed you use is different for all processes. If it would terminate, it would of course use some random amount of its timeslice before terminating. It should indicate to **oss** that it has decided to terminate and also how much of its timeslice it used, so that **oss** can increment the clock by the appropriate amount.

Once it has decided that it will not terminate, then we have to determine if it will use its entire timeslice or it it will get blocked on an event. This should be determined by a random number, but CPU-bound processes should be very unlikely to get interrupted (so they will usually use up their entire timeslice, without getting interrupted). On the other hand, I/O-bound processes should more likely than not get interrupted before finishing their timeslices. If a process determines that it would use up its timeslice, this information should be conveyed to **oss** when it cedes control to **oss**. Otherwise, the process would have to indicate to **oss** that it should be blocked, as well as the amount of its assigned quantam that it used up before being blocked. That process should then be put in a blocked queue waiting for an event that will happen in *r.s* seconds where *r* and *s* are random numbers with range [0,5] and [0,1000] respectively. As this could happen for multiple processes, this will require a blocked queue, checked by **oss** every time it makes a decision on scheduling to see if it should wake up these processes and put them back in the ready queue. Note that the simulated work of moving a process from a blocked queue to a ready queue would take more time than a normal scheduling decision so it would make sense to increment the system clock by a different amount to indicate this.
