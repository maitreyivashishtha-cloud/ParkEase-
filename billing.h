/*=====================================================================
 |  ParkEase - MODULE: BILLING                              (billing.h)
 |
 |  Everything about money:
 |    - duration of a stay (minutes, rounded up)
 |    - fee calculation (first-hour rate + hourly rate, grace period)
 |    - the printed parking receipt
 |    - the fee rate chart shown to the operator
 *=====================================================================*/

#ifndef BILLING_H
#define BILLING_H

#include <time.h>

/* Whole minutes between entry and exit, rounded UP */
long   durationMinutes(time_t entryTime, time_t exitTime);

/* Fee = first-hour rate + hourly rate for each extra hour.
   Stays up to GRACE_MINUTES are free.                               */
double calculateFee(int type, time_t entryTime, time_t exitTime);

/* Print the parking receipt for one exit */
void printReceipt(const char *regNo, const char *owner, int type,
                  int slotId, time_t entryTime, time_t exitTime,
                  double fee);

/* Menu option: current fee chart */
void showFeeRates(void);

#endif /* BILLING_H */
