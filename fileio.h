/*=====================================================================
 |  ParkEase - MODULE: FILE HANDLING                        (fileio.h)
 |
 |  The ONLY module that talks to the four data files. Every other
 |  module stores or retrieves its records through these functions:
 |
 |      vehicles.dat      <- Vehicle Records
 |      slots.dat         <- Parking Slot Records
 |      transactions.dat  <- Transaction Records
 |      queue.dat         <- Waiting Queue Records
 *=====================================================================*/

#ifndef FILEIO_H
#define FILEIO_H

#include <time.h>

/* Make sure a data file exists (creates an empty one if missing) */
void ensureFile(const char *name);

/* Vehicle records */
void saveVehicles(void);
void loadVehicles(void);

/* Parking slot records. loadSlots() returns 1 if a valid slot file
   existed, 0 otherwise (first run or corrupt file: re-initialize).  */
void saveSlots(void);
int  loadSlots(void);

/* Transaction records */
void appendTransaction(const char *regNo, int slotId, time_t entryTime,
                       time_t exitTime, double amount);
/* Load all transactions into a malloc-ed array (caller must free).
   Sets *countOut; returns NULL if there is no file or it is empty.  */
struct Transaction *loadTransactions(int *countOut);

/* Waiting queue records */
void saveQueue(void);
void loadQueue(void);

/* Admin option: delete every data file and start fresh */
void adminResetData(void);

#endif /* FILEIO_H */
