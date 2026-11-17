/*=====================================================================
 |  ParkEase - GLOBAL DATA DEFINITIONS                   (globals.c)
 |
 |  parkease.h declares the shared state with `extern`; this file
 |  is the ONE place where the actual variables are created, so the
 |  linker finds exactly one copy of each.
 *=====================================================================*/

#include "parkease.h"

/* Registered vehicles (filled by fileio.c -> loadVehicles) */
struct Vehicle vehicles[MAX_VEHICLES];
int vehicleCount = 0;

/* All parking slots (created by slots.c -> initializeSlots) */
struct Slot slots[TOTAL_SLOTS];

/* Waiting queue: head and rear pointers of the linked list */
struct QueueNode *queueFront = NULL;
struct QueueNode *queueRear  = NULL;
