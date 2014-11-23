#include <kernel.h>
#include <apps.h>
#include "../include/rtc.h"
#include "../include/genlistADT.h"

/*	Funciones que se pueden llamar desde el shell:
- date
Imprime la fecha actual.
-set_date [hh] [mm] [ss] [dd] [mm] [aaaa]
Guarda la fecha indicada por los parametros hh es hora, mm son minutos, ss son segundos, 
dd es el dia, mm es el minuto y aaaa es el año.
-calendar
Habilita el calendario en la consola 0. Si se lo vuelve a ejecutar, se elimina.
-cron [dd] [hh] [mm] [ss] [app]
Ejecuta el comando [app] en el dia [dd] a la hora [hh]:[mm]:[ss]. Se utiliza la interrupción
del CMOS, por lo que solo se permite indicar ese momento de tiempo.
-periodiccron [s] [app]
Ejecuta el comando [app] cada [s] segundos.
-timer [ms] [app]
Ejecuta el comando [app] cada [ms] milisegundos.
*/

/* Almacena el tiempo actual. */
time_t curr_time;

/* Flag para indicar si curr_time tiene datos cargados. */
int first = 1;

/* Indica si el calendario se encuentra en pantalla (consola 0). */
int cal_on = 0;

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

/* Almacena las tareas creadas por cron */
listADT tasks = NULL;

int tasksC = 0;

/* Timers creados. Máximo de 10. */
timer_t timers[10];

int timersC = 0;

unsigned long intC = 0;

static struct func_t {
    char *name;
    int (*func)(int argc, char **argv);
}

