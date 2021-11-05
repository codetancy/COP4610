//pcb.h

#ifndef PCB_H
#define PCB_H

class Thread;
class PCB{
	public:
		PCB (Thread *input);
		~PCB ();
		int getID();
		PCB *getParent();
		void set(int pid, PCB *parent);
	private:
		int MAX_FILES;
		Thread *processThread;
		int processID;
		PCB *parent_process;
};
#endif