// addrspace.h
//	Data structures to keep track of executing user programs
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#if defined(CHANGED)

#include "pcb.h"
class PCB;

#endif

#define UserStackSize 1024            // increase this as necessary!

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);  // Create an address space,
                                      // initializing it with the program
                                      // stored in the file "executable"
    ~AddrSpace();                     // De-allocate an address space

    void InitRegisters();   // Initialize user-level CPU registers,
                            // before jumping to user code

    void SaveState();         // Save/restore address space-specific
    void RestoreState();      // info on a context switch
#if defined(CHANGED)
    int Translate(int virtualAddress);  // Obtains physical address from virtual
                                        // address and page size
    int ReadFile(int virtAddr, OpenFile* file, int size, int fileAddr);
                                  // Loads code and data segments
    AddrSpace(unsigned int num); 	// Create an address space
    AddrSpace* copy();            // Copy old addrspace to new addrspace

    PCB *pcb;                     // Address space Process Control Block

    unsigned int getPages();
    void ReleasePhysicalMemory(); // Request the memory manager to clear pages
    bool Replace(int virtualAddress); // Replace the current address space
#endif
  private:
    TranslationEntry *pageTable;  // Page table
    unsigned int numPages;        // Number of pages in the virtual address space

#if defined(CHANGED)
    void InitializePageTable();   // Request the memory manager free physical pages
    int ReadFileName(int virtualAddress, char* fileName, int size);
#endif
};

#endif // ADDRSPACE_H
