// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "console.h"
#include "synch.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

static void ConvertIntToHex (unsigned v, Console *console)
{
   unsigned x;
   if (v == 0) return;
   ConvertIntToHex (v/16, console);
   x = v % 16;
   if (x < 10) {
      writeDone->P() ;
      console->PutChar('0'+x);
   }
   else {
      writeDone->P() ;
      console->PutChar('a'+x-10);
   }
}
void setChildContext(int arg){
  if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
  threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {   // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
  currentThread->space->RestoreStateOnSwitch();
    }
    machine -> Run();
#endif
}
void
ExceptionHandler(ExceptionType which)
{
//printf("!----------entered ExceptionHandler----------!");
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;
    unsigned printvalus;        // Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);;

    if ((which == SyscallException) && (type == SYScall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == SYScall_PrintInt)) {
       printval = machine->ReadRegister(4);
       if (printval == 0) {
	  writeDone->P() ;
          console->PutChar('0');
       }
       else {
          if (printval < 0) {
	     writeDone->P() ;
             console->PutChar('-');
             printval = -printval;
          }
          tempval = printval;
          exp=1;
          while (tempval != 0) {
             tempval = tempval/10;
             exp = exp*10;
          }
          exp = exp/10;
          while (exp > 0) {
	     writeDone->P() ;
             console->PutChar('0'+(printval/exp));
             printval = printval % exp;
             exp = exp/10;
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_PrintChar)) {
	writeDone->P() ;
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_PrintString)) {
       vaddr = machine->ReadRegister(4);
       machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
	         writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          machine->ReadMem(vaddr, 1, &memval);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_PrintIntHex)) {
       printvalus = (unsigned)machine->ReadRegister(4);
       writeDone->P() ;
       console->PutChar('0');
       writeDone->P() ;
       console->PutChar('x');
       if (printvalus == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          ConvertIntToHex (printvalus, console);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_GetReg)) {	
   // printf("!----------entered getReg----------!");
    	//--------------------------------IMPLEMENTED SYSCALL_GETREG ----------------------------------------------
       int target_register = machine->ReadRegister(4);
       int contents_in_target_register = machine->ReadRegister(target_register);
       machine->WriteRegister(2,contents_in_target_register);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    
    else if ((which == SyscallException) && (type == SYScall_GetPA)) {	
    	//--------------------------------IMPLEMENTED SYSCALL_GETPA ----------------------------------------------
       unsigned int virtual_address = machine->ReadRegister(4);
       unsigned int virtual_page_num = (unsigned) virtual_address / PageSize;
       int physical_address;
       if(virtual_page_num > machine->pageTableSize || !((machine->NachOSpageTable)[virtual_page_num].valid) || ((machine->NachOSpageTable)[virtual_page_num].physicalPage) > NumPhysPages){
       	machine->WriteRegister(2,-1);
       }
       else{
       	machine->Translate((int)virtual_address, &physical_address, sizeof(virtual_address)/sizeof(int), false);
       	machine->WriteRegister(2,physical_address);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    else if ((which == SyscallException) && (type == SYScall_GetPID)) {	
       int pid = currentThread->get_pid();
       machine->WriteRegister(2,pid);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    else if ((which == SyscallException) && (type == SYScall_GetPPID)) {	
       int ppid = currentThread->get_ppid();
       machine->WriteRegister(2,ppid);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_Time)) {
       //printf("got here\n");	
       machine->WriteRegister(2,stats->totalTicks);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_Yield)){
    	  // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    	  currentThread->YieldCPU();
    }
    else if ((which == SyscallException) && (type == SYScall_Sleep)){
        //printf("inside sleep\n");
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
      
        int time_in_sleep = machine->ReadRegister(4);
        if (time_in_sleep==0){
          currentThread->YieldCPU();   

        }
        else{
          IntStatus oldLevel = interrupt->SetLevel(IntOff);
          sleepList->SortedInsert(currentThread, time_in_sleep+stats->totalTicks);
          currentThread->PutThreadToSleep();
          (void) interrupt->SetLevel(oldLevel);   
        }

    }
    else if ((which == SyscallException) && (type == SYScall_NumInstr)){
        machine->WriteRegister(2,currentThread->instr_counter);
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_Fork)){
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        NachOSThread *child_thread = new NachOSThread("child");
        int child_pid = child_thread->get_pid();
        currentThread->childlist->SortedInsert(child_thread, child_pid);
        //printf("%d--------------------\n", child_pid);
        ProcessAddrSpace *child_space = new ProcessAddrSpace();
        //---------------&
        //machine->WriteRegister(2,0);  
        child_thread->space = child_space;
        child_thread->SaveUserState();
        child_thread->userRegisters[2] = 0;
        child_thread->ThreadFork(setChildContext,0);
        machine->WriteRegister(2,child_pid);
    }
    else if ((which == SyscallException) && (type == SYScall_Join)){
        int child_pid = machine->ReadRegister(4);
        bool found = FALSE;
        ListElement *curr = currentThread->childlist->first;
        IntStatus oldLevel = interrupt->SetLevel(IntOff);
        while(!found){
          if(curr!=NULL){
            if (curr->key == child_pid){
              found = true;
              bool in_completed= false;
              ListElement *temp = currentThread->completed_childs->first;
              while(!in_completed){
                if(temp!=NULL){
                  if (temp->key == child_pid){
                    in_completed = true;
                    machine->WriteRegister(2,(int)temp->item);
                  }
                  else{
                    temp = temp->next;
                  }
                }
                else{
                  break;
                }
              }
              if(!in_completed){
                NachOSThread*ex = (NachOSThread*) curr->item;
                ex->check_par = 1;
                currentThread->PutThreadToSleep();
              }
            }
            else{
              curr = curr->next;
            }
          }
          else{
            break;
          }
        }
        if (!found){
          machine->WriteRegister(2,-1);
        }
        //printf("\n------------chec : %d------------\n",currentThread->check_par );
        //printf("in join--------------------------------\n");
        (void) interrupt->SetLevel(oldLevel);
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_Exit)){
        //printf("in exit-------------------\n");
        void *exit_code = (void*) machine->ReadRegister(4);
        //printf("\n---------exit-code : %d\n",(int)exit_code);
        int curr_pid = currentThread->get_pid();
        if(currentThread->parent){
          currentThread->parent->completed_childs->SortedInsert(exit_code,curr_pid);
        }
        currentThread->FinishThread();
    }
    else if ((which == SyscallException) && (type == SYScall_Exec)){
      //from line 130
      vaddr = machine->ReadRegister(4);
      char str[200];
      int i=0;
      machine->ReadMem(vaddr, 1, &memval);
      while ((*(char*)&memval) != '\0') {
        str[i]=(char)memval;
        vaddr++;
        i++;
        machine->ReadMem(vaddr, 1, &memval);
      }
      str[i]='\0';

      //pasted from progtest.cc StartUserProcess()
      OpenFile *executable = fileSystem->Open(str);
      ProcessAddrSpace *space;

      if (executable == NULL) {
        printf("Unable to open file %s\n", str);
        return;
      }
      space = new ProcessAddrSpace(executable);    
      currentThread->space = space;

      delete executable;      // close file

      space->InitUserCPURegisters();    // set the initial register values
      space->RestoreStateOnSwitch();    // load page table register

      machine->Run();     // jump to the user progam
      ASSERT(FALSE);      // machine->Run never returns;
      // the address space exits
      // by doing the syscall "exit"
    }
    else {
    	printf("Unexpected user mode exception %d %d\n", which, type);
    	ASSERT(FALSE);
    }
}
