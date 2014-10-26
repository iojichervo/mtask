#ifndef RTC_H_INCLUDED
#define RTC_H_INCLUDED

typedef struct {
	unsigned char second;
	unsigned char minute;
	unsigned char hour;
	unsigned char day;
	unsigned char month;
	unsigned int year;
}time_t;

 int get_update_in_progress_flag() ;

unsigned char get_RTC_register(int reg);

void set_RTC_register(int reg ,unsigned char change_var );

void read_rtc();

int toBCD(int n);

void set_time();

#endif