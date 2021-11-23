// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

void update(){
    machine->WriteRegister(PCReg, machine->ReadRegister(PCReg) + 4);
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PrevPCReg) + 4);
}

void dummy(int arg){
    currentThread->RestoreUserState();
    currentThread->space->RestoreState();
    machine->Run();
}

/* PROJECT 3 Section */
void write(){
}

void open(){
}

void read(){
}

void close(){
}

void create(){
}


/* PROJECT 2 Section */
void fork(){
    printf("System Call: [%d] invoked Fork\n", currentThread->space->pcb->getID());
    // 1. Save old process registers.
    currentThread->SaveUserState();

    // 2. Create a new AddrSpace and copy old AddrSpace to new AddrSpace.
    // (Improve the current AddrSpace).
    AddrSpace *newAddrSpace = currentThread->space->copy();
    if (newAddrSpace == NULL) {
        machine->WriteRegister(2, -1);
        update();
        return;
    }

    // 3. Create a new Thread, associate new AddrSpace with new thread.
    Thread *newThread = new Thread("New Thread");
    newThread->space = newAddrSpace;

    // 4. Create a PCB (Process Control Block) and associate the new Address and
    // new Thread with PCB.
    int pid = processManager->getPID();
    if (pid < 0){
        machine->WriteRegister(2, -1);
        update();
        return;
    }
    PCB *newPCB = new PCB(newThread);
    newAddrSpace->pcb = newPCB;

    // 5. Complete PCB with information such as pid, ppid, etc.
    newPCB->set(pid, currentThread->space->pcb);
    currentThread->space->pcb->addChild(newPCB);
    processManager->addPCB(newPCB);

    // 6. Copy old register values to new register. Set pc reg value to value in r4.
    // save new register values to new AddrSpace.
    machine->WriteRegister(PCReg, machine->ReadRegister(4));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(4) + 4);
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(4) - 4);
    newThread->SaveUserState();

    // 7. Use Thread::Fork(func, arg) to set new thread behavior: restore registers,
    // restore memory and put machine to run.
    newThread->Fork(dummy, 0);

    // 8. Restore old process register
    currentThread->RestoreUserState();

    // 9. Write new process pid to r2
    machine->WriteRegister(2,pid);

    // 10. Update counter of old process and return.
    update();
    printf("Process [%d] Fork: start at address [0x%x] with [%d] pages memory\n",
        currentThread->space->pcb->getID(),
        machine->ReadRegister(4),
        currentThread->space->getPages());
}

void yield()
{
    printf("System Call: [%d] invoked Yield\n", currentThread->space->pcb->getID());
    // 1. Current thread yield
    currentThread->Yield();
    update();
}

void nullParent(int p){
    PCB *pcb = (PCB*) p;
    pcb->set(pcb->getID(), NULL);
}

void exit(){
    int pid = currentThread->space->pcb->getID();
    printf("System Call: [%d] invoked Exit\n", pid);

    // 1. Get exit code from r4;
    int code = machine->ReadRegister(4);
    // if current process has children, set their parent pointers to null;
    List *children = currentThread->space->pcb->getChildren();
    if(!children->IsEmpty()){
        children->Mapcar(nullParent);
    }

    // if current process has a parent, remove itself from the children list of its
    // parent process and set child exit value to parent.
    PCB *parent = currentThread->space->pcb->getParent();
    if(parent != NULL){
        parent->removeChild(currentThread->space->pcb);
        parent->setExit(code);
    }

    // 2. Remove current process from the pcb manager and pid manager.
    processManager->removePCB(pid);
    processManager->clearPID(pid);

    // 3. Deallocate the process memory and remove from the page table;
    // current thread finish.
    currentThread->space->ReleasePhysicalMemory();
    delete currentThread->space->pcb;
    delete currentThread->space;
    printf("Process [%d] exits with [%d]\n", pid, code);
    currentThread->Finish();
}

void join(){
    printf("System Call: [%d] invoked Join\n", currentThread->space->pcb->getID());

    // 1. Read process id from register r4.
    int pid = machine->ReadRegister(4);

    // 2. Make sure the requested process id is the child process of the current
    // process.
    if (pid < 0){
        machine->WriteRegister(2, -1);
        update();
        return;
    }
    PCB* pcb = processManager->getProcess(pid);
    if (!currentThread->space->pcb->isChild(pcb)){ //check if pid is a child
        machine->WriteRegister(2, -1);
        update();
        return;
    }

    // 3. Keep on checking if the requested process is finished. if not, yield the
    // current process.
    while(processManager->getProcess(pid)){
        currentThread->Yield();
    }

    // 4. If the requested process finished, write the requested process exit id to
    // register r2 to return it.
    machine->WriteRegister(2, currentThread->space->pcb->getCode());
    update();
}

