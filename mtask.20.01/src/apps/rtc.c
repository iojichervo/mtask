#include "../include/rtc.h"
#include "../include/list_ADT.h"
#include <kernel.h>
#include <apps.h>

static listADT alarms = NULL;
static int alarmCount = 0;
time_t curr_time;
alarm * first_alarm;
int first = 1;

int cons;

enum {
	cmos_address = 0x70,
	cmos_data    = 0x71
};

const char *_days[] = {
	"lun", "mar", "mie", "jue", 
	"vie", "sab", "dom"
};

const char *_months[] = {
	"ene", "feb", "mar",
	"abr", "may", "jun",
	"jul", "ago", "sep",
	"oct", "nov", "dic"
};

int get_update_in_progress_flag() {
	outb(cmos_address, 0x0A);
	return (inb(cmos_data) & 0x80);
}

unsigned char get_RTC_register(int reg) {
	outb(cmos_address, reg);
	return inb(cmos_data);
}

void set_RTC_register(int reg ,unsigned char change_var ) {
	outb(cmos_address, reg);
	outb(cmos_data, change_var);
}

void read_rtc() {
	unsigned char last_second;
	unsigned char last_minute;
	unsigned char last_hour;
	unsigned char last_day;
	unsigned char last_month;
	unsigned char last_year;
	unsigned char registerB;

      // Se leen los registros hasta que se obtengan los mismos valores dos veces para evitar datos
      // inconsistentes por la actualización del RTC

      while (get_update_in_progress_flag());                // Se asegura que no se este actualizando
      curr_time.second = get_RTC_register(0x00);
      curr_time.minute = get_RTC_register(0x02);
      curr_time.hour = get_RTC_register(0x04);
      curr_time.day = get_RTC_register(0x07);
      curr_time.month = get_RTC_register(0x08);
      curr_time.year = get_RTC_register(0x09);
      if(first) {											// La primera vez, el siglo es 21
      	curr_time.century = 21;
      	first = !first;
      }

      do {
      	last_second =curr_time.second;
      	last_minute = curr_time.minute;
      	last_hour = curr_time.hour;
      	last_day = curr_time.day;
      	last_month = curr_time.month;
      	last_year = curr_time.year;

      	while (get_update_in_progress_flag());
      	curr_time.second = get_RTC_register(0x00);
      	curr_time.minute = get_RTC_register(0x02);
      	curr_time.hour = get_RTC_register(0x04);
      	curr_time.day = get_RTC_register(0x07);
      	curr_time.month = get_RTC_register(0x08);
      	curr_time.year = get_RTC_register(0x09);
      } while( (last_second != curr_time.second) || (last_minute != curr_time.minute) || (last_hour != curr_time.hour) ||
      	(last_day != curr_time.day) || (last_month != curr_time.month) || (last_year != curr_time.year) );

      registerB = get_RTC_register(0x0B);

      // Se convierte a BCD en caso necesario

      if (!(registerB & 0x04)) {
      	curr_time.second = (curr_time.second & 0x0F) + ((curr_time.second / 16) * 10);
      	curr_time.minute = (curr_time.minute & 0x0F) + ((curr_time.minute / 16) * 10);
      	curr_time.hour = ( (curr_time.hour & 0x0F) + (((curr_time.hour & 0x70) / 16) * 10) ) | (curr_time.hour & 0x80);
      	curr_time.day = (curr_time.day & 0x0F) + ((curr_time.day / 16) * 10);
      	curr_time.month = (curr_time.month & 0x0F) + ((curr_time.month / 16) * 10);
      	curr_time.year = (curr_time.year & 0x0F) + ((curr_time.year / 16) * 10);
      }

      // Se convierte en formato de 12 horas a 24

      if (!(registerB & 0x02) && (curr_time.hour & 0x80)) {
      	curr_time.hour = ((curr_time.hour & 0x7F) + 12) % 24;
      }

      // Calculo final del año

      // curr_time.year += curr_time.century * 100;
      curr_time.year += (curr_time.year / 100) * 100;
      if(curr_time.year < curr_time.year)
      	curr_time.year += 100;

      curr_time.week_day = day_of_the_week();
  }

  int print_date(){
  	read_rtc();

    // Formato: jue dic 18 15:02:46 2014
  	printk("%s %s %d ", _days[curr_time.week_day], _months[curr_time.month-1], curr_time.day);
  	printk("%02d:%02d:%02d ", curr_time.hour, curr_time.minute, curr_time.second);
  	printk("%d%d\n", curr_time.century-1, curr_time.year);
  	return 0;
  }

  int set_date(int argc,char** argv){
  	char year[3];
  	char cent[3];

  	if(argc != 7){
  		cprintk(LIGHTRED,BLACK,"Los parametros deben ser 6.\n");
  	} else {
  		curr_time.hour = toBCD(atoi(argv[1]));
  		curr_time.minute = toBCD(atoi(argv[2]));
  		curr_time.second = toBCD(atoi(argv[3]));
  		curr_time.day = toBCD(atoi(argv[4]));
  		curr_time.month =toBCD(atoi(argv[5]));

	    // No esta implementado substr
  		cent[0] = argv[6][0];
  		cent[1] = argv[6][1];
  		cent[2] = 0;	

  		curr_time.century = atoi(cent) + 1;

  		year[0] = argv[6][2];
  		year[1] = argv[6][3];
  		year[2] = 0;
  		curr_time.year = toBCD(atoi(year));
  		set_time();
  	} 
  	printk("La nueva fecha es: ");						//RECORDAR ACTUALIZAR EL CALENDARIO CUANDO SE HACE set_date
  	print_date();
  	return 0;
  }

  int toBCD(int n) {
  	return ((n / 10) << 4)|(n% 10);
  }

  void set_time(){
  	set_RTC_register(0x00,curr_time.second);
  	set_RTC_register(0x02, curr_time.minute);
  	set_RTC_register(0x04,curr_time.hour );
  	set_RTC_register(0x07, curr_time.day);
  	set_RTC_register(0x08, curr_time.month);
  	set_RTC_register(0x09, curr_time.year);
  }


  int calendar(){
  	Task_t *t;

  	t = CreateTask(run_calendar, MAIN_STKSIZE, NULL, "calendar", DEFAULT_PRIO);
  	Protect(t);
  	SetConsole(t, 0);
  	Ready(t);

  	mt_cons_setcurrent(0);
  	mt_cons_setfocus(0);

  	return 0;
  }

  int run_calendar(void *arg) {
  	int i, j = 0, ctr = 1, x, m, mdays;
  	char * header = "     %s  -  %d%d    ";
  	char * days = " L  M  M  J  V  S  D ";

  	if(first)
  		read_rtc();

  	x = mt_cons_ncols() - strlen(days);
  	m = first_day_month();
  	mdays = days_in_month();

  	mt_cons_gotoxy(x, j++);
  	cprintk(BLACK, LIGHTRED,header, _months[curr_time.month-1], curr_time.century-1, curr_time.year);
  	mt_cons_gotoxy(x, j++);
  	cprintk(BLACK, LIGHTRED, days);

  	do {
  		mt_cons_gotoxy(x, j++);
  		for( i = 0 ; i < 7 ; i++){
  			if(ctr <= mdays && (i >= m ||  j > 3) )
  				if(ctr == curr_time.day) {
	  				cprintk(LIGHTRED, RED,"%2d ",ctr++);
  				} else {
	  				cprintk(BLACK, RED,"%2d ",ctr++);
  				}
  			else
  				cprintk(BLACK, RED,"   ");
  		}
  	} while(ctr <= mdays);

  	return 0;
  }

  int day_of_the_week(){
  	int h,q,k,j,m,y;

  	y = curr_time.year + (curr_time.century-1)*100;
  	m = curr_time.month;
  	if(m == 1) {
  		m = 13;
  		y--;
  	}

  	if (m == 2) {
  		m = 14;
  		y--;
  	}

  	q = curr_time.day;
  	k = y % 100;
  	j = y / 100;
  	h = q + 13*(m+1)/5 + k + k/4 + j/4 + 5*j;
  	h = h % 7;

  	if(h <= 1)
  		return h + 5;
  	else
  		return h - 2;

  	return -1;
  }

  int first_day_month() {
  	int aux = curr_time.day, ret;

  	curr_time.day = 1;
  	ret = day_of_the_week();
  	curr_time.day = aux;

  	return ret;
  }

  int days_in_month() {
  	int val, aux;

  	switch(curr_time.month) {
  		case 2:
	  		aux = curr_time.year + (curr_time.century-1)*100;
	  		if(is_leap_year(aux))
	  			val = 29;
	  		else
	  			val = 28;
	  		break;

  		case 4:
  		case 6:
  		case 9:
  		case 11:
  			val = 30;
  			break;

  		default:
  			val = 31;
  		break;
  	}

  	return val;
  }

  int is_leap_year(int year) {
  	return ( (year%4==0) && !(year%100==0) ) || (year%400==0);
  }

  int cron(int argc,char** argv){
    if(argc<6){
      printk("los parametros necesarios  deben ser mayores  a 5.\n");
    }
    time_t cron_time;

      cron_time.hour = toBCD(atoi(argv[1]));
      cron_time.minute =toBCD(atoi(argv[2]));
      cron_time.second =toBCD(atoi(argv[3]));
      cron_time.day = toBCD(atoi(argv[4]));
      if(alarms==NULL)
      alarms = NewList(sizeof(alarm), (int(*)(void *, void *)) compAlarm);
      alarm a_prepare;
      a_prepare.id = alarmCount++;
      a_prepare.name = argv[6]; //nombre de la alarma es el nombre del comando que disparara.
      a_prepare.repeat = 0;
      if(argc == 8) 
      { 
       a_prepare.cmd_path = Malloc(2 * sizeof(char *));
       a_prepare.cmd_path[0] = Malloc(strlen(argv[6] + 1));
      strcpy(a_prepare.cmd_path[0],argv[6]);
      a_prepare.cmd_path[1] = Malloc(strlen(argv[7] + 1));
      strcpy(a_prepare.cmd_path[1], argv[7]);
      a_prepare.cmd_variables = 2;
      }
     else if (argc == 7) 
     { 
         a_prepare.cmd_path= Malloc(1 * sizeof(char *));
         a_prepare.cmd_path[0] = Malloc(strlen(argv[6] + 1));
         strcpy( a_prepare.cmd_path[0], argv[6]);
         a_prepare.cmd_variables= 1;
      }
      else {
               cprintk(LIGHTRED, BLACK, "Comando invalido.\n");
                return 0;
            }

       a_prepare.alarm_time=cron_time;
       Insert(alarms,&a_prepare);

      return 0;



  }

  int compAlarm(alarm *a1, alarm *a2) {
    int difference;
    long a1secs = toBCD((a1->alarm_time).day) * 86400 + 
        toBCD((a1->alarm_time).hour) * 3600 + 
        toBCD((a1->alarm_time).minute) * 60 + 
        toBCD((a1->alarm_time).second);
    long a2secs = toBCD((a2->alarm_time).day) * 86400 +
        toBCD((a2->alarm_time).hour) * 3600 +
        toBCD((a2->alarm_time).minute) * 60 +
        toBCD((a2->alarm_time).second);
    difference = a1secs - a2secs;
    if(difference> 0)
        return 1;
    if(difference < 0)
        return -1;
    return 0;
}
