/*=====================================================================
 |  ParkEase - GENERAL UTILITY FUNCTIONS                     (util.c)
 |
 |  Small helpers shared by every module: safe keyboard input,
 |  string utilities and screen/formatting helpers.
 *=====================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parkease.h"

/*---------------------------------------------------------------------
 |  Read one line of text from the keyboard (safe against overflow).
 |  If the input stream closes (Ctrl+D / Ctrl+Z), exit the program.
 *-------------------------------------------------------------------*/
void readLine(char *buf, int size)
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
void trimStr(char *s)
{
    char *start = s;
    size_t len;
    while (*start != '\0' && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

/* Ask until the user types a non-empty line without spaces */
void readString(const char *prompt, char *out, int size)
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
void readName(const char *prompt, char *out, int size)
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
int readInt(const char *prompt, int min, int max)
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
void toUpperStr(char *s)
{
    for (; *s != '\0'; s++) *s = (char)toupper((unsigned char)*s);
}

/* Accept "y", "Y", "yes", "YES" ... as true */
int isYes(const char *s)
{
    char low[8];
    size_t i;
    for (i = 0; i < sizeof low - 1 && s[i] != '\0'; i++)
        low[i] = (char)tolower((unsigned char)s[i]);
    low[i] = '\0';
    return (strcmp(low, "y") == 0 || strcmp(low, "yes") == 0);
}

/* Pause until the user presses ENTER */
void pauseScreen(void)
{
    char dummy[8];
    printf("\nPress ENTER to return to the menu...");
    readLine(dummy, (int)sizeof dummy);
}

/* Print a section heading */
void printTitle(const char *title)
{
    printf("\n=====================================================\n");
    printf("   %s\n", title);
    printf("=====================================================\n");
}

/* Human-readable name of a vehicle type */
const char *typeName(int type)
{
    switch (type) {
        case TYPE_BIKE:  return "Bike (Two-Wheeler)";
        case TYPE_CAR:   return "Car";
        case TYPE_HEAVY: return "Heavy Vehicle";
        default:         return "Unknown";
    }
}

/* Short slot label, e.g. slot 1 -> "B01", slot 21 -> "C01",
   slot 36 -> "H01" (B = bike, C = car, H = heavy vehicle)          */
void slotLabel(int slotId, char *out /* size >= 6 */)
{
    if (slotId <= TOTAL_BIKE_SLOTS)
        sprintf(out, "B%02d", slotId);
    else if (slotId <= TOTAL_BIKE_SLOTS + TOTAL_CAR_SLOTS)
        sprintf(out, "C%02d", slotId - TOTAL_BIKE_SLOTS);
    else
        sprintf(out, "H%02d",
                slotId - TOTAL_BIKE_SLOTS - TOTAL_CAR_SLOTS);
}

/* Format a time_t value as "dd-mm-yyyy hh:mm" */
void formatTime(time_t t, char *out /* size >= 20 */)
{
    struct tm *tmPtr = localtime(&t);
    if (tmPtr == NULL) { strcpy(out, "?"); return; }
    strftime(out, 20, "%d-%m-%Y %H:%M", tmPtr);
}
