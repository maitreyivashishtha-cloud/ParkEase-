# ParkEase - Smart Parking Lot Management System
# Multi-file build. Just run:  make
# (or compile manually:  gcc -Wall -Wextra -O2 -o parkease *.c )

CC     = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = parkease

SRCS = main.c globals.c util.c vehicle.c slots.c entryexit.c \
       billing.c queue.c reports.c fileio.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Every .c file depends on parkease.h (the shared header)
%.o: %.c parkease.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
