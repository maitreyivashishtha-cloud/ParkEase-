/*=====================================================================
 |  ParkEase - MODULE: FILE HANDLING                         (fileio.c)
 |
 |  The ONLY module that opens the four data files. All other
 |  modules call these functions to store or retrieve their records:
 |
 |      vehicles.dat      - fixed-size struct Vehicle records
 |      slots.dat         - fixed-size struct Slot records (all 40)
 |      transactions.dat  - one struct Transaction appended per exit
 |      queue.dat         - one struct QueueSave record per waiting car
 |
 |  All files are binary (fwrite / fread), created automatically on
 |  the first run, and reloaded automatically on every start.
 *=====================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parkease.h"
#include "fileio.h"
#include "queue.h"
#include "slots.h"

/*---------------------------------------------------------------------
 |  Make sure a data file exists (creates an empty one if missing)
 |  so that all four files are visible right after the very first run.
 *-------------------------------------------------------------------*/
void ensureFile(const char *name)
{
    FILE *fp = fopen(name, "rb");
    if (fp != NULL) { fclose(fp); return; }      /* already there       */
    fp = fopen(name, "wb");                      /* create empty file   */
    if (fp != NULL) fclose(fp);
}

/*===================== Vehicle records =============================*/

void saveVehicles(void)
{
    FILE *fp = fopen(DATA_VEHICLES, "wb");
    if (fp == NULL) { printf("  [!] Cannot write %s\n", DATA_VEHICLES); return; }
    fwrite(vehicles, sizeof(struct Vehicle), (size_t)vehicleCount, fp);
    fclose(fp);
}

void loadVehicles(void)
{
    FILE *fp = fopen(DATA_VEHICLES, "rb");
    if (fp == NULL) return;                       /* first run: no file  */
    while (vehicleCount < MAX_VEHICLES &&
           fread(&vehicles[vehicleCount], sizeof(struct Vehicle), 1, fp) == 1)
        vehicleCount++;
    fclose(fp);
}

/*===================== Parking slot records ========================*/

void saveSlots(void)
{
    FILE *fp = fopen(DATA_SLOTS, "wb");
    if (fp == NULL) { printf("  [!] Cannot write %s\n", DATA_SLOTS); return; }
    fwrite(slots, sizeof(struct Slot), TOTAL_SLOTS, fp);
    fclose(fp);
}

/* Returns 1 if a slot file existed and was loaded, 0 otherwise */
int loadSlots(void)
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

/*===================== Transaction records =========================*/

void appendTransaction(const char *regNo, int slotId,
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
struct Transaction *loadTransactions(int *countOut)
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

/*===================== Waiting queue records =======================*/

void saveQueue(void)
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

void loadQueue(void)
{
    struct QueueSave rec;
    FILE *fp = fopen(DATA_QUEUE, "rb");
    if (fp == NULL) return;
    while (fread(&rec, sizeof rec, 1, fp) == 1)
        enqueueVehicle(rec.regNo, rec.waitTime);
    fclose(fp);
}

/*===================== Admin: full reset ===========================*/

/* Admin option: delete every data file and start fresh */
void adminResetData(void)
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
