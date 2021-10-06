#include "synch.h"
#include "system.h"
/**
 * @file elevator.cc
 * @author Group 6
 * @brief Build a controller fot the elevator in the ECS building, 
 * using SEMAPHORES or CONDITION variables. Need to implement routines 
 * by the arriving student/staff. 
 *      - Elevator Struct: represented as a thread
 *      - Person Struct: each staff/student represent a thread
 *      - Elevator(int numFloors): starts elevator thread the serves numFloor. Called 
 *        once and argument is greater than 0. Range: 1 - numFloor
 *      - ArrivingGoingFromTo(int atFloor, int toFloor): Create person thread. Wakes up
 *        elevator (not active), tells it the current floor the person is on. 
 * @version 0.1
 * @date 2021-09-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

struct PersonThread{
    int id; 
    int atFloor; 
    int toFloor; 
};

struct ElevatorThread
{
    int numFloors; 
    int currentFloor; 
    int numPeopleIn; 
};

int id = 0; //holder to track "unique id"
int generateID(){
    id++; 
    return id; 
}
int count; //Track request fullfilled

ElevatorThread eThread; 
bool movingUp = true;

int *upQueue; //Analogous to pressing elevator button going up
int *upToQueue; //Analogous to pressing desired floor inside when going up
int *downQueue; //Analogous to pressing elevator button going down
int *downToQueue; //Analogous to pressing desired floor inside when going down

Semaphore *c = new Semaphore("Thread Count Semaphore", 1);
Semaphore *q = new Semaphore("Queues", 1);
Lock *e = new Lock("Elevator Lock");
Condition *cv = new Condition("Elevator Condition");

void ManagerHelper(int *inQueue, int *outQueue) {
	q->P();
	bool getIn = false;
	bool getOut = false;
	
	int floor = eThread.currentFloor;
	//Elevator checks if there are any people in the current floor that want to get in and can due to capacity
	getIn = inQueue[floor] > 0 && eThread.numPeopleIn < 5;
	//Elevator checks if there are any people in the current floor that want to get out
	getOut = outQueue[floor] > 0;
	
	if (getIn || getOut) //Stop at current floor if anyone wants to get in or out
		printf("Elevator arrives on floor %d.\n", eThread.currentFloor);
	
	e->Release();
	q->V();
	while (getIn || getOut) { //While there are people still wanting to get in or out
		e->Acquire();
		q->P();
		getIn = inQueue[floor] > 0 && eThread.numPeopleIn < 5; //Check getin again
		getOut = outQueue[floor] > 0; //Check getout again
		cv->Broadcast(e); //Signal people to perform actions if any with the updated elevator information
		q->V();
		e->Release(); //Release lock so a person waiting can take the lock and process the elevator informaion
		currentThread->Yield();
	}
	e->Acquire(); //Once no more people can perform any actions "close the doors" and begin moving
	
	for (int delay = 0; delay < 50; delay++); //Moving delay
	
	if (movingUp)
		eThread.currentFloor++; //Move up one floor
	else
		eThread.currentFloor--; //Move down one floor
}

void Manager (int which) {
	currentThread->Yield();
	e->Acquire();
	while(true) { //Stay active until there are no more people in need of elevator
		
		if (eThread.currentFloor == 1) { //Elevator is on ground floor
			movingUp = true; //Indicate its going to move up
			ManagerHelper(upQueue, downToQueue); //People getting in are going up, People getting down were moving down
		} else if (eThread.currentFloor == eThread.numFloors) { //Elevator is on top floor
			movingUp = false; //Indicate its going to move down
			ManagerHelper(downQueue, upToQueue); //People getting in are going down, People getting down were moving up
		} else if (movingUp) { //Elevator is moving up
			ManagerHelper(upQueue, upToQueue); //People getting in are going up, People getting down were moving up
		} else if (!movingUp) { //Elevator is moving down
			ManagerHelper(downQueue, downToQueue); //People getting in are going down, People getting down were moving down
		}
		
		c->P();
		if (id == count) //Check if all people that wanted to use the elevator (id) have arrived at their destination (count)
			break; //Stop elevator
		c->V();
	}
	e->Release();
}

void Person (int person) {
	PersonThread *p = (PersonThread *) person;
	bool goingUp = p->toFloor >= p->atFloor;
	bool in = false;
	
	printf("Person %d wants to go to floor %d from floor %d.\n", p->id, p->toFloor, p->atFloor);
	
	q->P();
	if (goingUp) 
		upQueue[p->atFloor]++; //Person adds themselves to up queue (Press button to go up)
	else
		downQueue[p->atFloor]++; //Person add themselves to down queue (Press button to go down)
	q->V();
	
	while(true) { //Stay active until person arrives at desired floor
		e->Acquire();
		cv->Wait(e); //Wait for elevator to be done moving and processing
		bool readyEnter = eThread.currentFloor == p->atFloor; //Ready to enter the elevator
		bool readyLeave = eThread.currentFloor == p->toFloor; //Ready to leave the elevator
		bool capacity = eThread.numPeopleIn < 5; //Check if there is still space to go in
		bool direction = (goingUp && movingUp) || (!goingUp && !movingUp); //Check if elevator is going the same way
		if (p->atFloor == p->toFloor && readyEnter){ //Odd case where a persons current and desire floor are the same
			q->P();
			upQueue[p->atFloor]--; //Remove themselves from the waiting queue
			q->V();
			printf("Person %d got into the elevator.\n", p->id);
			printf("Person %d got out of the elevator.\n", p->id);
			break;
		} else if (!in && readyEnter && direction && capacity) { //Person can go in the elevator
			q->P();
			if (goingUp) {
				upToQueue[p->toFloor]++; //Person adds themselves to up destination queue (Press button of desired floor once inside)
				upQueue[p->atFloor]--; //Remove person from waiting queue
			}
			else {
				downToQueue[p->toFloor]++; //Person adds themselves to down destination queue (Press button of desired floor once inside)
				downQueue[p->atFloor]--; //Remove person from waiting queue
			}
			q->V();
			eThread.numPeopleIn++; //Person got into the elevator so increment count
			in = true; //Person is in the elevator
			printf("Person %d got into the elevator.\n", p->id);
		} else if (in && readyLeave) { //Person in elevator arrived to desired floor
			q->P();
			if (goingUp)
				upToQueue[p->toFloor]--; //Remove person from destination queue
			else
				downToQueue[p->toFloor]--; //Remove person from destination queue
			q->V();
			eThread.numPeopleIn--; //Person got out so decrement count
			printf("Person %d got out of the elevator.\n", p->id);
			break; //Leave while loop, person arrived at desired floor
		}
		e->Release(); //Release lock on elevator if person has no more available actions and is done using elevator information
	}
	c->P();
	count++; //Person's request has been fulfilled so increment count
	c->V();
	e->Release(); //Release lock on elevator once person has arrived
}

void ArrivingGoingFromTo(int atFloor, int toFloor){
    PersonThread *pThread = new PersonThread;
    Thread *person; // create person thread
    person = new Thread("Creating Person Thread"); 
    pThread -> atFloor = atFloor; //initialize current floor
    pThread -> toFloor = toFloor; //initialize desired floor 
	
	c->P(); //Shared variable id is also used to represent how many request are made
	pThread -> id = generateID(); //generate id for person
	c->V();
	
    person -> Fork(Person, (int)pThread); 
}

void Elevator(int numFloors){
    if(numFloors <= 0){//check for valid floors
        printf("Invalid Number Of Floors!"); 
        return; 
    }

    Thread *elevator = new Thread("Creating Elevator Thread");
	e->Acquire();
    eThread.numFloors = numFloors; //initialize num of floors for elevator thread
    eThread.currentFloor = 1; //start at ground floor
    eThread.numPeopleIn = 0; //starts as empty
	e->Release();

	q->P();
	int size = numFloors+1; //Initialize waiting queues
	upQueue = new int[size];
	upToQueue = new int[size];
	downQueue = new int[size];
	downToQueue = new int[size];
	q->V();

    elevator -> Fork(Manager, 0); 

}
