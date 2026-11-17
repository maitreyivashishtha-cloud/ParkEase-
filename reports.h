/*=====================================================================
 |  ParkEase - MODULE: REPORTS                              (reports.h)
 |
 |  All read-only views over the data, used from the menus:
 |    - list of currently parked vehicles (sorted by entry time)
 |    - list of all registered vehicles
 |    - transaction history (sorted by fee)
 |    - revenue summary by vehicle type
 |    - vehicle statistics for the administrator
 *=====================================================================*/

#ifndef REPORTS_H
#define REPORTS_H

/* Menu option: parked vehicles, sorted by entry time (oldest first) */
void showParkedVehicles(void);

/* Admin option: list all registered vehicles with live status */
void adminViewVehicles(void);

/* Admin option: full transaction history, sorted by fee */
void adminViewTransactions(void);

/* Admin option: revenue summary by vehicle type */
void adminRevenue(void);

/* Admin option: vehicle statistics (registered / parked / queue) */
void adminVehicleStatistics(void);

#endif /* REPORTS_H */
