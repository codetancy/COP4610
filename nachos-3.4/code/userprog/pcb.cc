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

int PCB::getCode(){
    return childExitValue;
}

Thread* PCB::getThread(){
    return processThread;
}

//----------------------------------------------------------------------
// PCB::getParent
// 	Return parent process
//----------------------------------------------------------------------

PCB* PCB::getParent(){
    return parent_process;
}

//----------------------------------------------------------------------
// PCB::getChildren
// 	Return the list of children process
//----------------------------------------------------------------------

List* PCB::getChildren(){
    return children;
}

//----------------------------------------------------------------------
// PCB::set
// 	Stores the PID in the pcb
//  "pid" is the process ID
//----------------------------------------------------------------------

void PCB::set(int pid, PCB *parent){
    processID = pid;
    parent_process = parent;
}

//----------------------------------------------------------------------
// PCB::setExit
// 	Stores the exit code of the child
//  "code" is the exit code of the child
//----------------------------------------------------------------------

void PCB::setExit(int code){
    childExitValue = code;
}

bool PCB::addChild(PCB* childId)
{
    lock->Acquire();
    children->Append((void*) childId);
    lock->Release();
    return TRUE;
}

bool PCB::removeChild(PCB* childId)
{
    lock->Acquire();
    void* child = children->RemoveElement((void*) childId);
    lock->Release();
    return child != NULL;
}

bool PCB::isChild(PCB* childId)
{
    return children->Search((void*) childId);
}

