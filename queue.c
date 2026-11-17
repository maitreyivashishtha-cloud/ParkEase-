/*=====================================================================
 |  ParkEase - MODULE: WAITING QUEUE                          (queue.c)
 |
 |  Dynamic queue = SINGLY LINKED LIST built with malloc() and
 |  released with free(). This is the data-structures highlight of
 |  the project:
 |      queueFront -> [C16] -> [C17] -> [C18] <- queueRear
 |
 |  When the lot is full, vehicles join at the REAR. When a slot is
 |  freed (exit or admin release), the FRONT is scanned and the slot
 |  is given to the first waiting vehicle of the matching type.
 |  Saving/loading the queue to queue.dat is done by fileio.c.
 *=====================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parkease.h"
#include "queue.h"
#include "vehicle.h"
#include "fileio.h"

/*---------------------------------------------------------------------
 |  Add a vehicle at the REAR of the waiting queue
 *-------------------------------------------------------------------*/
void enqueueVehicle(const char *regNo, time_t waitTime)
{
    struct QueueNode *node =
        (struct QueueNode *)malloc(sizeof(struct QueueNode));
    if (node == NULL) { printf("  [!] Out of memory for waiting queue.\n"); return; }
    strcpy(node->regNo, regNo);
    node->waitTime = waitTime;
    node->next = NULL;
    if (queueRear == NULL) queueFront = queueRear = node;
    else { queueRear->next = node; queueRear = node; }
}

/*---------------------------------------------------------------------
 |  1-based position of a vehicle in the queue; 0 if not waiting
 *-------------------------------------------------------------------*/
int queuePosition(const char *regNo)
{
    struct QueueNode *node = queueFront;
    int pos = 1;
    while (node != NULL) {
        if (strcmp(node->regNo, regNo) == 0) return pos;
        node = node->next;
        pos++;
    }
    return 0;
}

/*---------------------------------------------------------------------
 |  Number of vehicles currently waiting
 *-------------------------------------------------------------------*/
int queueLength(void)
{
    struct QueueNode *node = queueFront;
    int count = 0;
    while (node != NULL) { count++; node = node->next; }
    return count;
}

/*---------------------------------------------------------------------
 |  Unlink a node from the linked list (caller frees the node)
 *-------------------------------------------------------------------*/
static void unlinkNode(struct QueueNode *prev, struct QueueNode *node)
{
    if (prev == NULL) queueFront = node->next;
    else              prev->next = node->next;
    if (queueRear == node) queueRear = prev;
}

/*---------------------------------------------------------------------
 |  Remove a specific vehicle from the queue
 *-------------------------------------------------------------------*/
void removeFromQueue(const char *regNo)
{
    struct QueueNode *node = queueFront, *prev = NULL;
    while (node != NULL) {
        if (strcmp(node->regNo, regNo) == 0) {
            unlinkNode(prev, node);
            free(node);                    /* release dynamic memory   */
            return;
        }
        prev = node;
        node = node->next;
    }
}

/*---------------------------------------------------------------------
 |  Free the whole queue (used by the admin reset)
 *-------------------------------------------------------------------*/
void freeQueue(void)
{
    struct QueueNode *node = queueFront;
    while (node != NULL) {
        struct QueueNode *next = node->next;
        free(node);
        node = next;
    }
    queueFront = queueRear = NULL;
}

/*---------------------------------------------------------------------
 |  Operator option 6 : display the queue from front to rear
 *-------------------------------------------------------------------*/
void displayQueue(void)
{
    struct QueueNode *node = queueFront;
    int pos = 1;
    char sinceBuf[20];
    int vi;
    printTitle("WAITING QUEUE");
    if (queueFront == NULL) {
        printf("  The waiting queue is empty. All is well!\n");
        return;
    }
    printf("  %-4s %-14s %-20s %-16s\n", "Pos", "Reg. No", "Type",
           "Waiting since");
    printf("  -------------------------------------------------\n");
    while (node != NULL) {
        vi = findVehicleIndex(node->regNo);
        formatTime(node->waitTime, sinceBuf);
        printf("  %-4d %-14s %-20s %-16s\n",
               pos, node->regNo,
               (vi >= 0) ? typeName(vehicles[vi].type) : "Unknown",
               sinceBuf);
        node = node->next;
        pos++;
    }
    printf("  -------------------------------------------------\n");
    printf("  Total vehicles waiting : %d\n", pos - 1);
}

/*---------------------------------------------------------------------
 |  A slot has just been freed: scan the queue from the FRONT and
 |  give the slot to the first waiting vehicle of the matching type.
 |  Stale records (vehicle no longer registered) are dropped.
 *-------------------------------------------------------------------*/
int assignWaitingVehicle(int slotIndex)
{
    struct QueueNode *node = queueFront, *prev = NULL;
    int slotType = slots[slotIndex].type;
    int vi;
    char label[6];
    char nowBuf[20];

    while (node != NULL) {
        struct QueueNode *next = node->next;
        vi = findVehicleIndex(node->regNo);
        if (vi < 0) {
            /* stale record (vehicle no longer registered): drop it  */
            unlinkNode(prev, node);
            free(node);
            saveQueue();
            node = next;
            continue;
        }
        if (vehicles[vi].type == slotType) {
            slots[slotIndex].occupied = 1;
            strcpy(slots[slotIndex].regNo, node->regNo);
            slots[slotIndex].entryTime = time(NULL);
            saveSlots();
            slotLabel(slots[slotIndex].slotId, label);
            formatTime(slots[slotIndex].entryTime, nowBuf);
            printf("  [OK] Slot %s auto-allocated to waiting vehicle %s (%s).\n",
                   label, node->regNo, typeName(slotType));
            printf("       Entry time : %s\n", nowBuf);
            unlinkNode(prev, node);
            free(node);
            saveQueue();
            return 1;
        }
        prev = node;
        node = next;
    }
    return 0;
}
