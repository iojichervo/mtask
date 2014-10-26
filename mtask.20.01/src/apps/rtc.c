#include "../include/rtc.h"
#include <kernel.h>
#include <apps.h>

int century_register = 0x00;                                // Set by ACPI table parsing code if possible

time_t curr_time;

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
      unsigned char century;
      unsigned char last_second;
      unsigned char last_minute;
      unsigned char last_hour;
      unsigned char last_day;
      unsigned char last_month;
      unsigned char last_year;
      unsigned char last_century;
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
      if(century_register != 0) {
            century = get_RTC_register(century_register);
      }
 
      do {
            last_second =curr_time.second;
            last_minute = curr_time.minute;
            last_hour = curr_time.hour;
            last_day = curr_time.day;
            last_month = curr_time.month;
            last_year = curr_time.year;
            last_century = century;
 
            while (get_update_in_progress_flag());           // Make sure an update isn't in progress
            curr_time.second = get_RTC_register(0x00);
            curr_time.minute = get_RTC_register(0x02);
            curr_time.hour = get_RTC_register(0x04);
            curr_time.day = get_RTC_register(0x07);
            curr_time.month = get_RTC_register(0x08);
            curr_time.year = get_RTC_register(0x09);
            if(century_register != 0) {
                  century = get_RTC_register(century_register);
            }
      } while( (last_second != curr_time.second) || (last_minute != curr_time.minute) || (last_hour != curr_time.hour) ||
               (last_day != curr_time.day) || (last_month != curr_time.month) || (last_year != curr_time.year) ||
               (last_century != century) );
 
      registerB = get_RTC_register(0x0B);
 
      // Convert BCD to binary values if necessary
 
      if (!(registerB & 0x04)) {
            curr_time.second = (curr_time.second & 0x0F) + ((curr_time.second / 16) * 10);
            curr_time.minute = (curr_time.minute & 0x0F) + ((curr_time.minute / 16) * 10);
            curr_time.hour = ( (curr_time.hour & 0x0F) + (((curr_time.hour & 0x70) / 16) * 10) ) | (curr_time.hour & 0x80);
            curr_time.day = (curr_time.day & 0x0F) + ((curr_time.day / 16) * 10);
            curr_time.month = (curr_time.month & 0x0F) + ((curr_time.month / 16) * 10);
            curr_time.year = (curr_time.year & 0x0F) + ((curr_time.year / 16) * 10);
            if(century_register != 0) {
                  century = (century & 0x0F) + ((century / 16) * 10);
            }
      }
 
      // Convert 12 hour clock to 24 hour clock if necessary
 
      if (!(registerB & 0x02) && (curr_time.hour & 0x80)) {
            curr_time.hour = ((curr_time.hour & 0x7F) + 12) % 24;
      }
 
      // Calculate the full (4-digit) year
 
      if(century_register != 0) {
            curr_time.year += century * 100;
      } else {
            curr_time.year += (curr_time.year / 100) * 100;
            if(curr_time.year < curr_time.year) curr_time.year += 100;
      }

     
     
}

int print_date(){
      read_rtc();
      printk("%d/%d/%d - %d:%d:%d\n",curr_time.day,curr_time.month,curr_time.year,curr_time.hour,curr_time.minute,curr_time.second);
      return 0;
}

int set_date(int argc,char** argv){
   if(argc != 7){
      cprintk(LIGHTRED,BLACK,"Los parametros necesarios deben ser 6.\n");
   } else {
//      curr_time.hour = toBCD((int)argv[1]);
 //   curr_time.minute = toBCD((int)argv[2]);
  //  curr_time.second = toBCD((int)argv[3]);
   // curr_time.day = toBCD((int)argv[4]);
   // curr_time.month = toBCD((int)argv[5] - 1);
   // curr_time.year = toBCD((int)argv[6]);
            curr_time.hour = toBCD(atoi(argv[1]));
    curr_time.minute = toBCD(atoi(argv[2]));
    curr_time.second = toBCD(atoi(argv[3]));
    curr_time.day = toBCD(atoi(argv[4]));
    curr_time.month =toBCD(atoi(argv[5]));
    curr_time.year =toBCD(atoi(argv[6]));
    set_time();
   } 
   printk("La nueva hora es: ");
   print_date();
   return 0;
}

int toBCD(int n) {
   return ((n / 10) << 4)|(n% 10);
}

void set_time(){
      set_RTC_register(0x09,0000);
      set_RTC_register(0x00,curr_time.second);
       set_RTC_register(0x02, curr_time.minute);
      set_RTC_register(0x04,curr_time.hour );
      set_RTC_register(0x07, curr_time.day);
      set_RTC_register(0x08, curr_time.month);
      set_RTC_register(0x09, curr_time.year);
     
}
