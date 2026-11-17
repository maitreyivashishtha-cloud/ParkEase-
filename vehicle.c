/*=====================================================================
 |  ParkEase - MODULE: VEHICLE MANAGEMENT                  (vehicle.c)
 |
 |  Registration and searching of vehicles. The records live in the
 |  shared vehicles[] array; saving is delegated to the File Handling
 |  Module (fileio.h). To report "where is this vehicle now" the
 |  module cooperates with Parking Slot Management and the Waiting
 |  Queue Module - exactly like the arrows in the block diagram.
 *=====================================================================*/

#include <stdio.h>
#include <string.h>
#include "parkease.h"
#include "vehicle.h"
#include "slots.h"
#include "queue.h"
#include "fileio.h"

/*---------------------------------------------------------------------
 |  Linear search: index of a registered vehicle, or -1
 *-------------------------------------------------------------------*/
int findVehicleIndex(const char *regNo)
{
    int i;
    for (i = 0; i < vehicleCount; i++)
        if (strcmp(vehicles[i].regNo, regNo) == 0) return i;
    return -1;
}

/*---------------------------------------------------------------------
 |  Append a vehicle record and save it immediately.
 |  Returns 1 on success, 0 if the database is full.
 *-------------------------------------------------------------------*/
int addVehicle(const char *regNo, const char *ownerName, int type)
{
    if (vehicleCount >= MAX_VEHICLES) {
        printf("  [!] The vehicle database is full (%d records).\n",
               MAX_VEHICLES);
        return 0;
    }
    strcpy(vehicles[vehicleCount].regNo, regNo);
    strcpy(vehicles[vehicleCount].ownerName, ownerName);
    vehicles[vehicleCount].type = type;
    vehicleCount++;
    saveVehicles();
    return 1;
}

/*---------------------------------------------------------------------
 |  Text describing where a vehicle currently is
 *-------------------------------------------------------------------*/
void vehicleStatusText(const char *regNo, char *out, int size)
{
    int s = findSlotByReg(regNo);
    int pos;
    char label[6];
    char whenBuf[20];
    if (s >= 0) {
        slotLabel(slots[s].slotId, label);
        formatTime(slots[s].entryTime, whenBuf);
        snprintf(out, (size_t)size, "PARKED at slot %s since %s", label, whenBuf);
        return;
    }
    pos = queuePosition(regNo);
    if (pos > 0) snprintf(out, (size_t)size, "WAITING in queue at position %d", pos);
    else         snprintf(out, (size_t)size, "Not parked right now");
}

/*---------------------------------------------------------------------
 |  Operator option 1 : register a new vehicle
 *-------------------------------------------------------------------*/
void registerVehicle(void)
{
    char regNo[16], owner[32];
    int idx, type;
    printTitle("VEHICLE REGISTRATION");
    if (vehicleCount >= MAX_VEHICLES) {
        printf("  [!] The vehicle database is full (%d records).\n", MAX_VEHICLES);
        return;
    }
    readString("Enter vehicle registration number : ", regNo, (int)sizeof regNo);
    toUpperStr(regNo);
    idx = findVehicleIndex(regNo);
    if (idx >= 0) {
        char status[100];
        vehicleStatusText(regNo, status, (int)sizeof status);
        printf("  [!] Vehicle %s is ALREADY registered (owner: %s).\n",
               regNo, vehicles[idx].ownerName);
        printf("      Current status : %s\n", status);
        return;
    }
    readName ("Enter owner name                  : ", owner, (int)sizeof owner);
    type = readInt("Vehicle type  [1] Bike  [2] Car  [3] Heavy : ", 1, 3);

    if (!addVehicle(regNo, owner, type)) return;   /* saves the file  */

    printf("\n  [OK] Vehicle registered successfully!\n");
    printf("       Reg. No   : %s\n", regNo);
    printf("       Owner     : %s\n", owner);
    printf("       Type      : %s\n", typeName(type));
}

/*---------------------------------------------------------------------
 |  Operator option 5 : search a vehicle by registration number
 *-------------------------------------------------------------------*/
void searchVehicle(void)
{
    char regNo[16], status[100];
    int idx;
    printTitle("SEARCH VEHICLE");
    readString("Enter vehicle registration number : ", regNo, (int)sizeof regNo);
    toUpperStr(regNo);
    idx = findVehicleIndex(regNo);
    if (idx < 0) {
        printf("  [!] No vehicle with registration number %s was found.\n", regNo);
        return;
    }
    vehicleStatusText(regNo, status, (int)sizeof status);
    printf("\n  Vehicle found!\n");
    printf("  -------------------------------------------\n");
    printf("   Reg. No    : %s\n", vehicles[idx].regNo);
    printf("   Owner      : %s\n", vehicles[idx].ownerName);
    printf("   Type       : %s\n", typeName(vehicles[idx].type));
    printf("   Status     : %s\n", status);
    printf("  -------------------------------------------\n");
}