cmdtab[] =
{
	{	"setkb",		setkb_main, 		},
	{	"shell",		shell_main,			},
	{	"sfilo",		simple_phil_main,	},
	{	"filo",			phil_main,			},
	{	"xfilo",		extra_phil_main,	},
	{	"afilo",		atomic_phil_main,	},
	{	"camino",		camino_main,		},
	{	"camino_ns",	camino_ns_main,		},
	{	"prodcons",		prodcons_main,		},
	{	"divz",			divz_main,			},
	{	"pelu",			peluqueria_main,	},
	{	"events",		events_main,		},
	{	"disk",			disk_main,			},
	{	"ts", 			ts_main,			},
	{	"kill",			kill_main,			},
	{	"test",			test_main,			},
	{	"date",			print_date,			},
	{	"set_date",		set_date,			},
	{	"calendar",		calendar,			},
	{										}
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

  /* Se leen los registros hasta que se obtengan los mismos valores dos veces para evitar datos
   inconsistentes por la actualización del RTC */

	while (get_update_in_progress_flag());                /* Se asegura que no se este actualizando */
	curr_time.second = get_RTC_register(0x00);
	curr_time.minute = get_RTC_register(0x02);
	curr_time.hour = get_RTC_register(0x04);
	curr_time.day = get_RTC_register(0x07);
	curr_time.month = get_RTC_register(0x08);
	curr_time.year = get_RTC_register(0x09);
	if(first) {											/* La primera vez, el siglo es 21 */
		curr_time.century = 21;
		first = !first;
	}

	do {
  		last_second = curr_time.second;
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

  /* Se convierte a BCD en caso necesario */

	if (!(registerB & 0x04)) {
  		curr_time.second = fromBCD(curr_time.second);
  		curr_time.minute = fromBCD(curr_time.minute);
  		curr_time.hour = fromBCD(curr_time.hour);
  		curr_time.day = fromBCD(curr_time.day);
  		curr_time.month = fromBCD(curr_time.month);
  		curr_time.year = fromBCD(curr_time.year);
	}

  /* Se convierte en formato de 12 horas a 24 */

	if (!(registerB & 0x02) && (curr_time.hour & 0x80)) {
  		curr_time.hour = ((curr_time.hour & 0x7F) + 12) % 24;
  	}

  /* Calculo final del año */

  // curr_time.year += curr_time.century * 100;
  	curr_time.year += (curr_time.year / 100) * 100;
  	if(curr_time.year < curr_time.year)
  		curr_time.year += 100;

	curr_time.week_day = day_of_the_week();
}

int print_date(){
	read_rtc();

	/* Formato: jue dic 18 15:02:46 2014 */
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
		curr_time.month = toBCD(atoi(argv[5]));

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
	printk("La nueva fecha es: ");
	print_date();
	if(cal_on) {
		int cons = mt_cons_current();
		cal_on = !cal_on;
		calendar();
		mt_cons_setfocus(cons);
	}
	return 0;
}

int toBCD(int n) {
	return ((n / 10) << 4)|(n% 10);
}

int fromBCD(int n) {
	return ((n & 0xF0) >> 4) * 10 + (n & 0x0F);
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

	if(cal_on)
		t = CreateTask(delete_calendar, MAIN_STKSIZE, NULL, "dcalendar", DEFAULT_PRIO);
	else
		t = CreateTask(run_calendar, MAIN_STKSIZE, NULL, "calendar", DEFAULT_PRIO);
	cal_on = !cal_on;
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

int delete_calendar(void *arg) {
	int x, j;
	char * days = "                     ";

	x = mt_cons_ncols() - strlen(days);
	for(j = 0; j < 10; j++) {
		mt_cons_gotoxy(x, j);
		cprintk(BLACK, BLACK, days);
	}

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
	int aux;
	int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if(curr_time.month == 2) {
		aux = curr_time.year + (curr_time.century-1)*100;
		if(is_leap_year(aux))
			return 29;
	}

	return days[curr_time.month-1];
}

int is_leap_year(int year) {
	return ( (year%4==0) && !(year%100==0) ) || (year%400==0);
}

int cron(int argc,char** argv) {
    time_t cron_t;

    mt_enable_irq(8);
    mt_set_int_handler(8, rtc_handler);
    habilitate_cmos_int();

    if(tasks == NULL)
        tasks = NewList(sizeof(task), (int(*)(void *, void *)) taskcp);

    cron_t.day = toBCD(atoi(argv[1]));
    cron_t.hour = toBCD(atoi(argv[2]));
    cron_t.minute = toBCD(atoi(argv[3]));
    cron_t.second = toBCD(atoi(argv[4]));

    task a;
    a.id = tasksC++;
    a.name = argv[5];
    a.repeat = 0;
    a.console = mt_cons_current();

    a.argv = Malloc(1 * sizeof(char *));
    a.argv[0] = Malloc(strlen(argv[5] + 1));
    strcpy(a.argv[0], argv[5]);
    a.argc = 1;

    a.date = cron_t;
    Insert(tasks, &a);

    ToBegin(tasks);
	task * first = NextElement(tasks);

	set_alarm(&(first -> date));
    return 0;
}

int periodiccron(int argc,char** argv) {
    int secs;
    time_t now;

    mt_enable_irq(8);
    mt_set_int_handler(8, rtc_handler);
    habilitate_cmos_int();

    if(tasks == NULL)
        tasks = NewList(sizeof(task), (int(*)(void *, void *)) taskcp);

    read_rtc();
    now = curr_time;
    secs = atoi(argv[1]);
    addSeconds(&now, secs);

    task a;
    a.id = tasksC++;
    a.name = argv[2];
    a.repeat = 1;
    a.secs = secs;
    a.console = mt_cons_current();

    a.argv = Malloc(1 * sizeof(char *));
    a.argv[0] = Malloc(strlen(argv[2] + 1));
    strcpy(a.argv[0], argv[2]);
    a.argc = 1;

    a.date = now;
    Insert(tasks, &a);

    ToBegin(tasks);
	task * first = NextElement(tasks);
    set_alarm(&(first -> date));
    return 0;
}

int taskcp(task *a1, task *a2) {
    int diff;

    long a1secs = fromBCD((a1->date).day) * 86400 + 
        fromBCD((a1->date).hour) * 3600 + 
        fromBCD((a1->date).minute) * 60 + 
        fromBCD((a1->date).second);
    long a2secs = fromBCD((a2->date).day) * 86400 +
        fromBCD((a2->date).hour) * 3600 +
        fromBCD((a2->date).minute) * 60 +
        fromBCD((a2->date).second);
    diff = a1secs - a2secs;
    if(diff > 0)
        return 1;
    if(diff < 0)
        return -1;
    if(strcmp(a1->name, a2->name) == 0)
        return 0;
    else
        return 1;
}

void rtc_handler(unsigned irq_number) {
    struct func_t * com;
    task * f;
    task aux;

    get_RTC_register(0x0C);

    if(ListIsEmpty(tasks))
        return;

    ToBegin(tasks);
    f = NextElement(tasks);
    aux = * f;

    if(aux.repeat) {
        addSeconds(&(aux.date), aux.secs);
        Insert(tasks, &aux);
    }
 
 	/* Ejecución del comando agendado. */
    for ( com = cmdtab ; com->name ; com++ ) {
        if ( strcmp(f->name, com->name) == 0 ) {
        	mt_cons_setcurrent(f->console);
            int n = com->func(f->argc, f->argv);

            if ( n != 0 )
                cprintk(LIGHTRED, BLACK, "Status: %d\n", n);

            break;
        }
    }

    Delete(tasks, f);
    ToBegin(tasks);
    f = NextElement(tasks);
    if(f != NULL) {
        set_alarm(&(f -> date));
    }
}

void addSeconds(time_t * t, int secs) {
	int dephase = t->second + secs;

    if(dephase < 60) {
        t->second = dephase;
    } else {
        if(++t->minute == 60) {
    		t->minute = 0;
    		t->hour++;
    	}
        t->second = secs % 60;
    }

}

void habilitate_cmos_int() {
    unsigned reg_b;
    
    Atomic();
    reg_b = get_RTC_register(0x0B);
	reg_b |= (0x01 << 5);
	set_RTC_register(0x0B, reg_b);
    Unatomic();
}

void set_alarm(time_t * tp) {
    Atomic();
    set_RTC_register(0x01, toBCD(tp->second));
    set_RTC_register(0x03, toBCD(tp->minute));
    set_RTC_register(0x05, toBCD(tp->hour));
    set_RTC_register(0x0D, toBCD(tp->day));
    Unatomic();
}

int timer(int argc,char** argv) {
	unsigned reg_a, reg_b;

	/* Interrupcion cada 1 ms. */
	reg_a = get_RTC_register(0x0A);
	reg_a &= 0xF0;
	reg_a |= 0x06;
	set_RTC_register(0x0A, reg_a);
	
	/* Interrupciones periodicas. */
	reg_b = get_RTC_register(0x0B);
	reg_b |= (0x01 << 6);
	set_RTC_register(0x0B, reg_b);

	mt_enable_irq(8);
    mt_set_int_handler(8, timer_handler);

    if(timersC == 10) {
    	printk("La cantidad de timers actuales es máxima.\n");
    	return 0;
    }

    timer_t a;
    a.name = argv[2];
    a.msecs = atoi(argv[1]);
    a.console = mt_cons_current();

    a.argv = Malloc(1 * sizeof(char *));
    a.argv[0] = Malloc(strlen(argv[2] + 1));
    strcpy(a.argv[0], argv[2]);
    a.argc = 1;

    if(a.msecs > 0)
	    timers[timersC++] = a;
	else
		printk("La ejecucion del timer debe ser con un tiempo mayor a cero.");
    return 0;
}

void timer_handler(unsigned irq_number) {
	struct func_t * com;
    timer_t f;
    int i;

    get_RTC_register(0x0C);

    if(timers == NULL)
        return;

    intC++;

 	for(i = 0; i < timersC; i++){
 		f = timers[i];
 		if(intC % f.msecs == 0) {
		    for ( com = cmdtab ; com->name ; com++ ) {
		        if ( strcmp(f.name, com->name) == 0 ) {
		        	mt_cons_setcurrent(f.console);
		            int n = com->func(f.argc, f.argv);

		            if ( n != 0 )
		                cprintk(LIGHTRED, BLACK, "Status: %d\n", n);

		            break;
		        }
		    }
		}
	}
}