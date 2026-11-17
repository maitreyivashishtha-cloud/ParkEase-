/*=====================================================================
 |  ParkEase - Smart Parking Lot Management System
 |  Course Project : Programming with C (First Year CSE)
 |  Team           : 5 members
 |
 |  A menu-driven, terminal-based parking lot manager that supports:
 |    1. Vehicle registration and searching
 |    2. Vehicle entry with automatic slot allocation
 |    3. Vehicle exit with fee calculation (using <time.h>)
 |    4. Parking slot availability monitoring
 |    5. Waiting queue management (dynamic queue: pointers + malloc)
 |    6. Permanent record storage (binary file handling)
 |    7. Admin login with transaction history and revenue summary
 |
 |  HOW TO COMPILE AND RUN
 |      gcc -Wall -Wextra -O2 -o parkease parkEase.c
 |      ./parkease            (on Windows: parkease.exe)
 |
 |  DATA FILES (created automatically in the current folder)
 |      vehicles.dat      - registered vehicle records
 |      slots.dat         - all parking slots and their live status
 |      transactions.dat  - completed parking transactions
 |      queue.dat         - vehicles waiting for a free slot
 |
 |  ADMIN PASSWORD : admin123   (change it in the CONFIG section)
 |  FEE RATES      : edit the CONFIG section below and recompile
 *=====================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/*=====================================================================
 |  CONFIG SECTION - the whole team can tune the system from here
 *=====================================================================*/

/* Number of parking slots for each vehicle type */
#define TOTAL_BIKE_SLOTS    20
#define TOTAL_CAR_SLOTS     15
#define TOTAL_HEAVY_SLOTS    5
#define TOTAL_SLOTS (TOTAL_BIKE_SLOTS + TOTAL_CAR_SLOTS + TOTAL_HEAVY_SLOTS)

/* Fee rates (in Rs.). "First rate" covers the 1st hour, "hourly rate"
   is charged for every additional hour (any partial hour counts as a
   full hour). A stay shorter than GRACE_MINUTES is free.             */
#define BIKE_FIRST_RATE     10.0
#define BIKE_HOURLY_RATE     5.0
#define CAR_FIRST_RATE      20.0
#define CAR_HOURLY_RATE     10.0
#define HEAVY_FIRST_RATE    30.0
#define HEAVY_HOURLY_RATE   15.0
#define GRACE_MINUTES       15

#define ADMIN_PASSWORD      "admin123"
#define MAX_VEHICLES        500

/* Data file names */
#define DATA_VEHICLES   "vehicles.dat"
#define DATA_SLOTS      "slots.dat"
#define DATA_TXN        "transactions.dat"
#define DATA_QUEUE      "queue.dat"

/* Vehicle type codes */
#define TYPE_BIKE   1
#define TYPE_CAR    2
#define TYPE_HEAVY  3

/*=====================================================================
 |  MODULE 1 : DATA STRUCTURES AND GLOBAL STATE
 *=====================================================================*/

/* A registered vehicle */
struct Vehicle {
    char   regNo[16];        /* registration number, e.g. "MH12AB1234" */
    char   ownerName[32];    /* owner's full name                      */
    int    type;             /* TYPE_BIKE / TYPE_CAR / TYPE_HEAVY      */
};

/* One parking slot. A slot only accepts vehicles of its own type. */
struct Slot {
    int    slotId;           /* 1..TOTAL_SLOTS                         */
    int    type;             /* vehicle type the slot can hold         */
    int    occupied;         /* 1 = occupied, 0 = free                 */
    char   regNo[16];        /* reg. no. of parked vehicle ("" if free)*/
    time_t entryTime;        /* when the vehicle entered (time.h)      */
};

/* One completed parking transaction (saved when a vehicle exits) */
struct Transaction {
    char   regNo[16];
    int    slotId;
    time_t entryTime;
    time_t exitTime;
    double amount;           /* parking fee collected in Rs.           */
};

