#ifndef RTC_H_INCLUDED
#define RTC_H_INCLUDED

typedef struct {
	unsigned char second;
	unsigned char minute;
	unsigned char hour;
	unsigned char day;
	unsigned char month;
	unsigned int year;
	unsigned int century;
	unsigned char week_day;
} time_t;

int get_update_in_progress_flag() ;

unsigned char get_RTC_register(int reg);

void set_RTC_register(int reg ,unsigned char change_var );

void read_rtc();

int toBCD(int n);

int fromBCD(int n);

void set_time();

int day_of_the_week();

int run_calendar(void *arg);

int first_day_month();

int days_in_month();

int is_leap_year(int year);

#endif