//memorymanager.h

#include "synch.h"
#include "bitmap.h"

class MemoryManager {
	public:
		MemoryManager (int numTotalPages);
		~MemoryManager ();
		int getPage(); //Allocates the first clear page
		void clearPage (int pageId); //Takes the index of a page and frees it
		int getFree(); //Return number of free pages
	private:
	BitMap *virtMem;
	Lock *lock;
};