/* Node of the dynamic waiting queue (linked list + malloc) */
struct QueueNode {
    char   regNo[16];
    time_t waitTime;         /* when the vehicle joined the queue      */
    struct QueueNode *next;
};

/* Layout used to save the waiting queue to a file */
struct QueueSave {
    char   regNo[16];
    time_t waitTime;
};

/* ------------------ global state ------------------ */
static struct Vehicle vehicles[MAX_VEHICLES];   /* registered vehicles  */
static int vehicleCount = 0;

static struct Slot slots[TOTAL_SLOTS];          /* all parking slots    */

static struct QueueNode *queueFront = NULL;     /* waiting queue        */
static struct QueueNode *queueRear  = NULL;

/* ------------------ function prototypes ------------------ */
static void  readLine(char *buf, int size);
static void  trimStr(char *s);
static void  readString(const char *prompt, char *out, int size);
static void  readName(const char *prompt, char *out, int size);
static int   readInt(const char *prompt, int min, int max);
static void  toUpperStr(char *s);
static int   isYes(const char *s);
static void  pauseScreen(void);
static const char *typeName(int type);
static void  slotLabel(int slotId, char *out);
static void  formatTime(time_t t, char *out);
static void  printTitle(const char *title);
static int   freeSlotsOfType(int type);
static void  saveVehicles(void);
static void  loadVehicles(void);
static void  saveSlots(void);
static int   loadSlots(void);
static void  appendTransaction(const char *regNo, int slotId,
                               time_t entryTime, time_t exitTime, double amount);
static struct Transaction *loadTransactions(int *countOut);
static void  enqueueVehicle(const char *regNo, time_t waitTime);
static int   queuePosition(const char *regNo);
static void  unlinkNode(struct QueueNode *prev, struct QueueNode *node);
static void  removeFromQueue(const char *regNo);
static void  freeQueue(void);
static void  saveQueue(void);
static void  loadQueue(void);
static void  displayQueue(void);
static int   assignWaitingVehicle(int slotIndex);
static int   findVehicleIndex(const char *regNo);
static int   findSlotByReg(const char *regNo);
static int   findFreeSlotIndex(int type);
static void  vehicleStatusText(const char *regNo, char *out, int size);
static void  registerVehicle(void);
static void  searchVehicle(void);
static long  durationMinutes(time_t entryTime, time_t exitTime);
static double calculateFee(int type, time_t entryTime, time_t exitTime);
static void  vehicleEntry(void);
static void  vehicleExit(void);
static void  showAvailability(void);
static void  showParkedVehicles(void);
static void  showFeeRates(void);
static int   adminLogin(void);
static void  adminViewVehicles(void);
static void  adminViewTransactions(void);
static void  adminRevenue(void);
static void  adminResetData(void);
static void  adminMenu(void);
static void  initializeSlots(void);
static void  loadAll(void);
static void  printMainMenu(void);
static void  ensureFile(const char *name);

/*=====================================================================
 |  MODULE 2 : GENERAL UTILITY FUNCTIONS (input helpers, formatting)
 *=====================================================================*/

/* Read one line of text from the keyboard (safe against overflow).
   If the input stream closes (Ctrl+D / Ctrl+Z), exit the program.   */
static void readLine(char *buf, int size)
{
    int len;
    if (fgets(buf, size, stdin) == NULL) {
        printf("\n[!] Input stream closed. Saving is already done. Bye!\n");
        exit(0);
    }
    len = (int)strcspn(buf, "\n");
    buf[len] = '\0';                       /* remove the newline       */
}

