# ParkEase - Smart Parking Lot Management System

Menu-driven parking lot manager in C, built as a **multi-file project**.
The file layout matches the architecture / block diagram of the report:

```
User
 └── Main Menu Module          (main.c)
      ├── Vehicle Management   (vehicle.c   + vehicle.h)
      ├── Parking Slot Mgmt    (slots.c     + slots.h)
      ├── Entry/Exit Mgmt      (entryexit.c + entryexit.h)
      ├── Billing Module       (billing.c   + billing.h)
      ├── Waiting Queue Module (queue.c     + queue.h)
      └── Reports Module       (reports.c   + reports.h)
           └── File Handling Module         (fileio.c + fileio.h)
                ├── vehicles.dat       (vehicle records)
                ├── slots.dat          (parking slot records)
                ├── transactions.dat   (transaction records)
                └── queue.dat          (waiting queue records)
```

Shared things (constants, structs, global data, utility functions)
live in `parkease.h`, `globals.c` and `util.c`.

## How to compile and run

| Environment | Command |
| --- | --- |
| Linux / macOS / MinGW | `make` then `./parkease` |
| Without make | `gcc -Wall -Wextra -O2 -o parkease *.c` then `./parkease` |
| Dev-C++ / Code::Blocks | Add **all** `.c` and `.h` files to the project, build |

The four binary data files are created automatically in the folder
where you run the program.

## Configuration (parkease.h)

| Setting | Value |
| --- | --- |
| Slots | 20 bike (B01-B20), 15 car (C01-C15), 5 heavy (H01-H05) |
| Bike fee | Rs. 20 first hour + Rs. 10 per extra hour |
| Car fee | Rs. 40 first hour + Rs. 20 per extra hour |
| Heavy fee | Rs. 60 first hour + Rs. 30 per extra hour |
| Grace period | 15 minutes free |
| Admin password | `admin123` |
| Max vehicles | 500 records |

## Roles

- **Operator** (no password): register vehicles, vehicle entry,
  vehicle exit with receipt, parking lot status grid, search,
  waiting queue, fee rates.
- **Administrator** (password): manage parking (grid, slot details,
  parked list, manual slot release, reset), transaction history,
  revenue report, vehicle statistics.

## Module responsibilities

| File | Responsibilities |
| --- | --- |
| `parkease.h` | CONFIG section, all struct definitions, `extern` global data, utility prototypes |
| `globals.c` | the one copy of the shared global variables |
| `util.c` | safe keyboard input, string helpers, screen/format helpers |
| `main.c` | startup, main menu, operator/admin/manage menus, admin login |
| `vehicle.c` | registration, add/search helpers, live vehicle status |
| `slots.c` | slot creation, free-slot search, `[X]`/`[ ]` grid, slot details, manual release |
| `entryexit.c` | the entry workflow and the exit workflow |
| `billing.c` | duration, fee calculation, receipt, fee chart |
| `queue.c` | dynamic linked-list queue: enqueue, position, auto-allocate |
| `reports.c` | parked list, registered list, transactions, revenue, statistics |
| `fileio.c` | read/write of the four data files, admin reset |
