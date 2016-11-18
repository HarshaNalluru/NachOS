// addrspace.cc 
//  Routines to manage address spaces (executing user programs).
//
//  In order to run a user program, you must:
//
//  1. link with the -N -T 0 option 
//  2. run coff2noff to convert the object file to Nachos format
//      (Nachos object code format is essentially just a simpler
//      version of the UNIX executable object code format)
//  3. load the NOFF file into the Nachos file system
//      (if you haven't implemented the file system yet, you
//      don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#define INT_MAX 100000000;
//----------------------------------------------------------------------
// SwapHeader
//  Do little endian to big endian conversion on the bytes in the 
//  object file header, in case the file was generated on a little
//  endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// ProcessAddrSpace::ProcessAddrSpace
//  Create an address space to run a user program.
//  Load the program from a file "executable", and set everything
//  up so that we can start executing user instructions.
//
//  Assumes that the object code file is in NOFF format.
//
//  First, set up the translation from program memory to physical 
//  memory.  For now, this is really simple (1:1), since we are
//  only uniprogramming, and we have a single unsegmented page table
//
//  "executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

ProcessAddrSpace::ProcessAddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
            + UserStackSize;    // we need to increase the size
                        // to leave room for the stack
    numPagesInVM = divRoundUp(size, PageSize);
    size = numPagesInVM * PageSize;

    supportspace  = new char[size];



    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
                    numPagesInVM, size);
// first, set up the translation 
    NachOSpageTable = new TranslationEntry[numPagesInVM];
    for (i = 0; i < numPagesInVM; i++) {
    int tempphyspage = GetPhysPage(-1);
    NachOSpageTable[i].virtualPage = i;
    NachOSpageTable[i].physicalPage = tempphyspage;
    NachOSpageTable[i].valid = TRUE;
    NachOSpageTable[i].use = FALSE;
    NachOSpageTable[i].dirty = FALSE;
    NachOSpageTable[i].readOnly = FALSE;
    NachOSpageTable[i].shared = FALSE;
    NachOSpageTable[i].support= FALSE;
      // if the code segment was entirely on 
                    // a separate page, we could set its 
                    // pages to be read-only
    }
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
        ASSERT(pageRepAlgo == 0); 

    bzero(&machine->mainMemory[numPagesAllocated*PageSize], size);
 
    //numPagesAllocated += numPagesInVM;

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
         noffH.code.virtualAddr, noffH.code.size);
        vpn = noffH.code.virtualAddr/PageSize;
        offset = noffH.code.virtualAddr%PageSize;
        entry = &NachOSpageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
         noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
         noffH.initData.virtualAddr, noffH.initData.size);
        vpn = noffH.initData.virtualAddr/PageSize;
        offset = noffH.initData.virtualAddr%PageSize;
        entry = &NachOSpageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
         noffH.initData.size, noffH.initData.inFileAddr);
    }

    
    //---------------------- Ikkadi varaku comment cheyyandi ------------------------------------------------

}
ProcessAddrSpace::ProcessAddrSpace(char *filename)
{
    ASSERT(pageRepAlgo > 0);
    NoffHeader noffH;
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;


    executablename = filename;
    openExecutable = fileSystem->Open(executablename);
    if (openExecutable == NULL) {
    printf("Unable to open file %s\n", executablename);
    
    }
    openExecutable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
   

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
            + UserStackSize;    // we need to increase the size
                        // to leave room for the stack
    numPagesInVM = divRoundUp(size, PageSize);
    size = numPagesInVM * PageSize;


    supportspace  = new char[size];

    /*ASSERT(numPagesInVM+numPagesAllocated <= NumPhysPages);     // check we're not trying
                                        // to run anything too big --
                                        // at least until we have
                                        // virtual memory*/

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
                    numPagesInVM, size);
