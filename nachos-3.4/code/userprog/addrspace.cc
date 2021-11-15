// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader (NoffHeader *noffH)
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
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
#ifdef CHANGED
    NoffHeader noffH;
    unsigned int size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space? we need to increase the size to leave room for the stack
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;

    numPages = divRoundUp(size, PageSize);

    ASSERT(numPages <= memoryManager->getFree());	// Check there are enough pages

    pageTable = new TranslationEntry[numPages];
    InitializePageTable();

    if (noffH.code.size > 0)
        ReadFile(noffH.code.virtualAddr, executable, noffH.code.size, noffH.code.inFileAddr);

    if (noffH.initData.size > 0)
        ReadFile(noffH.code.virtualAddr, executable, noffH.initData.size, noffH.code.inFileAddr);
    
    printf("Loaded Program: [%d] code | [%d] data | [%d] bss\n", noffH.code.size, noffH.initData.size, noffH.uninitData.size);
    
#else
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space? we need to increase the size to leave room for the stack
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;

    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);	// check we're not trying to run anything too big --
					// at least until we have virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages, size);

    pageTable = new TranslationEntry[numPages];

    for (i = 0; i < numPages; i++)
    {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = i;
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
					// a separate page, we could set its
					// pages to be read-only
    }
    // zero out the entire address space, to zero the unitialized data segment
    // and the stack segment
    bzero(machine->mainMemory, size);

    // then, copy in the code and data segments into memory
    if (noffH.code.size > 0)
    {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
            noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(
            &(machine->mainMemory[noffH.code.virtualAddr]),
            noffH.code.size,
            noffH.code.inFileAddr
        );
    }

    if (noffH.initData.size > 0)
    {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
            noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(
            &(machine->mainMemory[noffH.initData.virtualAddr]),
            noffH.initData.size,
            noffH.initData.inFileAddr
        );
    }
#endif
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
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
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

#ifdef CHANGED

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space.
//	"num" is the number of pages
//----------------------------------------------------------------------

AddrSpace::AddrSpace(unsigned int num)
{
    numPages = num;
    pageTable = new TranslationEntry[numPages];
}

//----------------------------------------------------------------------
// AddrSpace::InitializePageTable
//
//----------------------------------------------------------------------
void AddrSpace::InitializePageTable()
{
    unsigned int i;
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;
	pageTable[i].physicalPage = memoryManager->getPage();
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;
	bzero(machine->mainMemory + pageTable[i].physicalPage * PageSize, PageSize);
    }
}

//----------------------------------------------------------------------
// AddrSpace::ReleasePhysicalMemory
// 	Asks the memory manager to clear the physical pages of the
//	current address space
//----------------------------------------------------------------------
void AddrSpace::ReleasePhysicalMemory()
{
    unsigned int i;
    for (i = 0; i < numPages; i = i + 1)
    {
        memoryManager->clearPage(pageTable[i].physicalPage);
    }
}

//----------------------------------------------------------------------
// AddrSpace::Translate
// 	Uses the virtual address and page number to calculate the
//	physical address.
//----------------------------------------------------------------------

int AddrSpace::Translate(int virtualAddress)
{
    if (virtualAddress < 0)
    {
        return -1;
    }

    int pageNumber = virtualAddress / PageSize;
    int memoryOffset = virtualAddress % PageSize;

    if ((unsigned)pageNumber >= numPages)
    {
        return -1;
    }

    if (!pageTable[pageNumber].valid)
    {
        return -1;
    }

    int frameNumber = pageTable[pageNumber].physicalPage;
    int physicalAddress = frameNumber * PageSize + memoryOffset;

    pageTable[pageNumber].use = TRUE;

    return physicalAddress;
}

//----------------------------------------------------------------------
// AddrSpace::ReadFile
// 	Loads the code and data segment into the translated memory
//----------------------------------------------------------------------

int AddrSpace::ReadFile(int virtAddr, OpenFile* file, int size, int fileAddr)
{
    int physAddr;

    int readSize;
    int numBytesRead = 0;
    int numBytesLeft = size;

    char buffer[size];
    file -> ReadAt(buffer, size, fileAddr);

    while(numBytesLeft > 0){
        readSize = numBytesLeft < PageSize? numBytesLeft: PageSize - virtAddr % PageSize;

        physAddr = Translate(virtAddr);
        ASSERT(physAddr >= 0);

        bcopy(buffer + numBytesRead, machine->mainMemory + physAddr, readSize);
        numBytesLeft -= readSize;
        numBytesRead += readSize;
        virtAddr += readSize;
    }

    return numBytesRead;
}

//----------------------------------------------------------------------
// AddrSpace::copy
// 	Copies address space
//	"num" is the number of pages
//----------------------------------------------------------------------

AddrSpace* AddrSpace::copy()
{
    if (numPages > memoryManager->getFree()){
        return NULL;
    }

    AddrSpace* newAddrSpace = new AddrSpace(numPages);

    for (unsigned int i = 0; i < numPages; i++) {
        newAddrSpace->pageTable[i].virtualPage = pageTable[i].virtualPage;
        newAddrSpace->pageTable[i].physicalPage = memoryManager->getPage();
        newAddrSpace->pageTable[i].valid = pageTable[i].valid;
        newAddrSpace->pageTable[i].use = pageTable[i].use;
        newAddrSpace->pageTable[i].dirty = pageTable[i].dirty;
        newAddrSpace->pageTable[i].readOnly = pageTable[i].readOnly;

        bcopy(
            machine->mainMemory + pageTable[i].physicalPage*PageSize, 
            machine->mainMemory + newAddrSpace->pageTable[i].physicalPage*PageSize,
            PageSize
        );
    }

    return newAddrSpace;
}

bool Replace(OpenFile* exec)
{
    return TRUE;
}

unsigned int AddrSpace::getPages(){
    return numPages;
}

#endif
