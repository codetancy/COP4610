//memorymanager.cc

#include "memorymanager.h"

//----------------------------------------------------------------------
// MemoryManager::MemoryManager
// 	Initialize the memory manager
//----------------------------------------------------------------------

MemoryManager::MemoryManager (int numTotalPages){
	virtMem = new BitMap(numTotalPages);
	lock = new Lock("bitmap lock");
}

//----------------------------------------------------------------------
// MemoryManager::MemoryManager
// 	De-allocate the memory manager.
//----------------------------------------------------------------------

MemoryManager::~MemoryManager(){
	delete virtMem;
	delete lock;
}

//----------------------------------------------------------------------
// MemoryManager::getPage
// 	Allocates the first clear page
//----------------------------------------------------------------------

int MemoryManager::getPage(){
	lock->Acquire();
	int pageNum = virtMem->Find(); //find and allocate a bit
	if (pageNum == -1){
		// Error: No clear bits
		// Add error handling if error is to be handled here
	}
	lock->Release();
	return pageNum;
}

//----------------------------------------------------------------------
// MemoryManager::clearPage
// 	Takes the index of a page and frees it
//  "pageId" is the index of the page to be cleared
//----------------------------------------------------------------------

void MemoryManager::clearPage(int pageId){
	lock->Acquire();
	virtMem->Clear(pageId); //Clear the bit
	lock->Release();
}