// first, set up the translation 
    NachOSpageTable = new TranslationEntry[numPagesInVM];
    for (i = 0; i < numPagesInVM; i++) {
    NachOSpageTable[i].virtualPage = i;
    NachOSpageTable[i].physicalPage = -1;
    NachOSpageTable[i].valid = FALSE;
    NachOSpageTable[i].use = FALSE;
    NachOSpageTable[i].dirty = FALSE;
    NachOSpageTable[i].readOnly = FALSE;
    NachOSpageTable[i].shared = FALSE;
    NachOSpageTable[i].support=FALSE;
      // if the code segment was entirely on 
                    // a separate page, we could set its 
                    // pages to be read-only
    }

}



//----------------------------------------------------------------------
// ProcessAddrSpace::ProcessAddrSpace (ProcessAddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

ProcessAddrSpace::ProcessAddrSpace(ProcessAddrSpace *parentSpace , int child_pid , void * child_thread)
{
    if(pageRepAlgo > 0)
    {
        executablename = parentSpace->executablename;
        openExecutable = fileSystem->Open(executablename);
        if (openExecutable == NULL) {
        printf("Unable to open file %s\n", executablename);
       
        }
    }


    numPagesInVM = parentSpace->GetNumPages();
    executablename = parentSpace->executablename;
    unsigned i, size = numPagesInVM * PageSize;

    supportspace  = new char[size];

    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    NachOSpageTable = new TranslationEntry[numPagesInVM];
    
    for (i = 0; i < numPagesInVM; i++) {
        NachOSpageTable[i].virtualPage = i;
        if (parentPageTable[i].shared == TRUE || parentPageTable[i].valid == FALSE){

            if (parentPageTable[i].shared==TRUE){
                NachOSpageTable[i].physicalPage = parentPageTable[i].physicalPage;
            }
            else
                NachOSpageTable[i].physicalPage = -1;
            
        }
        else{

            NachOSpageTable[i].physicalPage = GetPhysPage(parentPageTable[i].physicalPage);
            //IntStatus oldLevel = interrupt->SetLevel(IntOff);
            pgPID[NachOSpageTable[i].physicalPage] = child_pid; 
            pgVPN[NachOSpageTable[i].physicalPage] = i; 
            pgOwner[NachOSpageTable[i].physicalPage] = (NachOSThread *)child_thread; 


            FIFO[NachOSpageTable[i].physicalPage] = stats->totalTicks;
            LRU[NachOSpageTable[i].physicalPage] = stats->totalTicks;
            LRUclock[NachOSpageTable[i].physicalPage] = 1;

            stats->numPageFaults = stats->numPageFaults +1;
            int k = 0;
            while(k < PageSize){
                machine->mainMemory[k+NachOSpageTable[i].physicalPage*PageSize] = machine->mainMemory[k+parentPageTable[i].physicalPage*PageSize];
                k++;
            }
            //(void) interrupt->SetLevel(oldLevel); 
            FIFO[parentPageTable[i].physicalPage] = stats->totalTicks;
            LRU[parentPageTable[i].physicalPage] = stats->totalTicks;
            LRUclock[parentPageTable[i].physicalPage] = 1;

           // numPagesAllocated = numPagesAllocated + 1 ;

            currentThread->SortedInsertInWaitQueue(1000+stats->totalTicks);
        }
        //NachOSpageTable[i].physicalPage = i+numPagesAllocated;
        NachOSpageTable[i].valid = parentPageTable[i].valid;
        NachOSpageTable[i].use = parentPageTable[i].use;
        NachOSpageTable[i].dirty = parentPageTable[i].dirty;
        NachOSpageTable[i].readOnly = parentPageTable[i].readOnly;      // if the code segment was entirely on
                                                    // a separate page, we could set its
                                                    // pages to be read-only
        NachOSpageTable[i].shared = parentPageTable[i].shared;
        NachOSpageTable[i].support = parentPageTable[i].support;    
    }
      for(i = 0 ; i < size ; i++)
    {
        supportspace[i] = parentSpace->supportspace[i];
    }

}

//----------------------------------------------------------------------
// ProcessAddrSpace::~ProcessAddrSpace
//  Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

ProcessAddrSpace::~ProcessAddrSpace()
{
   delete NachOSpageTable;
}


