/*=====================================================================
 |  ParkEase - MODULE: ENTRY / EXIT MANAGEMENT          (entryexit.h)
 |
 |  The two core workflows of the parking lot:
 |    - vehicleEntry(): validate/ register the vehicle, find a free
 |      slot of the matching type and record the entry time, or add
 |      the vehicle to the waiting queue when the lot is full
 |    - vehicleExit(): print the receipt via the Billing Module,
 |      record the transaction, free the slot and let the Waiting
 |      Queue Module auto-allocate it to the first waiting vehicle
 *=====================================================================*/

#ifndef ENTRYEXIT_H
#define ENTRYEXIT_H

/* Menu option: vehicle entry + automatic slot allocation */
void vehicleEntry(void);

/* Menu option: vehicle exit + fee calculation */
void vehicleExit(void);

#endif /* ENTRYEXIT_H */
