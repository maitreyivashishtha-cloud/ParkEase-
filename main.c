/*=====================================================================
 |  ParkEase - MODULE: MAIN MENU                              (main.c)
 |
 |  The entry point of the program. It owns the role-based menus
 |  shown in the architecture slides:
 |
 |      Main Menu
 |        |-- Operator Menu    (daily work at the gate)
 |        |     1. Register a new vehicle
 |        |     2. Vehicle entry   (allocate a slot)
 |        |     3. Vehicle exit    (calculate parking fee)
 |        |     4. Parking lot status  ([X] / [ ] grid)
 |        |     5. Search vehicle
 |        |     6. View waiting queue
 |        |     7. View fee rates
 |        |
 |        `-- Administrator Menu   (password protected)
 |              1. Manage parking
 |              2. View transaction history
 |              3. Revenue report
 |              4. Vehicle statistics
 |
 |  At startup it loads all four data files through the File
 |  Handling Module; the whole lot is created on the first run.
 |
 |  HOW TO BUILD (see README.md and the Makefile):
 |      gcc -Wall -Wextra -O2 -o parkease *.c
 |      ./parkease            (on Windows: parkease.exe)
 *=====================================================================*/

#include <stdio.h>
#include <string.h>
#include "parkease.h"
#include "vehicle.h"
#include "slots.h"
#include "entryexit.h"
#include "billing.h"
#include "queue.h"
#include "reports.h"
#include "fileio.h"

/*---------------------------------------------------------------------
 |  Startup: load everything; create the lot on the very first run
 *-------------------------------------------------------------------*/
static void loadAll(void)
{
    loadVehicles();
    if (!loadSlots()) {
        initializeSlots();
        saveSlots();                 /* first run: create the lot      */
    }
    loadQueue();
    ensureFile(DATA_VEHICLES);       /* create any file still missing  */
    ensureFile(DATA_TXN);
    ensureFile(DATA_QUEUE);
}

/* One line of live lot status for the menu headers */
static void printLotStatusLine(void)
{
    printf("  Lot status : %d free / %d total   "
           "[Bike %d/%d | Car %d/%d | Heavy %d/%d]\n",
           freeSlotsOfType(TYPE_BIKE) + freeSlotsOfType(TYPE_CAR) +
               freeSlotsOfType(TYPE_HEAVY),
           TOTAL_SLOTS,
           freeSlotsOfType(TYPE_BIKE),  TOTAL_BIKE_SLOTS,
           freeSlotsOfType(TYPE_CAR),   TOTAL_CAR_SLOTS,
           freeSlotsOfType(TYPE_HEAVY), TOTAL_HEAVY_SLOTS);
}

/*---------------------------------------------------------------------
 |  MAIN MENU (role selection, as in the architecture slide)
 *-------------------------------------------------------------------*/
static void printMainMenu(void)
{
    printf("\n");
    printf("=====================================================\n");
    printf("                P A R K E A S E\n");
    printf("      Smart Parking Lot Management System\n");
    printf("=====================================================\n");
    printLotStatusLine();
    printf("-----------------------------------------------------\n");
    printf("   Login as:\n");
    printf("   1. Operator      (vehicle entry, exit, slots, search)\n");
    printf("   2. Administrator (manage parking, reports - password)\n");
    printf("   0. Save and exit\n");
    printf("-----------------------------------------------------\n");
}

/*---------------------------------------------------------------------
 |  OPERATOR MENU - the four daily tasks of the parking operator
 *-------------------------------------------------------------------*/
static void operatorMenu(void)
{
    int choice;
    do {
        printTitle("OPERATOR MENU");
        printLotStatusLine();
        printf("-----------------------------------------------------\n");
        printf("   1. Register a new vehicle\n");
        printf("   2. Vehicle entry  (allocate a slot)\n");
        printf("   3. Vehicle exit   (calculate parking fee)\n");
        printf("   4. Parking lot status (grid view)\n");
        printf("   5. Search vehicle status\n");
        printf("   6. View waiting queue\n");
        printf("   7. View parking fee rates\n");
        printf("   0. Logout (back to main menu)\n");
        printf("-----------------------------------------------------\n");
        choice = readInt("  Enter your choice : ", 0, 7);
        switch (choice) {
            case 1: registerVehicle();      pauseScreen(); break;
            case 2: vehicleEntry();         pauseScreen(); break;
            case 3: vehicleExit();          pauseScreen(); break;
            case 4: showAvailabilityGrid(); pauseScreen(); break;
            case 5: searchVehicle();        pauseScreen(); break;
            case 6: displayQueue();         pauseScreen(); break;
            case 7: showFeeRates();         pauseScreen(); break;
            case 0: printf("  [i] Operator logged out.\n"); break;
            default: break;
        }
    } while (choice != 0);
}