//----------------------------------------------------------------------
// ProcessAddrSpace::InitUserCPURegisters
//  Set the initial values for the user-level register set.
//
//  We write these directly into the "machine" registers, so
//  that we can immediately jump to user code.  Note that these
//  will be saved/restored into the currentThread->userRegisters
//  when this thread is context switched out.
//----------------------------------------------------------------------

void
ProcessAddrSpace::InitUserCPURegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);   

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPagesInVM * PageSize - 16);
    
}

//----------------------------------------------------------------------
// ProcessAddrSpace::SaveStateOnSwitch
//  On a context switch, save any machine state, specific
//  to this address space, that needs saving.
//
//  For now, nothing!
//----------------------------------------------------------------------

void ProcessAddrSpace::SaveStateOnSwitch() 
{}

//----------------------------------------------------------------------
// ProcessAddrSpace::RestoreStateOnSwitch
//  On a context switch, restore the machine state so that
//  this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void ProcessAddrSpace::RestoreStateOnSwitch() 
{
    machine->NachOSpageTable = NachOSpageTable;
    machine->NachOSpageTableSize = numPagesInVM;
}

unsigned
ProcessAddrSpace::GetNumPages()
{
   return numPagesInVM;
}

TranslationEntry*
ProcessAddrSpace::GetPageTable()
{
   return NachOSpageTable;
}
void
ProcessAddrSpace::SetPageTable(TranslationEntry* newpageTable, int newNumPages){
    NachOSpageTable = newpageTable ;
    numPagesInVM = newNumPages;
}
bool
ProcessAddrSpace::DemandPageAllocation(unsigned virtualaddress){
    int virtualPageNum = virtualaddress/PageSize;
    int physicalpageNum = GetPhysPage(-1);
    bzero(&machine->mainMemory[physicalpageNum*PageSize], PageSize);
    FIFO[physicalpageNum]=stats->totalTicks;

    if(NachOSpageTable[virtualPageNum].support ){
        int k = 0;
            while(k < PageSize){
                machine->mainMemory[k+physicalpageNum*PageSize] = machine->mainMemory[k+virtualPageNum*PageSize];
                k++;
            }
    }

    else{
        NoffHeader noffH;
        //openExecutable = fileSystem->Open(execFileName);

        if (openExecutable == NULL) {
            printf("Unable to open file %s\n", executablename);
            
        }

        openExecutable->ReadAt((char *)&noffH, sizeof(noffH), 0);
        
            if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
            SwapHeader(&noffH);

           
            
            openExecutable->ReadAt( &(machine->mainMemory[physicalpageNum * PageSize]), PageSize, noffH.code.inFileAddr + virtualPageNum*PageSize );
    }
    NachOSpageTable[virtualPageNum].valid = TRUE;
    NachOSpageTable[virtualPageNum].dirty = FALSE; 
    NachOSpageTable[virtualPageNum].physicalPage = physicalpageNum;
    pgPID[physicalpageNum] = currentThread->GetPID();
    pgVPN[physicalpageNum] = virtualPageNum;

    return TRUE;
}
int GetPhysPage(int pp){
    if (pageRepAlgo == 0 ){
       
          return numPagesAllocated++;
    }
    int pageval = -1 ;
    for (int i = 0; i < NumPhysPages; ++i){
       if (pgPID[i] == -1){
            return i;
       }
    }    
    int physicalpagepid = pgPID[pageval];
    int physicalpagevpn = pgVPN[pageval];
    
    //=============================================================================

    if(threadArray[physicalpagepid]->space->NachOSpageTable[physicalpagevpn].dirty){     
        for(int j = 0;j<PageSize;j++){
            threadArray[physicalpagepid]->space->supportspace[physicalpagevpn*PageSize + j] = machine->mainMemory[pageval*PageSize + j] ;
            threadArray[physicalpagepid]->space->NachOSpageTable[physicalpagevpn].support = TRUE;
        }
    }
    threadArray[physicalpagepid]->space->NachOSpageTable[physicalpagevpn].valid = FALSE;
    pgPID[pageval] = -1;
    pgOwner[pageval] = NULL;
    pgVPN[pageval] = -1;

    LRU[pageval] = stats->totalTicks;
    LRUclock[pageval] = 1;

    
    return pageval;
}

