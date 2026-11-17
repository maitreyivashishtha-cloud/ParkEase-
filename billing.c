/*=====================================================================
 |  ParkEase - MODULE: BILLING                               (billing.c)
 |
 |  The money side of the project, built on <time.h>:
 |    - durationMinutes(): how long a vehicle stayed
 |    - calculateFee():    first-hour rate + hourly rate per extra
 |                         hour, with a grace period for short stays
 |    - printReceipt():    the printed parking receipt
 |    - showFeeRates():    the dynamic fee chart shown to the operator
 |
 |  FEE STRUCTURE (from the project slides)
 |      Bike  : Rs. 20 first hour, then Rs. 10 per extra hour
 |      Car   : Rs. 40 first hour, then Rs. 20 per extra hour
 |      Heavy : Rs. 60 first hour, then Rs. 30 per extra hour
 *-------------------------------------------------------------------*/

#include <stdio.h>
#include "parkease.h"
#include "billing.h"

/*---------------------------------------------------------------------
 |  Whole minutes between entry and exit, rounded UP
 *-------------------------------------------------------------------*/
long durationMinutes(time_t entryTime, time_t exitTime)
{
    long seconds = (long)difftime(exitTime, entryTime);
    long minutes = seconds / 60;
    if (seconds % 60 != 0) minutes++;
    if (minutes < 0) minutes = 0;
    return minutes;
}

/*---------------------------------------------------------------------
 |  Fee = first-hour rate + hourly rate for each extra hour.
 |  Any part of an hour is billed as a full hour; stays up to
 |  GRACE_MINUTES are free.
 *-------------------------------------------------------------------*/
double calculateFee(int type, time_t entryTime, time_t exitTime)
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

/*---------------------------------------------------------------------
 |  Print the parking receipt for one exit
 *-------------------------------------------------------------------*/
void printReceipt(const char *regNo, const char *owner, int type,
                  int slotId, time_t entryTime, time_t exitTime, double fee)
{
    char label[6], entryBuf[20], exitBuf[20];
    long minutes    = durationMinutes(entryTime, exitTime);
    long hours      = minutes / 60;
    long minsPart   = minutes % 60;

    slotLabel(slotId, label);
    formatTime(entryTime, entryBuf);
    formatTime(exitTime, exitBuf);

    printf("\n  ---------------- PARKING RECEIPT ----------------\n");
    printf("   Reg. No     : %s\n", regNo);
    printf("   Owner       : %s\n", owner);
    printf("   Vehicle     : %s\n", typeName(type));
    printf("   Slot        : %s\n", label);
    printf("   Entry time  : %s\n", entryBuf);
    printf("   Exit time   : %s\n", exitBuf);
    printf("   Duration    : %ldh %02ldm (%ld minutes)\n",
           hours, minsPart, minutes);
    if (fee <= 0.0)
        printf("   Parking fee : Rs. 0.00  (within %d-minute grace period)\n",
               GRACE_MINUTES);
    else
        printf("   Parking fee : Rs. %.2f\n", fee);
    printf("  --------------------------------------------------\n");
}

/*---------------------------------------------------------------------
 |  Operator option 7 : current fee chart
 *-------------------------------------------------------------------*/
void showFeeRates(void)
{
    printTitle("PARKING FEE RATES");
    printf("  -------------------------------------------------\n");
    printf("   Vehicle type    First hour   Every extra hour\n");
    printf("  -------------------------------------------------\n");
    printf("   Bike            Rs. %5.2f    Rs. %5.2f\n",
           BIKE_FIRST_RATE, BIKE_HOURLY_RATE);
    printf("   Car             Rs. %5.2f    Rs. %5.2f\n",
           CAR_FIRST_RATE, CAR_HOURLY_RATE);
    printf("   Heavy Vehicle   Rs. %5.2f    Rs. %5.2f\n",
           HEAVY_FIRST_RATE, HEAVY_HOURLY_RATE);
    printf("  -------------------------------------------------\n");
    printf("  * Any part of an hour is billed as a full hour.\n");
    printf("  * Stays up to %d minutes are FREE (grace period).\n",
           GRACE_MINUTES);
}
