//processmanager.h
#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include "synch.h"
#include "bitmap.h"
#include "pcb.h"

class ProcessManager {
	public:
		ProcessManager (int size);
		~ProcessManager ();
		int getPID(); //Returns unused process ID
		void clearPID (int pid); //Clears a process ID
		int getFree(); //Return number of free process IDs
		void addPCB(PCB *pcb); //Adds PCB to PCB Table
		void removePCB(int pid); //Removes PCB from PCB Table
		PCB *getProcess (int pid); //Returns pcb associated with a process ID
	private:
	BitMap *map;
	Lock *lock;
	PCB** pcbTable;
};

#endif