/*---------------------------------------------------------------------
 |  ADMIN LOGIN - returns 1 if the password was accepted (3 tries)
 *-------------------------------------------------------------------*/
static int adminLogin(void)
{
    char pass[40];
    int attempt;
    printTitle("ADMIN LOGIN");
    for (attempt = 1; attempt <= 3; attempt++) {
        readString("Enter admin password : ", pass, (int)sizeof pass);
        if (strcmp(pass, ADMIN_PASSWORD) == 0) {
            printf("  [OK] Access granted. Welcome, admin!\n");
            return 1;
        }
        printf("  [!] Incorrect password (attempt %d of 3).\n", attempt);
    }
    printf("  [!] Too many failed attempts. Returning to the main menu.\n");
    return 0;
}

/*---------------------------------------------------------------------
 |  MANAGE PARKING - the administrator's slot control panel
 *-------------------------------------------------------------------*/
static void manageParkingMenu(void)
{
    int choice;
    do {
        printTitle("MANAGE PARKING (ADMIN)");
        printf("   1. Parking lot status (grid view)\n");
        printf("   2. Slot details table (who is where)\n");
        printf("   3. Currently parked vehicles (by entry time)\n");
        printf("   4. List all registered vehicles\n");
        printf("   5. Manually release a slot (no billing)\n");
        printf("   6. Reset ALL data\n");
        printf("   0. Back to admin menu\n");
        printf("-----------------------------------------------------\n");
        choice = readInt("  Enter your choice : ", 0, 6);
        switch (choice) {
            case 1: showAvailabilityGrid(); pauseScreen(); break;
            case 2: slotDetailsTable();     pauseScreen(); break;
            case 3: showParkedVehicles();   pauseScreen(); break;
            case 4: adminViewVehicles();    pauseScreen(); break;
            case 5: adminReleaseSlot();     pauseScreen(); break;
            case 6: adminResetData();       pauseScreen(); break;
            case 0: break;
            default: break;
        }
    } while (choice != 0);
}

/*---------------------------------------------------------------------
 |  ADMIN MENU - manage parking, transactions, revenue, statistics
 *-------------------------------------------------------------------*/
static void adminMenu(void)
{
    int choice;
    if (!adminLogin()) return;
    do {
        printTitle("ADMIN MENU");
        printf("   1. Manage parking\n");
        printf("   2. View transaction history\n");
        printf("   3. Revenue report\n");
        printf("   4. Vehicle statistics\n");
        printf("   0. Logout (back to main menu)\n");
        printf("-----------------------------------------------------\n");
        choice = readInt("  Enter your choice : ", 0, 4);
        switch (choice) {
            case 1: manageParkingMenu();        break;
            case 2: adminViewTransactions();    pauseScreen(); break;
            case 3: adminRevenue();             pauseScreen(); break;
            case 4: adminVehicleStatistics();   pauseScreen(); break;
            case 0: printf("  [i] Admin logged out.\n"); break;
            default: break;
        }
    } while (choice != 0);
}

/*---------------------------------------------------------------------
 |  main(): startup, then the role-selection loop
 *-------------------------------------------------------------------*/
int main(void)
{
    int choice;
    loadAll();

    printf("=====================================================\n");
    printf("   Welcome to ParkEase - Smart Parking Lot Manager!\n");
    printf("=====================================================\n");
    printf("  Data is stored in the current folder:\n");
    printf("    %s, %s,\n", DATA_VEHICLES, DATA_SLOTS);
    printf("    %s, %s\n", DATA_TXN, DATA_QUEUE);
    printf("  Default admin password : %s (change it in parkease.h)\n",
           ADMIN_PASSWORD);

    do {
        printMainMenu();
        choice = readInt("  Enter your choice : ", 0, 2);
        switch (choice) {
            case 1: operatorMenu(); break;
            case 2: adminMenu();    break;
            case 0: break;
            default: break;
        }
    } while (choice != 0);

    printf("\n  All data has been saved. Thank you for using ParkEase!\n");
    printf("=====================================================\n");
    return 0;
}
