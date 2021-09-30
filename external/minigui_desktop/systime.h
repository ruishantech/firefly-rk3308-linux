#ifndef __SYSTIME_H__
#define __SYSTIME_H__

void time_flush(void);
void time_set(struct tm time);
void timing_set(struct tm time, int n);
void timing_power_off_set(void);
void timing_power_on_set(void);

#endif
