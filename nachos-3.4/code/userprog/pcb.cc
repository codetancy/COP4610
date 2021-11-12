//pcb.cc

#include "pcb.h"

//----------------------------------------------------------------------
// PCB::PCB
// 	Initialize pcb
//----------------------------------------------------------------------

PCB::PCB (Thread *input){
    processThread = input;
    children = new List();
    lock = new Lock("Children lock");
}

//----------------------------------------------------------------------
// PCB::PCB
// 	De-allocate pcb
//----------------------------------------------------------------------

PCB::~PCB(){
    delete children;
    delete lock;
}

//----------------------------------------------------------------------
// PCB::getID
// 	Return PID
//----------------------------------------------------------------------

int PCB::getID(){
    return processID;
}

//----------------------------------------------------------------------
// PCB::getParent
// 	Return parent process
//----------------------------------------------------------------------

PCB* PCB::getParent(){
    return parent_process;
}

//----------------------------------------------------------------------
// PCB::setID
// 	Stores the PID in the pcb
//  "pid" is the process ID
//----------------------------------------------------------------------

void PCB::set(int pid, PCB *parent){
    processID = pid;
    parent_process = parent;
}

bool PCB::addChild(int* childId)
{
    lock->Acquire();
    children->Append((void*) childId);
    lock->Release();
    return TRUE;
}

bool PCB::removeChild(int* childId)
{
    lock->Acquire();
    void* child = children->RemoveElement((void*) childId);
    lock->Release();
    return child != NULL;
}

bool PCB::isChild(int* childId)
{
    return children->Search((void*) childId);
}

