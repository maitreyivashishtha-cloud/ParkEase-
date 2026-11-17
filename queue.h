/*=====================================================================
 |  ParkEase - MODULE: WAITING QUEUE                        (queue.h)
 |
 |  A DYNAMIC queue implemented as a linked list with malloc/free
 |  (the data-structures highlight of this project):
 |    - vehicles join at the REAR when no slot of their type is free
 |    - when a slot is freed, the FRONT is scanned first and the slot
 |      goes to the first waiting vehicle of the matching type
 |  Persistence of the queue is done by the File Handling Module.
 *=====================================================================*/

#ifndef QUEUE_H
#define QUEUE_H

#include <time.h>

/* Add a vehicle at the REAR of the waiting queue */
void enqueueVehicle(const char *regNo, time_t waitTime);

/* 1-based position of a vehicle in the queue; 0 if not waiting */
int  queuePosition(const char *regNo);

/* Number of vehicles currently waiting */
int  queueLength(void);

/* Remove a specific vehicle from the queue (it left / gave up) */
void removeFromQueue(const char *regNo);

/* Free the whole queue (used by the admin reset) */
void freeQueue(void);

/* Menu option: display the queue from front to rear */
void displayQueue(void);

/* A slot has just been freed: give it to the first waiting vehicle
   of the matching type. Returns 1 if somebody got the slot.         */
int  assignWaitingVehicle(int slotIndex);

#endif /* QUEUE_H */