void kill(){
    int pid = currentThread->space->pcb->getID();
    printf("System Call: [%d] invoked Kill.\n", pid); 
    
    // 1. Read killed ID from r4. 
    int killID = machine->ReadRegister(4);
    
    // 2. Make sure if the killed process exists.
    PCB* killPCB = processManager->getProcess(killID);
    if(killPCB == NULL){
        machine->WriteRegister(2, -1);
        update();
        printf("Process [%d] cannot kill process [%d]: doesn't exist\n", pid, killID); 
        return;
    }
    
    // 3. If killed process is the current process, simply call exit 
    if(killID == pid){
        exit();
        machine->WriteRegister(2, 0);
        update();
        return;
    }
        // if the killed process has children, set their parent pointers to null
    List *children = killPCB->getChildren();
    if(!children->IsEmpty()){
        children->Mapcar(nullParent);
    }
        // if the killed process has a parent, remove itself from the child list.
    PCB *parent = killPCB->getParent();
    if(parent != NULL){
        parent->removeChild(killPCB);
    }
    
    // 4. Remove itself from the pcb list and pid list.
    processManager->removePCB(killID);
    processManager->clearPID(killID);
    
    // 5. Free up memory and remove page table item.
    Thread* killThread = killPCB->getThread();
    AddrSpace* killSpace = killThread->space;
    killSpace->ReleasePhysicalMemory();
    delete killPCB;
    delete killSpace;
    
    // 6. Remove killed thread from scheduler. 
    scheduler->Remove(killThread);
    
    // 7. Return 0 in r2 to show succeed.
    machine->WriteRegister(2, 0);
    update();
    printf("Process [%d] killed process [%d]\n", pid, killID);
}

void exec()
{
    printf("System Call: [%d] invoked Exec\n", currentThread->space->pcb->getID());

    // 1. Read register r4 to get the executable path.
    int address = machine->ReadRegister(4);

    // 2. Replace the process memory with the content of the executable.
    // 3. Init registers.
    bool success = currentThread->space->Replace(address);
    
    // 4. Write 1 to register r2 indicating exec() invoked successful;
    // 5. Return -1 if any step failed. e.g., the executable is unrecognizable. 
    if (success)
        machine->WriteRegister(2, 1);
    else
        machine->WriteRegister(2, -1);

    update();
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    /* Make System Calls*/
    if ((which == SyscallException)){
        switch(type){
            /*FIXME: PROJECT 3 - Implement System Calls for Open, Read, Write, Close, Create*/
            case SC_Open:
                DEBUG('a', "Open, initiated by user program.\n");
               	//open(); 
                break;
			
            case SC_Read:
                DEBUG('a', "Read, initiated by user program.\n");
                //read(); 
                break;
			
            case SC_Write:
                DEBUG('a', "Write, initiated by user program.\n");
                //write(); 
                break;	
			
            case SC_Close:
                DEBUG('a', "Close, initiated by user program.\n");
                //close(); 
                break;
			
            case SC_Create:
                DEBUG('a', "Create, initiated by user program.\n");
                //create(); 
                break;
			
	    /* PROJECT 2 - Fork, Yield, Exec, Join, Kill, Exit */
            case SC_Fork:
                DEBUG('a', "Fork, initiated by user program.\n");
                fork();
                break;

            case SC_Yield:
                DEBUG('a', "Yield, initiated by user program.\n");
                yield();
                break;

            case SC_Exec:
                DEBUG('a', "Exec, initiated by user program.\n");
                exec();
               	break;

            case SC_Join:
                DEBUG('a', "Join, initiated by user program.\n");
                join();
                break;

            case SC_Exit:
                DEBUG('a', "Exit, initiated by user program.\n");
                exit();
                break;

            case SC_Kill:
                DEBUG('a', "Kill, initiated by user program.\n");
		kill(); 
                break;

            default: //default -- halt
                DEBUG('a', "Shutdown, initiated by user program.\n");
                    interrupt->Halt();
                break;
        }// end of switch case
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}

/* Original ExceptionHandler
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
*/
