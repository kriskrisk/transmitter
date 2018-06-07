#ifndef ASSIGNMENT2_SIK_ERR_H
#define ASSIGNMENT2_SIK_ERR_H

/* wypisuje informacje o blednym zakonczeniu funkcji systemowej
i konczy dzialanie */
extern void syserr(const char *fmt, ...);

/* wypisuje informacje o bledzie i konczy dzialanie */
extern void fatal(const char *fmt, ...);

#endif //ASSIGNMENT2_SIK_ERR_H

