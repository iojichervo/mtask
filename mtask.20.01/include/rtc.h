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

typedef struct {
	int id;
	char * name;
    char * * argv;
    int argc;
	time_t date;
    int repeat;
    int secs;
    int console;
} task;

typedef struct {
	char * name;
    char * * argv;
    int argc;
    int msecs;
    int console;
} timer_t;

int get_update_in_progress_flag() ;

unsigned char get_RTC_register(int reg);

void set_RTC_register(int reg ,unsigned char change_var );

void read_rtc();

int toBCD(int n);

int fromBCD(int n);

void set_time();

int day_of_the_week();

int run_calendar(void *arg);

int delete_calendar(void *arg);

int first_day_month();

int days_in_month();

int is_leap_year(int year);

int taskcp(task *a1, task *a2);

void rtc_handler(unsigned irq_number);

void addSeconds(time_t * t, int secs);

void habilitate_cmos_int();

void set_alarm(time_t * tp);

void timer_handler(unsigned irq_number);

#endif