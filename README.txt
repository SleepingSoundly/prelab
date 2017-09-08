Description

You are going to demonstrate your understanding of the semantics of an existing threads library (pthreads, to be exact) and how to use it.

Download the tarball (.tar.gz file) attached to this assignment and uncompress it with the command:

tar zxvf prelab.tar.gz
Contained within, you will find a source (.c) file and a Makefile. (Although we're not asking you to modify the Makefile now, this would be a good opportunity to take a look and make sure you understand how it works, because you may need to modify Makefiles for later projects.) You can compile the source file with the command make and clean up the compiler output with make clean. The program should compile and run correctly on any Red Hat Enterprise Linux (RHEL) system or Ubuntu 12 +.

The source code provided is a multi-threaded consumer-producer program. That means producer threads add items (in this case, a character) to a queue, and consumer threads remove the items from the queue and do something with them (in this case, print the character to standard output). The main thread spawns a consumer thread and a producer thread, then the producer thread also spawns a consumer thread. So in total there are 1 producer thread and 2 consumer threads (plus the main thread). You will find that it compiles correctly, but when you run the program there are in fact several bugs.

For this pre-lab, you will be asked to do two things: First, find and fix the five (5) bugs that have been placed in the source code. (Some of these bugs will appear at runtime, but others may only be apparent by code inspection.) Putting comments near the bugs you found will also help us to grade your pre-lab. Second, answer the questions below (short answers, one or two sentences each). It may be convenient to make a copy of the original source file before you modify it, since the questions below include line numbers.

To find information about pthread functions, you may use the command "man [func name]" (e.g. man pthread_create).

Please note that all the bugs in the program pertain to the pthreads threading and mutual exclusion libraries in some way. There are not bugs pertaining to program semantics (except for the semantics of pthreads and mutexes), and the intended bugs are all things that are definitely wrong regardless of any common sense interpretation of the program's intended behavior. Also, all the string constants should be correct. If there is a mistake in a string constant, then that's my mistake and not one of the intended bugs. (You can still point it out, though, so we can fix it for next time.) These guidelines are meant to help, but if you find something that you're unsure about, you can ask the TA.

Collaboration

Students are not allowed to collaborate on this assignment. Each student is expected to find the 5 bugs, fix them, and answer the questions on their own. (You may use reference material, however, to help you understand how pthreads work and the semantics of the function calls.)

Questions

The main function contains calls to exit() (line 66) and pthread_exit() (line 80). How will the effect of these two calls differ when they are executed?
	The regular exit will remove all the resources that were created by main. This includes any shared memory between processes, and is generally not advisable
	when you're working with threads. 
	Pthread_exit() leaves process-shared resources (eg mutexes), and after the last thread in a process terminates, it will call regular exit with a 0 status,
	so that the regular exit routines can be processed. 



The main function calls pthread_join() (line 77) with the parameter thread_return. Where does the value stored in thread_return come from when the consumer_thread is joined?
	The value stored in thread_return is assigned by the exit function to a field within the p_thread type structure for the thread we're exiting from. That structure is keyed to a PID, 
	so we will be able to find it that way (or if the process asking for the join is the parent by using the variable), and pull out the return value. 


Where does the value stored in thread_return come from if the joined thread terminated by calling pthread_exit instead of finishing normally?
On the same call to pthread_join() (line 77), what will it do if the thread being joined (consumer_thread, in this case) finishes before the main thread reaches the that line of code (line 77)?
	Because the value is placed in a thread structure created elsewhere, that structure will still contain the return value of that thread. Therefor, when the code reaches the end,
	if consumer has already finished, that value will be sitting within the structure waiting. 


In this program, the main thread calls pthread_join() on the threads it created. Could a different thread call pthread_join() on those threads instead? Could a thread call pthread_join() on the main thread (assuming it knew the main thread's thread ID - i.e. pthread_t)?
	Yes, it could. The function waits on the finishing of a thread, which can be identified by a thread created in the context of that function, or a pointer to
	a thread from another process. Either way, when that thread is determined to be finished, the pthread_exit routine will place a return value in the pthread_join call. This is possible with any thread, including main. 



The consumer_routine function calls sched_yield() (line 180) when there are no items in the queue. Why does it call sched_yield() instead of just continuing to check the queue for an item until one arrives?
	Sched_yield() allows other threads to take CPU time. If we just kept attempting to grab the lock, then one thread could feesibly retain all the CPU uptime 
	and the other threads would never process. 



Deliverables

Answer the questions in the text box, or in an attached file (PDF or MS-Word)
Submit your modified source file (with the 5 bugs fixed) by attaching it to the assignment
You do not need to turn in the Makefile, but your modified source code should compile correctly with the original, unmodified Makefile (in future assignments, you will need to turn in Makefiles, but this one is an exception)