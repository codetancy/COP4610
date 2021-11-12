//pcb.h

#ifndef PCB_H
#define PCB_H

#include "copyright.h"
#include "list.h"
#include "synch.h"

class Thread;
class Lock;

class PCB {
    public:
        PCB (Thread *input);
        ~PCB ();
        int getID();
        PCB *getParent();
        void set(int pid, PCB *parent);
        bool addChild(int *childId);
        bool removeChild(int *childId);
        bool isChild(int *childId);
    private:
        int MAX_FILES;
        Thread *processThread;
        int processID;
        PCB *parent_process;
        List *children;
        Lock *lock;
};
#endif
