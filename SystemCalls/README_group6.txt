
SYScall_GetReg :
	->Register passed as an argument is read from register 4 using ReadRegister
	->Content of the register is read and written in register 2 using WriteRegister(return value)
	->Program Counter is incremented after writing to the register

SYScall_GetPA :
	->Virtual address passed as an argument is read from register 4
	->Virtual Page number of the corresponding virtual address is calculated using (virtual_address / PageSize)
	-> -1 is returned if 
		--virtual_page_num > machine->pageTableSize
		--pagetable entry doesn't have a valid field
		--physical page number is larger than the number of physical pages
	->Corresponding Physical Address calculated from "Translate method" is returned  
	->Program Counter is incremented after writing to the register

SYScall_GetPID :
	->get_pid() method is created in 'thread.h' which returns the pid of the calling thread
	->The pid value is assigned to the thread inside the constructor using the thread_count till now (pid = thread_count)
	->Program Counter is incremented after writing to the register

SYScall_GetPPID :
	->get_ppid() method is created in 'thread.h' which returns the ppid(Parent pid) of the calling thread
	->The ppid value is assigned to the thread inside the constructor using the pid of the parent thread (ppid = currentThread->get_pid())
	->Program Counter is incremented after writing to the register

SYScall_Time :
	->Returns the value of the totalTicks using 'stats->totalTicks'
	->Program Counter is incremented after writing to the register

SYScall_Yield :
	->The YieldCPU() method defined in NachOSThread class of the current thread is called
	->Program Counter is incremented after returning from the YieldCPU() method

SYScall_Sleep :
	->Time to be in sleep passed as an argument is read from register 4
	->A list is created in the 'system.cc' to keep track of all threads in the Sleep Status
	->If the read time is '0' then the yieldCPU() method is called
	->Else the thread is inserted using SortedInsert() with key as sleeptime + totalTicks
	->The interrupts are set off while inserting the thread
	->For each clock tick, the TimerInterruptHandler() removes the threads after their sleeptime by adding them in the Ready Queue using ThreadIsReadyToRun() method

SYScall_NumInstr :
	->Returns the total number of instructions executed by the process till now
	->An Instruction counter is assigned '1' in 'thread.cc' every time a process is created
	->The Instruction counter is incremented in the 'mipssim.cc' 
	->Program Counter is incremented after writing to the register

SYScall_Exec :
	->The executable string is obtained by the similar code used in syscall_PrintString 
	->The OpenFile instance is created by using open() method declared in 'filesys'
	->A new space is created using ProcessAddressSpace() methos for the given executable
	->This new space is assigned to the space pointer of the currentThread and the executable file is deleted after the assignment
	->The registers of the space created are initialized and the state is restored for the process by using the same methods as they are in StartUserProcess()
	->The new process created is made to run using the Run() method as used in the StartUserProcess()

SYScall_Fork :
	->New Child is created using the NachOSThread constructor and is inserted in the childlist of the current thread
	->New ProcessAddressSpace() constructor is created which copies the registers of the parent 
	->New address space is created for this child and is assigned to the space pointer of this child
	->ThreadFork() is called which uses setChildContext context of the child is prepared and sleeps or keeps in the ready queue for future
	->pid of the child is returned to the parent by writing in register 2 and 0 is written in the register 2 of the child
	
SYScall_Exit :
	->The thread to be exited is inserted in the completed_childs list of the parent thread(which will help during the join sys_call)
	->FinishThread() method is called currentThread is assigned to the threadToBeDestroyed, which is deleted when the scheduler checks for the next thread to run
	->In the FinishThread(), if the status of the parent is Sleep, then the parent is pushed into the ready queue using ThreadIsReadyToRun() (status of the parent to be verified, is assigned in Join sys_call)
	->thread_count is decremented by 1, and if there are no more threads to be processed, i.e.,thread_count == 0, Halt() interrupt is called

SYScall_Join :
	->pid of the child to be processed is passed as an argument, read from register 4 using ReadRegister
	->Search for the given pid in the childList of the current thread, if the pid is not found then return -1 by writing to the register 2 
	->Else if the pid is found then we need to check for the exit_code of the given pid by searching in the completed_childs list 
	->If the pid is found in the completed list then the child with the given pid is already exited and we return the exit_code of the child 
	->Else the parent is put to sleep by PutThreadToSleep() and the status of the thread is put to sleep
	->Program Counter is incremented after writing to the register






-----------------------------------------------------------------------------------------------
Results of the programs in Test---------------------------------------------------------------- 
-----------------------------------------------------------------------------------------------

----------------------------------------printtest.c---------------------------------------------
hello world
Executed 28 instructions.
Machine halting!

Ticks: total 4155, idle 3674, system 430, user 51
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 37
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

----------------------------------------halt.c--------------------------------------------------
Machine halting!

Ticks: total 22, idle 0, system 10, user 12
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

----------------------------------------testregPA.c---------------------------------------------

Starting physical address of array: 1616
Physical address of array[50]: 1816
Current physical address of stack top: 1600
We are currently at PC: 0xf4
Total sum: 4950
Machine halting!

Ticks: total 22430, idle 16194, system 2280, user 3956
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 165
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

----------------------------------------vectorsum.c---------------------------------------------

Total sum: 4950
Executed instruction count: 3858
Machine halting!

Ticks: total 9616, idle 4762, system 980, user 3874
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 48
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

----------------------------------------testexec.c----------------------------------------------

Before calling Exec.
Total sum: 4950
Executed instruction count: 3879
Machine halting!

Ticks: total 11866, idle 6752, system 1220, user 3894
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 69
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

---------------------------------------testyield.c----------------------------------------------

***** *t htrheraeda d1  2l oloopoepde d0  0t itmiemse.s
.*
**** *t htrheraeda d1  2l oloopoepde d1  1t itmiemse.s
.*
**** *t htrheraeda d1  2l oloopoepde d2  2t itmiemse.s
.*
**** *t htrheraeda d1  2l oloopoepde d3  3t itmiemse.s
.*
**** *t htrheraeda d1  2l oloopoepde d4  4t itmiemse.s
.B
efore join.
After join.
Machine halting!

Ticks: total 34958, idle 30616, system 3700, user 642
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 314
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

----------------------------------------forkjoin.c----------------------------------------------

Parent PID: 1
PCahrielndt  PaIfDt:e r2 Cfhoirlkd 'wsa iptairnegn tf oPrI Dc:h i1l
d:C h2i
ld called sleep at time: 9039
Child returned from sleep at time: 9229
Child executed 123 instructions.
Parent executed 84 instructions.
Machine halting!

Ticks: total 25045, idle 22220, system 2570, user 255
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 225
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

-----------------------------------------matmult.c-----------------------------------------------

Machine halting!

Ticks: total 664840, idle 0, system 66490, user 598350
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