/* Remove leading and trailing blank characters from a string */
static void trimStr(char *s)
{
    char *start = s;
    size_t len;
    while (*start != '\0' && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

/* Ask until the user types a non-empty line without spaces */
static void readString(const char *prompt, char *out, int size)
{
    char buf[128];
    for (;;) {
        printf("%s", prompt);
        readLine(buf, (int)sizeof buf);
        trimStr(buf);
        if (buf[0] != '\0' && strlen(buf) < (size_t)size &&
            strcspn(buf, " \t") == strlen(buf)) {
            strcpy(out, buf);
            return;
        }
        printf("  [!] Please enter text up to %d characters, no spaces.\n",
               size - 1);
    }
}

/* Ask until the user types a non-empty line (spaces are allowed) */
static void readName(const char *prompt, char *out, int size)
{
    char buf[128];
    for (;;) {
        printf("%s", prompt);
        readLine(buf, (int)sizeof buf);
        trimStr(buf);
        if (buf[0] != '\0' && strlen(buf) < (size_t)size) {
            strcpy(out, buf);
            return;
        }
        printf("  [!] Please enter a name up to %d characters.\n", size - 1);
    }
}

/* Ask until the user types a valid integer in [min, max] */
static int readInt(const char *prompt, int min, int max)
{
    char buf[64];
    char *end;
    long value;
    for (;;) {
        printf("%s", prompt);
        readLine(buf, (int)sizeof buf);
        trimStr(buf);
        value = strtol(buf, &end, 10);
        if (end != buf && *end == '\0' && value >= min && value <= max)
            return (int)value;
        printf("  [!] Invalid input. Enter a number between %d and %d.\n",
               min, max);
    }
}

/* Convert a whole string to upper case (for registration numbers) */
static void toUpperStr(char *s)
{
    for (; *s != '\0'; s++) *s = (char)toupper((unsigned char)*s);
}

/* Accept "y", "Y", "yes", "YES" ... as true */
static int isYes(const char *s)
{
    char low[8];
    size_t i;
    for (i = 0; i < sizeof low - 1 && s[i] != '\0'; i++)
        low[i] = (char)tolower((unsigned char)s[i]);
    low[i] = '\0';
    return (strcmp(low, "y") == 0 || strcmp(low, "yes") == 0);
}

/* Pause until the user presses ENTER */
static void pauseScreen(void)
{
    char dummy[8];
    printf("\nPress ENTER to return to the menu...");
    readLine(dummy, (int)sizeof dummy);
}

/* Human-readable name of a vehicle type */
static const char *typeName(int type)
{
    switch (type) {
        case TYPE_BIKE:  return "Bike (Two-Wheeler)";
        case TYPE_CAR:   return "Car";
        case TYPE_HEAVY: return "Heavy Vehicle";
        default:         return "Unknown";
    }
}

/* Short slot label, e.g. slot 7 -> "S07" */
static void slotLabel(int slotId, char *out /* size >= 6 */)
{
    sprintf(out, "S%02d", slotId);
}

/* Format a time_t value as "dd-mm-yyyy hh:mm" */
static void formatTime(time_t t, char *out /* size >= 20 */)
{
    struct tm *tmPtr = localtime(&t);
    if (tmPtr == NULL) { strcpy(out, "?"); return; }
    strftime(out, 20, "%d-%m-%Y %H:%M", tmPtr);
}

/* Print a section heading */
static void printTitle(const char *title)
{
    printf("\n=====================================================\n");
    printf("   %s\n", title);
    printf("=====================================================\n");
}

/* Count free slots of one vehicle type */
static int freeSlotsOfType(int type)
{
    int i, freeCount = 0;
    for (i = 0; i < TOTAL_SLOTS; i++)
        if (slots[i].type == type && !slots[i].occupied) freeCount++;
    return freeCount;
}

/*=====================================================================
 |  MODULE 3 : FILE HANDLING (permanent storage of all records)
 *=====================================================================*/

static void saveVehicles(void)
{
    FILE *fp = fopen(DATA_VEHICLES, "wb");
    if (fp == NULL) { printf("  [!] Cannot write %s\n", DATA_VEHICLES); return; }
    fwrite(vehicles, sizeof(struct Vehicle), (size_t)vehicleCount, fp);
    fclose(fp);
}

static void loadVehicles(void)
{
    FILE *fp = fopen(DATA_VEHICLES, "rb");
    if (fp == NULL) return;                       /* first run: no file  */
    while (vehicleCount < MAX_VEHICLES &&
           fread(&vehicles[vehicleCount], sizeof(struct Vehicle), 1, fp) == 1)
        vehicleCount++;
    fclose(fp);
}

static void saveSlots(void)
{
    FILE *fp = fopen(DATA_SLOTS, "wb");
    if (fp == NULL) { printf("  [!] Cannot write %s\n", DATA_SLOTS); return; }
    fwrite(slots, sizeof(struct Slot), TOTAL_SLOTS, fp);
    fclose(fp);
}

/* Returns 1 if a slot file existed and was loaded, 0 otherwise */
static int loadSlots(void)
{
    FILE *fp = fopen(DATA_SLOTS, "rb");
    if (fp == NULL) return 0;
    if (fread(slots, sizeof(struct Slot), TOTAL_SLOTS, fp) != TOTAL_SLOTS) {
        fclose(fp);
        return 0;                                 /* corrupt: re-init    */
    }
    fclose(fp);
    return 1;
}

static void appendTransaction(const char *regNo, int slotId,
                              time_t entryTime, time_t exitTime, double amount)
{
    struct Transaction t;
    FILE *fp = fopen(DATA_TXN, "ab");
    if (fp == NULL) { printf("  [!] Cannot write %s\n", DATA_TXN); return; }
    memset(&t, 0, sizeof t);
    strcpy(t.regNo, regNo);
    t.slotId    = slotId;
    t.entryTime = entryTime;
    t.exitTime  = exitTime;
    t.amount    = amount;
    fwrite(&t, sizeof t, 1, fp);
    fclose(fp);
}

/* Load a transaction file into a malloc-ed array.
   Sets *countOut. Returns NULL if there is no file or it is empty.  */
static struct Transaction *loadTransactions(int *countOut)
{
    FILE *fp;
    long size;
    int count, i;
    struct Transaction *list;

    *countOut = 0;
    fp = fopen(DATA_TXN, "rb");
    if (fp == NULL) return NULL;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    count = (int)(size / (long)sizeof(struct Transaction));
    if (count <= 0) { fclose(fp); return NULL; }
    list = (struct Transaction *)malloc((size_t)count * sizeof(struct Transaction));
    if (list == NULL) { fclose(fp); return NULL; }
    for (i = 0; i < count; i++)
        if (fread(&list[i], sizeof(struct Transaction), 1, fp) != 1) {
            count = i;
            break;
        }
    fclose(fp);
    *countOut = count;
    return list;
}

/*=====================================================================
 |  MODULE 4 : WAITING QUEUE (dynamic queue with pointers + malloc)
 *=====================================================================*/

/* Add a vehicle at the REAR of the waiting queue */
static void enqueueVehicle(const char *regNo, time_t waitTime)
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

/* 1-based position of a vehicle in the queue; 0 if not waiting */
static int queuePosition(const char *regNo)
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

/* Unlink a node from the linked list (caller frees the node) */
static void unlinkNode(struct QueueNode *prev, struct QueueNode *node)
{
    if (prev == NULL) queueFront = node->next;
    else              prev->next = node->next;
    if (queueRear == node) queueRear = prev;
}

/* Remove a specific vehicle from the queue */
static void removeFromQueue(const char *regNo)
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

/* Free the whole queue (used on reset) */
static void freeQueue(void)
{
    struct QueueNode *node = queueFront;
    while (node != NULL) {
        struct QueueNode *next = node->next;
        free(node);
        node = next;
    }
    queueFront = queueRear = NULL;
}

static void saveQueue(void)
{
    struct QueueNode *node = queueFront;
    struct QueueSave rec;
    FILE *fp = fopen(DATA_QUEUE, "wb");
    if (fp == NULL) { printf("  [!] Cannot write %s\n", DATA_QUEUE); return; }
    while (node != NULL) {
        memset(&rec, 0, sizeof rec);
        strcpy(rec.regNo, node->regNo);
        rec.waitTime = node->waitTime;
        fwrite(&rec, sizeof rec, 1, fp);
        node = node->next;
    }
    fclose(fp);
}

static void loadQueue(void)
{
    struct QueueSave rec;
    FILE *fp = fopen(DATA_QUEUE, "rb");
    if (fp == NULL) return;
    while (fread(&rec, sizeof rec, 1, fp) == 1)
        enqueueVehicle(rec.regNo, rec.waitTime);
    fclose(fp);
}

/* Display the current waiting queue from front to rear */
static void displayQueue(void)
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

/* A slot has just been freed: scan the queue from the FRONT and give
   the slot to the first waiting vehicle of the matching type.        */
static int assignWaitingVehicle(int slotIndex)
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

/*=====================================================================
 |  MODULE 5 : REGISTRATION AND SEARCHING
 *=====================================================================*/

/* Linear search: index of a registered vehicle, or -1 */
static int findVehicleIndex(const char *regNo)
{
    int i;
    for (i = 0; i < vehicleCount; i++)
        if (strcmp(vehicles[i].regNo, regNo) == 0) return i;
    return -1;
}

/* Index of the occupied slot holding regNo, or -1 */
static int findSlotByReg(const char *regNo)
{
    int i;
    for (i = 0; i < TOTAL_SLOTS; i++)
        if (slots[i].occupied && strcmp(slots[i].regNo, regNo) == 0) return i;
    return -1;
}

/* Index of the first free slot for a vehicle type, or -1 */
static int findFreeSlotIndex(int type)
{
    int i;
    for (i = 0; i < TOTAL_SLOTS; i++)
        if (slots[i].type == type && !slots[i].occupied) return i;
    return -1;
}

/* Text describing where a vehicle currently is */
static void vehicleStatusText(const char *regNo, char *out, int size)
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

/* Option 1 : register a new vehicle */
static void registerVehicle(void)
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

    strcpy(vehicles[vehicleCount].regNo, regNo);
    strcpy(vehicles[vehicleCount].ownerName, owner);
    vehicles[vehicleCount].type = type;
    vehicleCount++;
    saveVehicles();

    printf("\n  [OK] Vehicle registered successfully!\n");
    printf("       Reg. No   : %s\n", regNo);
    printf("       Owner     : %s\n", owner);
    printf("       Type      : %s\n", typeName(type));
}

/* Option 7 : search a vehicle by registration number */
static void searchVehicle(void)
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

/*=====================================================================
 |  MODULE 6 : VEHICLE ENTRY, EXIT AND FEE CALCULATION (time.h)
 *=====================================================================*/

/* Whole minutes between entry and exit, rounded UP */
static long durationMinutes(time_t entryTime, time_t exitTime)
{
    long seconds = (long)difftime(exitTime, entryTime);
    long minutes = seconds / 60;
    if (seconds % 60 != 0) minutes++;
    if (minutes < 0) minutes = 0;
    return minutes;
}

/* Fee = first-hour rate + hourly rate for each extra hour.
   Stays up to GRACE_MINUTES are free.                               */
static double calculateFee(int type, time_t entryTime, time_t exitTime)
{
    long minutes, hours;
    double firstRate, hourlyRate;

    switch (type) {
        case TYPE_BIKE:  firstRate = BIKE_FIRST_RATE;  hourlyRate = BIKE_HOURLY_RATE;  break;
        case TYPE_CAR:   firstRate = CAR_FIRST_RATE;   hourlyRate = CAR_HOURLY_RATE;   break;
        case TYPE_HEAVY: firstRate = HEAVY_FIRST_RATE; hourlyRate = HEAVY_HOURLY_RATE; break;
        default:         firstRate = CAR_FIRST_RATE;   hourlyRate = CAR_HOURLY_RATE;   break;
    }
    minutes = durationMinutes(entryTime, exitTime);
    if (minutes <= GRACE_MINUTES) return 0.0;
    hours = (minutes + 59) / 60;              /* ceiling to full hours */
    if (hours < 1) hours = 1;
    if (hours == 1) return firstRate;
    return firstRate + (double)(hours - 1) * hourlyRate;
}

/* Option 2 : vehicle entry + automatic slot allocation */
static void vehicleEntry(void)
{
    char regNo[16], label[6], nowBuf[20];
    int idx, s;
    time_t now;

    printTitle("VEHICLE ENTRY AND SLOT ALLOCATION");
    readString("Enter vehicle registration number : ", regNo, (int)sizeof regNo);
    toUpperStr(regNo);

    idx = findVehicleIndex(regNo);
    if (idx < 0) {
        printf("  [!] Vehicle %s is NOT registered. Please register it first.\n",
               regNo);
        return;
    }
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

    now = time(NULL);                          /* current date + time   */
    s = findFreeSlotIndex(vehicles[idx].type);
    if (s >= 0) {
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

/* Option 3 : vehicle exit + fee calculation */
static void vehicleExit(void)
{
    char regNo[16], label[6], entryBuf[20], exitBuf[20], answer[8];
    int s, idx;
    time_t entry, now;
    double fee;
    long minutes, hours, minsPart;
    int pos;

    printTitle("VEHICLE EXIT AND FEE CALCULATION");
    readString("Enter vehicle registration number : ", regNo, (int)sizeof regNo);
    toUpperStr(regNo);

    s = findSlotByReg(regNo);
    if (s < 0) {
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

    entry = slots[s].entryTime;
    now   = time(NULL);
    fee   = calculateFee(slots[s].type, entry, now);
    minutes    = durationMinutes(entry, now);
    hours      = minutes / 60;
    minsPart   = minutes % 60;
    idx   = findVehicleIndex(regNo);
    slotLabel(slots[s].slotId, label);
    formatTime(entry, entryBuf);
    formatTime(now, exitBuf);

    printf("\n  ---------------- PARKING RECEIPT ----------------\n");
    printf("   Reg. No     : %s\n", regNo);
    printf("   Owner       : %s\n", (idx >= 0) ? vehicles[idx].ownerName : "(unknown)");
    printf("   Vehicle     : %s\n", typeName(slots[s].type));
    printf("   Slot        : %s\n", label);
    printf("   Entry time  : %s\n", entryBuf);
    printf("   Exit time   : %s\n", exitBuf);
    printf("   Duration    : %ldh %02ldm (%ld minutes)\n", hours, minsPart, minutes);
    if (fee <= 0.0)
        printf("   Parking fee : Rs. 0.00  (within %d-minute grace period)\n",
               GRACE_MINUTES);
    else
        printf("   Parking fee : Rs. %.2f\n", fee);
    printf("  --------------------------------------------------\n");

    readString("Confirm payment collected and allow exit? (yes/no) : ",
               answer, (int)sizeof answer);
    if (!isYes(answer)) {
        printf("  [i] Exit cancelled. The slot remains occupied.\n");
        return;
    }

    appendTransaction(regNo, slots[s].slotId, entry, now, fee);
    slots[s].occupied  = 0;
    slots[s].regNo[0]  = '\0';
    slots[s].entryTime = 0;
    saveSlots();
    printf("  [OK] Payment recorded. Slot %s is now FREE.\n", label);

    /* the freed slot may be needed by a waiting vehicle */
    assignWaitingVehicle(s);
}

/*=====================================================================
 |  MODULE 7 : SLOT AVAILABILITY AND PARKED VEHICLE LISTS
 *=====================================================================*/

/* Option 4 : show every slot and its live status */
static void showAvailability(void)
{
    int i;
    char label[6];
    int freeBike  = freeSlotsOfType(TYPE_BIKE);
    int freeCar   = freeSlotsOfType(TYPE_CAR);
    int freeHeavy = freeSlotsOfType(TYPE_HEAVY);

    printTitle("PARKING SLOT AVAILABILITY");
    printf("  %-6s %-18s %s\n", "Slot", "Type", "Status");
    printf("  -------------------------------------------------\n");
    for (i = 0; i < TOTAL_SLOTS; i++) {
        slotLabel(slots[i].slotId, label);
        printf("  %-6s %-18s %s\n", label, typeName(slots[i].type),
               slots[i].occupied ? slots[i].regNo : "FREE");
    }
    printf("  -------------------------------------------------\n");
    printf("  Bike  slots : %2d free / %2d total\n", freeBike,  TOTAL_BIKE_SLOTS);
    printf("  Car   slots : %2d free / %2d total\n", freeCar,   TOTAL_CAR_SLOTS);
    printf("  Heavy slots : %2d free / %2d total\n", freeHeavy, TOTAL_HEAVY_SLOTS);
    printf("  ---------------------------------\n");
    printf("  TOTAL       : %2d free / %2d total\n",
           freeBike + freeCar + freeHeavy, TOTAL_SLOTS);
}

/* Option 5 : parked vehicles, sorted by entry time (oldest first) */
static void showParkedVehicles(void)
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

/* Option 8 : current fee chart */
static void showFeeRates(void)
{
    printTitle("PARKING FEE RATES");
    printf("  -------------------------------------------------\n");
    printf("   Vehicle type    First hour   Every extra hour\n");
    printf("  -------------------------------------------------\n");
    printf("   Bike            Rs. %5.2f    Rs. %5.2f\n", BIKE_FIRST_RATE, BIKE_HOURLY_RATE);
    printf("   Car             Rs. %5.2f    Rs. %5.2f\n", CAR_FIRST_RATE,  CAR_HOURLY_RATE);
    printf("   Heavy Vehicle   Rs. %5.2f    Rs. %5.2f\n", HEAVY_FIRST_RATE, HEAVY_HOURLY_RATE);
    printf("  -------------------------------------------------\n");
    printf("  * Any part of an hour is billed as a full hour.\n");
    printf("  * Stays up to %d minutes are FREE (grace period).\n", GRACE_MINUTES);
}

/*=====================================================================
 |  MODULE 8 : ADMIN FEATURES (login, reports, reset)
 *=====================================================================*/

/* Returns 1 if the password was accepted (3 attempts allowed) */
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

/* Admin : list all registered vehicles */
static void adminViewVehicles(void)
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

/* Admin : full transaction history, sorted by fee (highest first) */
static void adminViewTransactions(void)
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

/* Admin : revenue summary by vehicle type */
static void adminRevenue(void)
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

/* Admin : delete every data file and start fresh */
static void adminResetData(void)
{
    char answer[32];
    printTitle("RESET ALL DATA (ADMIN)");
    printf("  [!] This permanently deletes ALL registered vehicles,\n");
    printf("      all transactions and the waiting queue.\n");
    readString("  Type RESET to confirm (anything else cancels) : ",
               answer, (int)sizeof answer);
    if (strcmp(answer, "RESET") != 0) {
        printf("  [i] Reset cancelled. All data is safe.\n");
        return;
    }
    remove(DATA_VEHICLES);
    remove(DATA_SLOTS);
    remove(DATA_TXN);
    remove(DATA_QUEUE);
    vehicleCount = 0;
    freeQueue();
    initializeSlots();
    saveVehicles();
    saveSlots();
    saveQueue();
    printf("  [OK] All data has been cleared. The lot is empty and fresh.\n");
}

/* Admin menu */
static void adminMenu(void)
{
    int choice;
    if (!adminLogin()) return;
    do {
        printTitle("ADMIN MENU");
        printf("   1. View all registered vehicles\n");
        printf("   2. Transaction history (sorted by fee)\n");
        printf("   3. Revenue summary\n");
        printf("   4. Reset all data\n");
        printf("   0. Back to main menu\n");
        printf("  -------------------------------------------------\n");
        choice = readInt("  Enter your choice : ", 0, 4);
        switch (choice) {
            case 1: adminViewVehicles(); pauseScreen(); break;
            case 2: adminViewTransactions(); pauseScreen(); break;
            case 3: adminRevenue(); pauseScreen(); break;
            case 4: adminResetData(); pauseScreen(); break;
            case 0: break;
            default: break;
        }
    } while (choice != 0);
}

/*=====================================================================
 |  MODULE 9 : STARTUP AND MAIN MENU
 *=====================================================================*/

/* Create the whole lot: bike slots first, then car, then heavy */
static void initializeSlots(void)
{
    int id = 1, i;
    for (i = 0; i < TOTAL_BIKE_SLOTS;  i++, id++) {
        slots[id - 1].slotId = id;  slots[id - 1].type = TYPE_BIKE;
        slots[id - 1].occupied = 0; slots[id - 1].regNo[0] = '\0';
        slots[id - 1].entryTime = 0;
    }
    for (i = 0; i < TOTAL_CAR_SLOTS;   i++, id++) {
        slots[id - 1].slotId = id;  slots[id - 1].type = TYPE_CAR;
        slots[id - 1].occupied = 0; slots[id - 1].regNo[0] = '\0';
        slots[id - 1].entryTime = 0;
    }
    for (i = 0; i < TOTAL_HEAVY_SLOTS; i++, id++) {
        slots[id - 1].slotId = id;  slots[id - 1].type = TYPE_HEAVY;
        slots[id - 1].occupied = 0; slots[id - 1].regNo[0] = '\0';
        slots[id - 1].entryTime = 0;
    }
}

/* Make sure a data file exists (creates an empty one if missing) so
   that all four files are visible right after the very first run.   */
static void ensureFile(const char *name)
{
    FILE *fp = fopen(name, "rb");
    if (fp != NULL) { fclose(fp); return; }      /* already there       */
    fp = fopen(name, "wb");                      /* create empty file   */
    if (fp != NULL) fclose(fp);
}

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

static void printMainMenu(void)
{
    printf("\n");
    printf("=====================================================\n");
    printf("                P A R K E A S E\n");
    printf("      Smart Parking Lot Management System\n");
    printf("=====================================================\n");
    printf("  Lot status : %d free / %d total   "
           "[Bike %d/%d | Car %d/%d | Heavy %d/%d]\n",
           freeSlotsOfType(TYPE_BIKE) + freeSlotsOfType(TYPE_CAR) +
               freeSlotsOfType(TYPE_HEAVY),
           TOTAL_SLOTS,
           freeSlotsOfType(TYPE_BIKE),  TOTAL_BIKE_SLOTS,
           freeSlotsOfType(TYPE_CAR),   TOTAL_CAR_SLOTS,
           freeSlotsOfType(TYPE_HEAVY), TOTAL_HEAVY_SLOTS);
    printf("-----------------------------------------------------\n");
    printf("   1. Register a new vehicle\n");
    printf("   2. Vehicle entry  (allocate a slot)\n");
    printf("   3. Vehicle exit   (calculate parking fee)\n");
    printf("   4. View parking slot availability\n");
    printf("   5. View currently parked vehicles\n");
    printf("   6. View waiting queue\n");
    printf("   7. Search vehicle status\n");
    printf("   8. View parking fee rates\n");
    printf("   9. Admin login\n");
    printf("   0. Save and exit\n");
    printf("-----------------------------------------------------\n");
}

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
    printf("  Default admin password : %s (change it in the source)\n",
           ADMIN_PASSWORD);

    do {
        printMainMenu();
        choice = readInt("  Enter your choice : ", 0, 9);
        switch (choice) {
            case 1: registerVehicle();      pauseScreen(); break;
            case 2: vehicleEntry();         pauseScreen(); break;
            case 3: vehicleExit();          pauseScreen(); break;
            case 4: showAvailability();     pauseScreen(); break;
            case 5: showParkedVehicles();   pauseScreen(); break;
            case 6: displayQueue();         pauseScreen(); break;
            case 7: searchVehicle();        pauseScreen(); break;
            case 8: showFeeRates();         pauseScreen(); break;
            case 9: adminMenu();            break;
            case 0: break;
            default: break;
        }
    } while (choice != 0);

    printf("\n  All data has been saved. Thank you for using ParkEase!\n");
    printf("=====================================================\n");
    return 0;
}
