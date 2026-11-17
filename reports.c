/*=====================================================================
 |  ParkEase - MODULE: REPORTS                               (reports.c)
 |
 |  Read-only views over the data for the menus:
 |    - currently parked vehicles (selection sort by entry time)
 |    - all registered vehicles with live status
 |    - transaction history (selection sort by fee, highest first)
 |    - revenue summary by vehicle type
 |    - vehicle statistics for the administrator
 |  Transaction data is loaded through the File Handling Module.
 *=====================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parkease.h"
#include "reports.h"
#include "vehicle.h"
#include "slots.h"
#include "queue.h"
#include "billing.h"
#include "fileio.h"

/*---------------------------------------------------------------------
 |  Menu option: parked vehicles, sorted by entry time (oldest first)
 *-------------------------------------------------------------------*/
void showParkedVehicles(void)
{
    int order[TOTAL_SLOTS];
    int i, j, n = 0, minIdx, tmp;
    int vi;
    char label[6], whenBuf[20];

    printTitle("CURRENTLY PARKED VEHICLES");
    for (i = 0; i < TOTAL_SLOTS; i++)
        if (slots[i].occupied) order[n++] = i;

    if (n == 0) {
        printf("  No vehicle is parked right now.\n");
        return;
    }

    /* selection sort on entry time (ascending) */
    for (i = 0; i < n - 1; i++) {
        minIdx = i;
        for (j = i + 1; j < n; j++)
            if (slots[order[j]].entryTime < slots[order[minIdx]].entryTime)
                minIdx = j;
        if (minIdx != i) { tmp = order[i]; order[i] = order[minIdx]; order[minIdx] = tmp; }
    }

    printf("  %-6s %-18s %-13s %-14s %s\n",
           "Slot", "Type", "Reg. No", "Owner", "Entry time");
    printf("  ---------------------------------------------------------------\n");
    for (i = 0; i < n; i++) {
        vi = findVehicleIndex(slots[order[i]].regNo);
        slotLabel(slots[order[i]].slotId, label);
        formatTime(slots[order[i]].entryTime, whenBuf);
        printf("  %-6s %-18s %-13s %-14s %s\n",
               label, typeName(slots[order[i]].type), slots[order[i]].regNo,
               (vi >= 0) ? vehicles[vi].ownerName : "(unknown)", whenBuf);
    }
    printf("  ---------------------------------------------------------------\n");
    printf("  Total parked vehicles : %d\n", n);
}

/*---------------------------------------------------------------------
 |  Admin option: list all registered vehicles with live status
 *-------------------------------------------------------------------*/
void adminViewVehicles(void)
{
    int i;
    char status[100];
    printTitle("REGISTERED VEHICLES (ADMIN)");
    if (vehicleCount == 0) {
        printf("  No vehicle has been registered yet.\n");
        return;
    }
    printf("  %-13s %-16s %-20s %s\n",
           "Reg. No", "Owner", "Type", "Status");
    printf("  ---------------------------------------------------------------\n");
    for (i = 0; i < vehicleCount; i++) {
        vehicleStatusText(vehicles[i].regNo, status, (int)sizeof status);
        printf("  %-13s %-16s %-20s %s\n",
               vehicles[i].regNo, vehicles[i].ownerName,
               typeName(vehicles[i].type), status);
    }
    printf("  ---------------------------------------------------------------\n");
    printf("  Total registered vehicles : %d\n", vehicleCount);
}

/*---------------------------------------------------------------------
 |  Admin option: full transaction history, sorted by fee (highest
 |  first) - the fee keeps its signed %8.2f format for the sort demo.
 *-------------------------------------------------------------------*/
void adminViewTransactions(void)
{
    int count = 0, i, j, minIdx;
    struct Transaction *list;
    struct Transaction tmp;
    double total = 0.0;
    long hours, minsPart, minutes;
    char label[6], entryBuf[20], exitBuf[20];

    printTitle("TRANSACTION HISTORY (ADMIN)");
    list = loadTransactions(&count);
    if (list == NULL || count == 0) {
        printf("  No transactions have been recorded yet.\n");
        free(list);
        return;
    }

    /* selection sort by amount, descending */
    for (i = 0; i < count - 1; i++) {
        minIdx = i;
        for (j = i + 1; j < count; j++)
            if (list[j].amount > list[minIdx].amount) minIdx = j;
        if (minIdx != i) { tmp = list[i]; list[i] = list[minIdx]; list[minIdx] = tmp; }
    }

    printf("  %-13s %-5s %-16s %-16s %8s\n",
           "Reg. No", "Slot", "Entry", "Exit", "Fee Rs.");
    printf("  ------------------------------------------------------------\n");
    for (i = 0; i < count; i++) {
        slotLabel(list[i].slotId, label);
        formatTime(list[i].entryTime, entryBuf);
        formatTime(list[i].exitTime, exitBuf);
        minutes  = durationMinutes(list[i].entryTime, list[i].exitTime);
        hours    = minutes / 60;
        minsPart = minutes % 60;
        printf("  %-13s %-5s %-16s %-16s %8.2f\n",
               list[i].regNo, label, entryBuf, exitBuf, list[i].amount);
        printf("        duration : %ldh %02ldm\n", hours, minsPart);
        total += list[i].amount;
    }
    printf("  ------------------------------------------------------------\n");
    printf("  Transactions : %d      Total revenue : Rs. %.2f\n", count, total);
    free(list);
}

