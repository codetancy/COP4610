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
    int currFloor; 
    int numPeopleIn; 
};

int id = 0; //holder to track "unique id"
int generateID(){
    id++; 
    return id; 
}

ElevatorThread eThread; 
Thread *elevator; 


Semaphore *semaphore  = new Semaphore("Semaphore", 1); 
Semaphore *barrier = new Semaphore("Barrier", 0);


void ArrivingGoingFromToTest(int p){
    if(eThread.numPeopleIn <= 5){// meet capacity requirements
        printf("Too Many People in Elevator!"); 
        return;
    }

    PersonThread *pThread = (PersonThread*) p; 
    semaphore -> P(); //waits
    printf("person %d  wants to go to floor %d from floor %d ", pThread-> id, pThread -> toFloor, pThread->atFloor);
    
    eThread.currFloor = pThread->atFloor; //elevator starts/arives at current floor
    eThread.numPeopleIn++; //increase amount of people in elevator
     
    
    printf("person %d got into the elevator", pThread -> id); 
    currentThread -> Yield(); 

	//floors are 50 clicks apart
    for(int i = (pThread->atFloor * 50); i <= (pThread->toFloor * 50); i++){// loops until reaches destination 
        semaphore -> V(); 
        currentThread -> Yield(); 

        if((i % 50) == 0){
            eThread.currFloor++; 
            printf("Elevator arrives at %d", eThread.currFloor); 
        }
    }

    semaphore -> P(); 
    printf("person %d got out of the elevator", pThread -> id);
    eThread.numPeopleIn--; 

	//use better solution to find out if remaining threads/people exist
    if(eThread.numPeopleIn == 0){ 
        barrier ->V(); 
    }
   
    barrier ->P(); 
    barrier ->V(); 

}

void ArrivingGoingFromTo(int atFloor, int toFloor){
    PersonThread *pThread;
    Thread *person; // create person thread
    person = new Thread("Creating Person Thread"); 
    pThread -> id = generateID(); //generate id for person
    pThread -> atFloor = atFloor; //initialize current floor
    pThread -> toFloor = toFloor; //initialize desired floor 

    person -> Fork(ArrivingGoingFromToTest, (int)pThread); 
    ArrivingGoingFromToTest((int)pThread);// run simulation
}

void Elevator(int numFloors){
    if(numFloors <= 0){//check for valid floors
        printf("Invalid Number Of Floors!"); 
        return; 
    }

    elevator = new Thread("Creating Elevator Thread");
    eThread.numFloors = numFloors; //initialize num of floors for elevator thread
    eThread.currFloor = 0; //start at ground floor
    eThread.numPeopleIn = 0; //starts as empty

    elevator -> Fork(Elevator, 1); 

}
