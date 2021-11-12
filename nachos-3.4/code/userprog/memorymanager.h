//memorymanager.h

#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include "synch.h"
#include "bitmap.h"

class MemoryManager {
    public:
        MemoryManager (int numTotalPages);	// Creates a memory manager with set number of pages
        ~MemoryManager ();
        int getPage(); 				// Allocates the first clear page
        void clearPage (int pageId); 		// Takes the index of a page and frees it
        unsigned int getFree(); 		// Return number of free pages
    private:
        BitMap *virtMem;
        Lock *lock;
};

#endif