/*---------------------------------------------------------------------
 |  Admin option: revenue summary by vehicle type
 *-------------------------------------------------------------------*/
void adminRevenue(void)
{
    int count = 0, i, vi;
    struct Transaction *list;
    double total = 0.0;
    double byType[4] = {0.0, 0.0, 0.0, 0.0};     /* index 0 = unknown   */
    int byCount[4]  = {0, 0, 0, 0};

    printTitle("REVENUE SUMMARY (ADMIN)");
    list = loadTransactions(&count);
    if (list == NULL || count == 0) {
        printf("  No transactions have been recorded yet.\n");
        free(list);
        return;
    }
    for (i = 0; i < count; i++) {
        vi = findVehicleIndex(list[i].regNo);
        if (vi >= 0) { byType[vehicles[vi].type] += list[i].amount; byCount[vehicles[vi].type]++; }
        else         { byType[0] += list[i].amount; byCount[0]++; }
        total += list[i].amount;
    }
    printf("  Total transactions : %d\n", count);
    printf("  Total revenue      : Rs. %.2f\n", total);
    printf("  -----------------------------------------\n");
    printf("  By vehicle type:\n");
    printf("    Bike          : Rs. %10.2f  (%d vehicles)\n", byType[TYPE_BIKE],  byCount[TYPE_BIKE]);
    printf("    Car           : Rs. %10.2f  (%d vehicles)\n", byType[TYPE_CAR],   byCount[TYPE_CAR]);
    printf("    Heavy Vehicle : Rs. %10.2f  (%d vehicles)\n", byType[TYPE_HEAVY], byCount[TYPE_HEAVY]);
    printf("    Other/Unknown : Rs. %10.2f  (%d vehicles)\n", byType[0], byCount[0]);
    free(list);
}

/*---------------------------------------------------------------------
 |  Admin option: vehicle statistics - the live state of the whole
 |  lot in one screen (registered, parked, free slots, queue, money).
 *-------------------------------------------------------------------*/
void adminVehicleStatistics(void)
{
    int i, parkedBike = 0, parkedCar = 0, parkedHeavy = 0;
    int count = 0, regBike = 0, regCar = 0, regHeavy = 0;
    struct Transaction *list;
    double total = 0.0;

    printTitle("VEHICLE STATISTICS (ADMIN)");

    /* registered vehicles by type */
    for (i = 0; i < vehicleCount; i++) {
        if      (vehicles[i].type == TYPE_BIKE)  regBike++;
        else if (vehicles[i].type == TYPE_CAR)   regCar++;
        else if (vehicles[i].type == TYPE_HEAVY) regHeavy++;
    }

    /* currently parked vehicles by type */
    for (i = 0; i < TOTAL_SLOTS; i++) {
        if (!slots[i].occupied) continue;
        if      (slots[i].type == TYPE_BIKE)  parkedBike++;
        else if (slots[i].type == TYPE_CAR)   parkedCar++;
        else if (slots[i].type == TYPE_HEAVY) parkedHeavy++;
    }

    /* lifetime figures from the transaction file */
    list = loadTransactions(&count);
    if (list != NULL) {
        for (i = 0; i < count; i++) total += list[i].amount;
        free(list);
    }

    printf("  Registered vehicles : %3d  [Bike %d | Car %d | Heavy %d]\n",
           vehicleCount, regBike, regCar, regHeavy);
    printf("  Currently parked    : %3d  [Bike %d | Car %d | Heavy %d]\n",
           parkedBike + parkedCar + parkedHeavy,
           parkedBike, parkedCar, parkedHeavy);
    printf("  Free slots          : %3d  [Bike %d/%d | Car %d/%d | Heavy %d/%d]\n",
           freeSlotsOfType(TYPE_BIKE) + freeSlotsOfType(TYPE_CAR) +
               freeSlotsOfType(TYPE_HEAVY),
           freeSlotsOfType(TYPE_BIKE),  TOTAL_BIKE_SLOTS,
           freeSlotsOfType(TYPE_CAR),   TOTAL_CAR_SLOTS,
           freeSlotsOfType(TYPE_HEAVY), TOTAL_HEAVY_SLOTS);
    printf("  Waiting in queue    : %3d\n", queueLength());
    printf("  -------------------------------------------\n");
    printf("  Transactions recorded : %d\n", count);
    printf("  Lifetime revenue      : Rs. %.2f\n", total);
}
