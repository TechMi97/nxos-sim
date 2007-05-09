/* Various test routines for components of the NXT. */

#include "at91sam7s256.h"

#include "mytypes.h"
#include "interrupts.h"
#include "aic.h"
#include "systick.h"
#include "avr.h"
#include "twi.h"
#include "lcd.h"
#include "display.h"
#include "sound.h"
#include "memmap.h"
#include "sensors.h"
#include "motors.h"
#include "usb.h"


static bool test_silent = FALSE;

static void hello() {
  if (test_silent)
    return;
  sound_freq(1000, 100);
  systick_wait_ms(50);
  sound_freq(2000, 100);
  systick_wait_ms(900);
}

static void goodbye() {
  if (test_silent)
    return;
  sound_freq(2000, 100);
  systick_wait_ms(50);
  sound_freq(1000, 100);
  systick_wait_ms(900);
}

void beep_word(U32 value) {
  U32 i=32;

  hello();

  while (i > 0 && !(value & 0x80000000)) {
    value <<= 1;
    i--;
  }
  while (i > 0) {
    if (value & 0x80000000)
      sound_freq(2000, 300);
    else
      sound_freq(1000, 300);
    systick_wait_ms(700);
    value <<= 1;
    i--;
  }

  goodbye();
}


void tests_display() {
  char buf[2] = { 0, 0 };
  int i;

  hello();

  display_clear();
  display_cursor_set_pos(0, 0);

  display_string("- Display test -\n"
                 "----------------\n");
  for (i=32; i<128; i++) {
    buf[0] = i;
    display_string(buf);
    if ((i % 16) == 15)
      display_string("\n");
  }

  systick_wait_ms(5000);
  goodbye();
}


void tests_sound() {
  enum {
    end = 0, sleep500 = 1, si = 990, dod = 1122,
    re = 1188, mi = 1320, fad = 1496, sol = 1584,
  } pain[] = {
    si, sleep500,
    fad, si, sol, sleep500,
    fad, mi, fad, sleep500,
    mi, fad, sol, sol, fad, mi, si, sleep500,
    fad, si, sol, sleep500,
    fad, mi, re,  sleep500,
    mi, re,  dod, dod, re,  dod, si, end
  };
  int i = 0;

  hello();

  display_clear();
  display_cursor_set_pos(0,0);
  display_string("-- Sound test --\n"
                 "----------------\n");

  while (pain[i] != end) {
    if (pain[i] == sleep500)
      systick_wait_ms(150);
    else
      sound_freq(pain[i], 150);
    systick_wait_ms(150);
    i++;
  }

  systick_wait_ms(1000);
  goodbye();
}


void
tests_motor() {
  hello();

  display_clear();
  display_cursor_set_pos(0,0);
  display_string("--- AVR test ---\n"
                 "----------------\n");

  avr_set_motor(0, 80, 0);
  systick_wait_ms(1000);

  avr_set_motor(0, -80, 0);
  systick_wait_ms(1000);

  avr_set_motor(0, 80, 0);
  systick_wait_ms(1000);

  avr_set_motor(0, 0, 1);
  systick_wait_ms(200);

  goodbye();
}


void tests_tachy() {
  int i;
  hello();

  motors_rotate_angle(0, 80, 1024, TRUE);
  motors_rotate_time(1, -80, 3000, FALSE);
  motors_rotate(2, 80);

  for (i=0; i<30; i++) {
    display_clear();
    display_cursor_set_pos(0,0);

    display_clear();
    display_cursor_set_pos(0,0);
    display_string("Tachymeter  test\n"
                   "----------------\n");

    display_string("Tach A: ");
    display_hex(motors_get_tach_count(0));
    display_end_line();

    display_string("Tach B: ");
    display_hex(motors_get_tach_count(1));
    display_end_line();

    display_string("Tach C: ");
    display_hex(motors_get_tach_count(2));
    display_end_line();

    display_string("Refresh: ");
    display_uint(i);
    display_end_line();

    systick_wait_ms(250);
  }

  motors_stop(2, TRUE);

  goodbye();
}


