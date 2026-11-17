/*=====================================================================
 |  ParkEase - MODULE: ENTRY / EXIT MANAGEMENT            (entryexit.c)
 |
 |  The two core data flows of the project slides:
 |
 |  ENTERING: operator enters plate number -> the Vehicle Management
 |  module validates (or creates) the registration -> Parking Slot
 |  Management finds a free slot of the matching type -> the slot is
 |  assigned and the entry time recorded -> the File Handling Module
 |  saves the updated slot record. If no slot is free, the Waiting
 |  Queue Module takes over.
 |
 |  LEAVING: operator enters plate number -> entry record retrieved
 |  -> the Billing Module calculates duration and fee and prints the
 |  receipt -> the slot is freed and the transaction saved -> the
 |  freed slot is auto-allocated to the first matching waiting vehicle.
 *=====================================================================*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "parkease.h"
#include "entryexit.h"
#include "vehicle.h"
#include "slots.h"
#include "billing.h"
#include "queue.h"
#include "fileio.h"

/*---------------------------------------------------------------------
 |  Operator option 2 : vehicle entry + automatic slot allocation
 *-------------------------------------------------------------------*/
void vehicleEntry(void)
{
    char regNo[16], label[6], nowBuf[20], owner[32], answer[8];
    int idx, s, type;
    time_t now;

    printTitle("VEHICLE ENTRY AND SLOT ALLOCATION");
    readString("Enter vehicle registration number : ", regNo, (int)sizeof regNo);
    toUpperStr(regNo);

    /* 1. validate the registration (offer to register on the spot) */
    idx = findVehicleIndex(regNo);
    if (idx < 0) {
        printf("  [!] Vehicle %s is NOT registered.\n", regNo);
        readString("      Register it now? (yes/no) : ", answer, (int)sizeof answer);
        if (!isYes(answer)) {
            printf("  [i] Entry cancelled. Register the vehicle from the\n"
                   "      Operator Menu (option 1) first.\n");
            return;
        }
        readName("      Enter owner name                  : ",
                 owner, (int)sizeof owner);
        type = readInt("      Vehicle type  [1] Bike  [2] Car  [3] Heavy : ",
                       1, 3);
        if (!addVehicle(regNo, owner, type)) return;    /* saves file  */
        printf("  [OK] Vehicle %s registered (%s, owner %s).\n",
               regNo, typeName(type), owner);
    }

    /* already inside or already waiting? */
    s = findSlotByReg(regNo);
    if (s >= 0) {
        slotLabel(slots[s].slotId, label);
        printf("  [!] Vehicle %s is ALREADY parked at slot %s.\n", regNo, label);
        return;
    }
    if (queuePosition(regNo) > 0) {
        printf("  [!] Vehicle %s is already in the waiting queue (position %d).\n",
               regNo, queuePosition(regNo));
        return;
    }

    /* 2. find a free slot of the matching type                     */
    now = time(NULL);                          /* current date + time   */
    idx = findVehicleIndex(regNo);             /* (now definitely >= 0) */
    s = findFreeSlotIndex(vehicles[idx].type);
    if (s >= 0) {
        /* 3. assign the slot, record the entry time, save          */
        slots[s].occupied  = 1;
        strcpy(slots[s].regNo, regNo);
        slots[s].entryTime = now;
        saveSlots();
        slotLabel(slots[s].slotId, label);
        formatTime(now, nowBuf);
        printf("  [OK] Entry successful!\n");
        printf("       Allocated slot : %s (%s)\n", label, typeName(vehicles[idx].type));
        printf("       Entry time     : %s\n", nowBuf);
    } else {
        /* lot full for this type: the waiting queue takes over     */
        enqueueVehicle(regNo, now);
        saveQueue();
        printf("  [!] Sorry, no free slot is available for a %s right now.\n",
               typeName(vehicles[idx].type));
        printf("       Vehicle %s was added to the WAITING QUEUE at position %d.\n",
               regNo, queuePosition(regNo));
        printf("       A suitable slot will be allotted automatically\n"
               "       as soon as one becomes free.\n");
    }
}

/*---------------------------------------------------------------------
 |  Operator option 3 : vehicle exit + fee calculation
 *-------------------------------------------------------------------*/
void vehicleExit(void)
{
    char regNo[16], label[6], answer[8];
    int s, idx;
    time_t entry, now;
    double fee;
    int pos;

    printTitle("VEHICLE EXIT AND FEE CALCULATION");
    readString("Enter vehicle registration number : ", regNo, (int)sizeof regNo);
    toUpperStr(regNo);

    s = findSlotByReg(regNo);
    if (s < 0) {
        /* not parked: maybe it is still waiting in the queue       */
        pos = queuePosition(regNo);
        if (pos > 0) {
            printf("  [i] Vehicle %s is in the waiting queue (position %d),\n",
                   regNo, pos);
            printf("      it has not parked yet.\n");
            readString("      Remove it from the waiting queue? (yes/no) : ",
                       answer, (int)sizeof answer);
            if (isYes(answer)) {
                removeFromQueue(regNo);
                saveQueue();
                printf("  [OK] Vehicle removed from the waiting queue.\n");
            } else {
                printf("      Vehicle is still waiting for a slot.\n");
            }
        } else {
            printf("  [!] Vehicle %s is not currently parked in this lot.\n", regNo);
        }
        return;
    }

    /* billing: duration + fee, printed as a receipt                */
    entry = slots[s].entryTime;
    now   = time(NULL);
    fee   = calculateFee(slots[s].type, entry, now);
    idx   = findVehicleIndex(regNo);
    printReceipt(regNo,
                 (idx >= 0) ? vehicles[idx].ownerName : "(unknown)",
                 slots[s].type, slots[s].slotId, entry, now, fee);

    readString("Confirm payment collected and allow exit? (yes/no) : ",
               answer, (int)sizeof answer);
    if (!isYes(answer)) {
        printf("  [i] Exit cancelled. The slot remains occupied.\n");
        return;
    }

    /* record the transaction, free the slot, save both             */
    appendTransaction(regNo, slots[s].slotId, entry, now, fee);
    slotLabel(slots[s].slotId, label);
    slots[s].occupied  = 0;
    slots[s].regNo[0]  = '\0';
    slots[s].entryTime = 0;
    saveSlots();
    printf("  [OK] Payment recorded. Slot %s is now FREE.\n", label);

    /* the freed slot may be needed by a waiting vehicle            */
    assignWaitingVehicle(s);
}
