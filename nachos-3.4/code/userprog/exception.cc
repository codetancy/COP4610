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

void fork(){
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
}

void yield()
{
    // 1. Current thread yield
    currentThread->Sleep();
    update();
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    /* Make System Calls*/
    if ((which == SyscallException)){
        switch(type){
            /*FIXME: Make edits to each system call to work successfully*/
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
                break;

            case SC_Join:
                DEBUG('a', "Join, initiated by user program.\n");
                break;

            case SC_Exit:
                DEBUG('a', "Exit, initiated by user program.\n");
                break;

            case SC_Kill:
                DEBUG('a', "Kill, initiated by user program.\n");
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