void tests_sensors() {
  U32 i;
  const U32 display_seconds = 15;
  hello();

  sensors_analog_enable(0);

  for (i=0; i<(display_seconds*4); i++) {
    display_clear();
    display_cursor_set_pos(0,0);
    display_string("- Sensor  info -\n"
                   "----------------\n");

    display_string("Port 1: ");
    display_uint(sensors_analog_get(0));
    display_end_line();

    display_string("Port 2: ");
    display_uint(sensors_analog_get(1));
    display_end_line();

    display_string("Port 3: ");
    display_uint(sensors_analog_get(2));
    display_end_line();

    display_string("Port 4: ");
    display_uint(sensors_analog_get(3));
    display_end_line();

    systick_wait_ms(250);
  }

  goodbye();
}


void tests_sysinfo() {
  U32 i, t;
  const U32 display_seconds = 15;
  U8 avr_major, avr_minor;
  hello();

  avr_get_version(&avr_major, &avr_minor);

  for (i=0; i<(display_seconds*4); i++) {
    if (i % 4 == 0)
      t = systick_get_ms();

    display_clear();
    display_cursor_set_pos(0,0);
    display_string("- System  info -\n"
                   "----------------\n");

    display_string("Time  : ");
    display_uint(t);
    display_end_line();

    display_string("Boot from ");
    if (BOOT_FROM_SAMBA)
      display_string("SAM-BA");
    else
      display_string("ROM");
    display_end_line();

    display_string("Free RAM: ");
    display_uint(USERSPACE_SIZE);
    display_end_line();

    display_string("Buttons: ");
    display_uint(avr_get_button());
    display_end_line();

    display_string("AVR Ver.: ");
    display_uint(avr_major);
    display_string(".");
    display_uint(avr_minor);

    systick_wait_ms(250);
  }

  goodbye();
}



/* returns 1 if they are identic
 * 0 else
 */
static U8 compare_str(char *str_a, char *str_b, U32 max)
{

  while (*str_a != '\0'
	 && *str_b != '\0'
	 && max > 0)
    {
      if (*str_a != *str_b) {
	return 0;
      }
      str_a++;
      str_b++;
      max--;
    }

  if (*str_a != *str_b && max > 0) {
    return 0;
  }

  return 1;
}


#define USB_UNKNOWN "Unknown"
#define USB_OK      "Ok"

void tests_all();

void tests_usb() {
  U16 i;
  U32 lng;
  char *buffer;

  hello();

  buffer = (char *)usb_get_buffer();

  while(1) {
    for (i = 0 ; i < 500 && !usb_has_data(); i++)
    {
      usb_display_debug();
      systick_wait_ms(200);
    }

    if (i >= 500)
      break;

    lng = usb_has_data();


    display_cursor_set_pos(0, 0);
    display_string("==");
    display_uint(lng);

    display_cursor_set_pos(0, 1);
    display_string(buffer);

    display_cursor_set_pos(0, 2);
    display_hex((U32)(USB_UNKNOWN));

    /* Start interpreting */

    i = 0;
    if (compare_str(buffer, "motor", lng))
      tests_motor();
    else if (compare_str(buffer, "sound", lng))
      tests_sound();
    else if (compare_str(buffer, "display", lng))
      tests_display();
    else if (compare_str(buffer, "sysinfo", lng))
      tests_sysinfo();
    else if (compare_str(buffer, "sensors", lng))
      tests_sensors();
    else if (compare_str(buffer, "tachy", lng))
      tests_tachy();
    else if (compare_str(buffer, "all", lng))
      tests_all();
    else if (compare_str(buffer, "halt", lng))
      break;
    else {
      i = 1;
      usb_send((U8 *)USB_UNKNOWN, sizeof(USB_UNKNOWN));
    }

    if (i == 0)
      usb_send((U8 *)USB_OK, sizeof(USB_OK));

    /* Stop interpreting */

    systick_wait_ms(500);

    display_clear();

    usb_flush_buffer();

  }

  goodbye();
}


void tests_all() {
  test_silent = TRUE;

  tests_display();
  tests_sound();
  tests_motor();
  tests_tachy();
  tests_sensors();
  tests_sysinfo();

  test_silent = FALSE;
  goodbye();
}
