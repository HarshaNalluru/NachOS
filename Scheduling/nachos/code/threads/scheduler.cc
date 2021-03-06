// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextThreadToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// NachOSscheduler::NachOSscheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

NachOSscheduler::NachOSscheduler()
{ 
    readyThreadList = new List; 
    updatedList = new List;
} 

//----------------------------------------------------------------------
// NachOSscheduler::~NachOSscheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

NachOSscheduler::~NachOSscheduler()
{ 
    delete readyThreadList; 
} 

//----------------------------------------------------------------------
// NachOSscheduler::ThreadIsReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
NachOSscheduler::ThreadIsReadyToRun (NachOSThread *thread)
{
    DEBUG('t', "Putting thread %s with PID %d on ready list.\n", thread->getName(), thread->GetPID());
    if (thread->running){
        thread->numBursts++;
        thread->totalBurstTime = thread->totalBurstTime + (stats->totalTicks - thread->burstStartTime);
        g_avgcpuburst += stats->totalTicks - thread->burstStartTime;
      if (g_mincpuburst > stats->totalTicks - thread->burstStartTime ) {
            g_mincpuburst = stats->totalTicks - thread->burstStartTime ;
        }
        if (g_maxcpuburst < stats->totalTicks - thread->burstStartTime) {
            g_maxcpuburst = stats->totalTicks - thread->burstStartTime ;
        }
        thread->cpuDelayTime = thread->cpuDelayTime + (stats->totalTicks - thread->burstStartTime);
        g_varcpuburst += abs(thread->burstEstimateTime-(stats->totalTicks - thread->burstStartTime));
        thread->burstEstimateTime = 0.5*thread->burstEstimateTime + 0.5*(stats->totalTicks - thread->burstStartTime);

    // if (thread->GetPID() !=0 && (stats->totalTicks - thread->burstStartTime)!=0)
    // {
    //   g_avgcpuburst += stats->totalTicks - thread->burstStartTime;
    //   if (g_mincpuburst > stats->totalTicks - thread->burstStartTime ) {
    //         g_mincpuburst = stats->totalTicks - thread->burstStartTime ;
    //     }
    //     if (g_maxcpuburst < stats->totalTicks - thread->burstStartTime) {
    //         g_maxcpuburst = stats->totalTicks - thread->burstStartTime ;
    //     }
    //     thread->cpuDelayTime = thread->cpuDelayTime + (stats->totalTicks - thread->burstStartTime);
    //     thread->burstEstimateTime = 0.5*thread->burstEstimateTime + 0.5*(stats->totalTicks - thread->burstStartTime);
                  
    // }
        
    //      g_varcpuburst += abs(thread->burstEstimateTime-(stats->totalTicks - thread->burstStartTime));


    }
    thread->setStatus(READY);
    thread->waitStartTime = stats->totalTicks;
    if (algorithm==2){
        readyThreadList->SortedInsert(thread,thread->burstEstimateTime);
    }
    else{
        readyThreadList->Append((void *)thread);    
    }
}

//----------------------------------------------------------------------
// NachOSscheduler::FindNextThreadToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

NachOSThread *
NachOSscheduler::FindNextThreadToRun ()
{
    return (NachOSThread *)readyThreadList->Remove();
}

//----------------------------------------------------------------------
// NachOSscheduler::Schedule
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
NachOSscheduler::Schedule (NachOSThread *nextThread)
{
    NachOSThread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveStateOnSwitch();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    //added------------updating statuses
    currentThread->burstStartTime = stats->totalTicks;
    currentThread->total_waitTime += stats->totalTicks - currentThread->waitStartTime;
    currentThread->running = true;

    //--added
    DEBUG('t', "Switching from thread \"%s\" with pid %d to thread \"%s\" with pid %d\n",
	  oldThread->getName(), oldThread->GetPID(), nextThread->getName(), nextThread->GetPID());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    _SWITCH(oldThread, nextThread);
    
    DEBUG('t', "Now in thread \"%s\" with pid %d\n", currentThread->getName(), currentThread->GetPID());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in NachOSThread::FinishThread()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreStateOnSwitch();
    }
#endif
}

//----------------------------------------------------------------------
// NachOSscheduler::Tail
//      This is the portion of NachOSscheduler::Schedule after _SWITCH(). This needs
//      to be executed in the startup function used in fork().
//----------------------------------------------------------------------

void
NachOSscheduler::Tail ()
{
    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in NachOSThread::FinishThread()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {         // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreStateOnSwitch();
    }
#endif
}

//----------------------------------------------------------------------
// NachOSscheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
NachOSscheduler::Print()
{
    printf("Ready list contents:\n");
    readyThreadList->Mapcar((VoidFunctionPtr) ThreadPrint);
}

void
NachOSscheduler::UpdateThreadPriorities(){
    #ifdef USER_PROGRAM
    NachOSThread * temp ;
    for (int i = 0; i < thread_index; ++i){
        if (!exitThreadArray[i]){
            temp = threadArray[i];
            if (temp->GetPID() != currentThread->GetPID()){
                temp->cpuDelayTime = (temp->cpuDelayTime)/2;
            }
            temp->priority = temp->basePriority + temp->nice + (temp->cpuDelayTime)/2;
        }
    }

    updatedList = new List();

    while(!readyThreadList->IsEmpty()){
        temp = (NachOSThread*)readyThreadList->Remove();
        updatedList->SortedInsert(temp,temp->priority);

    }
    readyThreadList = updatedList;
    #endif

}
