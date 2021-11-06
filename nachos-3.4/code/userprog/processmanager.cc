//processmanager.cc

#include "processmanager.h"

//----------------------------------------------------------------------
// ProcessManager::ProcessManager
// 	Initialize the process manager
//----------------------------------------------------------------------

ProcessManager::ProcessManager (int size){
	map = new BitMap(size);
	lock = new Lock("Process bitmap lock");
	pcbTable = new PCB*[size];
}

//----------------------------------------------------------------------
// ProcessManager::ProcessManager
// 	De-allocate the process manager.
//----------------------------------------------------------------------

ProcessManager::~ProcessManager(){
	delete map;
	delete lock;
	delete pcbTable;
}

//----------------------------------------------------------------------
// ProcessManager::getPID
// 	Returns unused process ID
//----------------------------------------------------------------------

int ProcessManager::getPID(){
	lock->Acquire();
	int PID = map->Find();
	lock->Release();
	return PID;
}

//----------------------------------------------------------------------
// ProcessManager::clearPID
// 	Clears a process ID
//  "pid" is the process ID
//----------------------------------------------------------------------

void ProcessManager::clearPID(int pid){
	lock->Acquire();
	map->Clear(pid);
	pcbTable[pid] = NULL;
	lock->Release();
}

//----------------------------------------------------------------------
// ProcessManager::getFree
// 	Return number of free process IDs
//----------------------------------------------------------------------

int ProcessManager::getFree(){
	lock->Acquire();
	int numFree = map->NumClear();
	lock->Release();
	return numFree;
}

//----------------------------------------------------------------------
// ProcessManager::addPCB
// 	Adds PCB to PCB Table
//----------------------------------------------------------------------

void ProcessManager::addPCB(PCB *pcb){
	lock->Acquire();
	pcbTable[pcb->getID()] = pcb;
	lock->Release();
}

//----------------------------------------------------------------------
// ProcessManager::getProcess
// 	Returns pcb associated with a process ID
//  "pid" is the process ID
//----------------------------------------------------------------------

PCB* ProcessManager::getProcess(int pid){
	lock->Acquire();
	PCB *pcb = map->Test(pid) ? pcbTable[pid] : NULL;
	lock->Release();
	return pcb;
}
