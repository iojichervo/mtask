#include "../include/rtc.h"
#include <kernel.h>
#include <apps.h>
static int calendar_flag=0;
time_t curr_time;

int first = 1;

enum {
      cmos_address = 0x70,
      cmos_data    = 0x71
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

      
 
      // Note: This uses the "read registers until you get the same values twice in a row" technique
      //       to avoid getting dodgy/inconsistent values due to RTC updates
 
      while (get_update_in_progress_flag());                // Make sure an update isn't in progress
      curr_time.second = get_RTC_register(0x00);
      curr_time.minute = get_RTC_register(0x02);
      curr_time.hour = get_RTC_register(0x04);
      curr_time.day = get_RTC_register(0x07);
      curr_time.month = get_RTC_register(0x08);
      curr_time.year = get_RTC_register(0x09);
      if(first) {
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
 
            while (get_update_in_progress_flag());           // Make sure an update isn't in progress
            curr_time.second = get_RTC_register(0x00);
            curr_time.minute = get_RTC_register(0x02);
            curr_time.hour = get_RTC_register(0x04);
            curr_time.day = get_RTC_register(0x07);
            curr_time.month = get_RTC_register(0x08);
            curr_time.year = get_RTC_register(0x09);
      } while( (last_second != curr_time.second) || (last_minute != curr_time.minute) || (last_hour != curr_time.hour) ||
               (last_day != curr_time.day) || (last_month != curr_time.month) || (last_year != curr_time.year) );
 
      registerB = get_RTC_register(0x0B);
 
      // Convert BCD to binary values if necessary
 
      if (!(registerB & 0x04)) {
            curr_time.second = (curr_time.second & 0x0F) + ((curr_time.second / 16) * 10);
            curr_time.minute = (curr_time.minute & 0x0F) + ((curr_time.minute / 16) * 10);
            curr_time.hour = ( (curr_time.hour & 0x0F) + (((curr_time.hour & 0x70) / 16) * 10) ) | (curr_time.hour & 0x80);
            curr_time.day = (curr_time.day & 0x0F) + ((curr_time.day / 16) * 10);
            curr_time.month = (curr_time.month & 0x0F) + ((curr_time.month / 16) * 10);
            curr_time.year = (curr_time.year & 0x0F) + ((curr_time.year / 16) * 10);
      }
 
      // Convert 12 hour clock to 24 hour clock if necessary
 
      if (!(registerB & 0x02) && (curr_time.hour & 0x80)) {
            curr_time.hour = ((curr_time.hour & 0x7F) + 12) % 24;
      }
 
      // Calculate the full (4-digit) year
 
      //curr_time.year += curr_time.century * 100;
      curr_time.year += (curr_time.year / 100) * 100;
      if(curr_time.year < curr_time.year) curr_time.year += 100;
}

int print_date(){
      read_rtc();
      printk("%d/%d/%d%d - %d:%d:%d\n",curr_time.day,curr_time.month,curr_time.century-1,curr_time.year,curr_time.hour,curr_time.minute,curr_time.second,curr_time.century);
      return 0;
}

int set_date(int argc,char** argv){
   char year[3];
   char cent[3];

   if(argc != 7){
      cprintk(LIGHTRED,BLACK,"Los parametros necesarios deben ser 6.\n");
   } else {
    curr_time.hour = toBCD(atoi(argv[1]));
    curr_time.minute = toBCD(atoi(argv[2]));
    curr_time.second = toBCD(atoi(argv[3]));
    curr_time.day = toBCD(atoi(argv[4]));
    curr_time.month =toBCD(atoi(argv[5]));
    //No esta implementado substring
    year[0] = argv[6][2];
    year[1] = argv[6][3];
    year[2] = 0;
    curr_time.year =toBCD(atoi(year));
    curr_time.century = atoi(strncat(cent,argv[6],2)) + 1;
    set_time();
   } 
   printk("La nueva fecha es: ");
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
   calendar_flag=1;
   int m = month_first_date();
   unsigned  last_x,last_y;
   int i,j,ctr=1;

   if(first)
      read_rtc();

   mt_cons_getxy(&last_x,&last_y);
   mt_cons_gotoxy(61,0);
   cprintk(BLACK, LIGHTRED,"    %d   -   %d    \n", curr_time.month,curr_time.year);
   mt_cons_gotoxy(61,1);
   cprintk(BLACK, LIGHTRED,"L  M  M  J  V  S  D\n");
   
   for(j=2;j<8;j++){
      mt_cons_gotoxy(61, j);
      for(i=0;i<6;i++){
         cprintk(BLACK, RED,"%d  ",ctr++);
      }
   }
 
//   mt_cons_gotoxy(last_x,last_y);
   return 0;
 }

int month_first_date(){
   int h,q,k,j,m,y;
  
  y = curr_time.year + (curr_time.century-1)*100;
  m = curr_time.month;
  if(m == 1)
  {
    m = 13;
    y--;
  }

  if (m == 2)
  {
    m = 14;
    y--;
  }
  
  q = 1; //El primer dia del mes
  k = y % 100;
  j = y / 100;
  h = q + 13*(m+1)/5 + k + k/4 + j/4 + 5*j;
  h = h % 7;
  switch(h)
  {
    case 0 : return 5;
    case 1 : return 6;
    case 2 : return 0;
    case 3 : return 1;
    case 4 : return 2;
    case 5 : return 3;
    case 6 : return 4; 
  }
return -1;
}