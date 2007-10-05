/* Driver for the NXT ultrasonic radar.
 *
 * This driver provides a high level interface to the NXT ultrasonic
 * radar. The radar is the first digital sensor for the NXT, and thus
 * makes use of the I2C communication protocol over the two wire
 * interface of DIGIxI0 + DIGIxI1. See drivers/i2c.{c,h}.
 *
 * Among the functionnalities provided by this sensor, this driver
 * supports the following features:
 */

#include "at91sam7s256.h"

#include "mytypes.h"
#include "nxt.h"
#include "avr.h"
#include "interrupts.h"
#include "systick.h"
#include "sensors.h"
#include "display.h"
#include "i2c.h"
#include "usb.h"

/* As defined in the NXT Hardware Developer Kit, the Ultrasonic sensor
 * has been given address 1 (within a 7 bit context).
 */
#define RADAR_I2C_ADDRESS 0x1

/** Radar's internal memory addresses.
 *
 * This enum contains the radar's internal memory addresses of the
 * radar parameters and readings.
 */
typedef enum {
  RADAR_VERSION = 0x00,

  RADAR_PRODUCT_ID = 0x08,

  RADAR_SENSOR_TYPE = 0x10,
  RADAR_FACTORY_ZERO,
  RADAR_FACTORY_SCALE_FACTOR,
  RADAR_FACTORY_SCALE_DIVISOR,
  RADAR_MEASUREMENT_UNITS,

  RADAR_INTERVAL = 0x40,
  RADAR_OP_MODE,
  RADAR_R0,
  RADAR_R1,
  RADAR_R2,
  RADAR_R3,
  RADAR_R4,
  RADAR_R5,
  RADAR_R6,
  RADAR_R7,
  RADAR_CURRENT_ZERO,
  RADAR_CURRENT_SCALE_FACTOR,
  RADAR_CURRENT_SCALE_DIVISOR,
} radar_commands;

extern U32 offset;
extern U8 dump[1024];

/** Initializes the radar sensor. */
void radar_init(U8 sensor)
{
  sensors_i2c_enable(sensor);
  i2c_register(sensor, RADAR_I2C_ADDRESS);
}

void radar_display_lines(U8 sensor)
{
  U32 lines = *AT91C_PIOA_PDSR;

  sensor_pins pins = sensors_get_pins(sensor);
  display_string("[");
  display_uint(lines & pins.sda ? 1 : 0);
  display_string("/");
  display_uint(lines & pins.scl ? 1 : 0);
  display_string("]\n");
}

void radar_txn(U8 sensor, U8 *data, U8 size, i2c_txn_mode mode)
{
  i2c_txn_err err;
  i2c_txn_status status;

  err = i2c_start_transaction(sensor, data, size, mode);
  if (err != I2C_ERR_OK) {
    display_string("EE txn (");
    display_uint(err);
    display_string(") !\n");
  } else {
    while (i2c_get_txn_status(sensor) == TXN_STAT_IN_PROGRESS);
    systick_wait_ms(50);
    status = i2c_get_txn_status(sensor);
    if (status == TXN_STAT_SUCCESS) {
      if (mode == TXN_MODE_READ) {
        display_string("OK:");
        display_uint(data[0]);
        display_string(". ");
      } else {
        display_string("OK. ");
      }
    }

    else {
      display_string("EE (");
      display_uint(status);
      display_string(") ");
    }
  }

  radar_display_lines(sensor);
}

void radar_test(U8 sensor)
{
  /* Send the command READ_SENSOR_TYPE */
  display_clear();
  display_cursor_set_pos(0, 0);
  display_string(">> send command\n");

  U8 cmd = 0x55; /* 0b01010101 */
  radar_txn(sensor, &cmd, 1, TXN_MODE_WRITE);
  systick_wait_ms(1500);
  display_cursor_set_pos(0, 0);
  display_string("+");
  while (avr_get_button() != BUTTON_OK);

  /* Try to read result */
  display_clear();
  display_cursor_set_pos(0, 0);
  display_string("<< read result\n");

  U8 buf[2] = { 0x42 };
  radar_txn(sensor, buf, 1, TXN_MODE_READ);

  systick_wait_ms(1500);
  display_cursor_set_pos(0, 0);
  display_string("+");
  while (avr_get_button() != BUTTON_OK);

  display_clear();
  display_cursor_set_pos(0, 0);

  int i, j, k = 12;
  for (i=0 ; i<8 ; i++) {
    for (j=0 ; j<k ; j++) {
      display_uint(dump[i*k + j]);
      if (j % 2 == 1)
        display_string(" ");
    }
    display_end_line();
  }

  systick_wait_ms(1500);
  while (avr_get_button() != BUTTON_OK);
}
