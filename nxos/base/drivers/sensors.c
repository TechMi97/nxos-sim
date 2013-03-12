/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/assert.h"
#include "base/drivers/systick.h"
#include "base/drivers/aic.h"
#include "base/drivers/_avr.h"
#include "base/drivers/rs485.h"

#include "base/drivers/_sensors.h"

static const nx__sensors_pins sensors_pinmap[NXT_N_SENSORS] = {
  { AT91C_PIO_PA23, AT91C_PIO_PA18 },
  { AT91C_PIO_PA28, AT91C_PIO_PA19 },
  { AT91C_PIO_PA29, AT91C_PIO_PA20 },
  { AT91C_PIO_PA30, AT91C_PIO_PA2  },
};

static const nx__sensors_adcmap sensors_adcmap[NXT_N_SENSORS] = {
  { AT91C_ADC_CH1, (volatile U32 *) AT91C_ADC_CDR1 },
  { AT91C_ADC_CH2, (volatile U32 *) AT91C_ADC_CDR2 },
  { AT91C_ADC_CH3, (volatile U32 *) AT91C_ADC_CDR3 },
  { AT91C_ADC_CH7, (volatile U32 *) AT91C_ADC_CDR7 },
};

static enum {
  OFF = 0, /* Unused. */
  LEGACY,  /* Legacy RCX sensor support (Not currently supported). */
  ANALOG,  /* NXT sensor in analog mode. */
  DIGITAL, /* NXT sensor in digital (i2c) mode. */
  LEGOCOLOR, /* NXT Color Sensor (hybrid) mode */
} sensors_mode[NXT_N_SENSORS] = {
  /* All sensors start off. */
  OFF, OFF, OFF, OFF
};

void nx__sensors_init(void) {
  U32 pinmask = 0;
  int i;

  for (i=0; i<NXT_N_SENSORS; i++) {
    pinmask |= sensors_pinmap[i].sda | sensors_pinmap[i].scl;
  }

  /* Disable output on all DIGI0/1 pins, which will set the lines high
   * due to the internal PIO pull-up resistor. We will keep the lines
   * in this idle state until a sensor driver tells us what to do with
   * the lines.
   */
  *AT91C_PIOA_PER = pinmask;
  *AT91C_PIOA_PPUDR = pinmask;					/* Disable Pullups */
  *AT91C_PIOA_ODR = pinmask;

  nx_rs485_switch_to_pioa();					/* Make sure that we disable RS485 */

}

void nx__sensors_i2c_enable(U32 sensor) {
  U32 pinmask;

  NX_ASSERT(sensor < NXT_N_SENSORS);
  NX_ASSERT(sensors_mode[sensor] == OFF);

  sensors_mode[sensor] = DIGITAL;

  /* In digital mode, the DIGI outputs (SDA and SCL) are left up, and
   * enabled in multi-drive mode.
   */
  pinmask = sensors_pinmap[sensor].sda |
    sensors_pinmap[sensor].scl;

  *AT91C_PIOA_OER = pinmask;
//  *AT91C_PIOA_PPUDR = pinmask;					/* Disable Pullups */
  *AT91C_PIOA_SODR = pinmask;
  *AT91C_PIOA_MDER = pinmask;

#if 0
  if (sensor == 3)
	  nx_rs485_switch_to_pioa();				/* Paranoid */
#endif
}

void nx__sensors_color_enable(U32 sensor) {
  U32 pinmask;

  NX_ASSERT(sensor < NXT_N_SENSORS);
  NX_ASSERT(sensors_mode[sensor] == OFF);

  sensors_mode[sensor] = LEGOCOLOR;

  /* In color sensor mode, the DIGI outputs (SDA and SCL) are left up. */
  pinmask = sensors_pinmap[sensor].sda |
    sensors_pinmap[sensor].scl;

  *AT91C_PIOA_OER = pinmask;
  *AT91C_PIOA_SODR = pinmask;

#if 0
  if (sensor == 3)
	  nx_rs485_switch_to_pioa();				/* Paranoid */
#endif
}

const nx__sensors_pins *nx__sensors_get_pins(U32 sensor) {
  NX_ASSERT(sensor < NXT_N_SENSORS);

  return &sensors_pinmap[sensor];
}

const nx__sensors_adcmap *nx__sensors_get_adcmap(U32 sensor) {
  NX_ASSERT(sensor < NXT_N_SENSORS);

  return &sensors_adcmap[sensor];
}

void nx__sensors_disable(U32 sensor) {
  U32 pinmask;

  NX_ASSERT(sensor < NXT_N_SENSORS);
  if (sensor >= NXT_N_SENSORS)
    return;


  switch (sensors_mode[sensor]) {
  case OFF:
  case LEGACY:
    break;
  case ANALOG:
  case DIGITAL:
  case LEGOCOLOR:
	pinmask = sensors_pinmap[sensor].sda |
	    sensors_pinmap[sensor].scl;
    /* Disable output on the DIGI pins to return to the idle state. */
    *AT91C_PIOA_SODR = pinmask;
    *AT91C_PIOA_ODR = pinmask;
    *AT91C_PIOA_MDDR = pinmask;		/* Disable Multi-Drive */
    break;
  }

  sensors_mode[sensor] = OFF;
}

void nx_sensors_analog_enable(U32 sensor) {
  NX_ASSERT(sensor < NXT_N_SENSORS);
  NX_ASSERT(sensors_mode[sensor] == OFF);

  sensors_mode[sensor] = ANALOG;

  /* In analog mode, the DIGI outputs are driven low. */
  *AT91C_PIOA_OER = (sensors_pinmap[sensor].sda |
                     sensors_pinmap[sensor].scl);
  *AT91C_PIOA_CODR = (sensors_pinmap[sensor].sda |
                      sensors_pinmap[sensor].scl);
}

U32 nx_sensors_analog_get(U32 sensor) {
  NX_ASSERT(sensor < NXT_N_SENSORS);
  NX_ASSERT(sensors_mode[sensor] == ANALOG);

  return nx__avr_get_sensor_value(sensor);
}

U8 nx_sensors_analog_get_normalized(U32 sensor) {
  U32 raw_reading;
  U8  norm_reading;
  raw_reading = nx_sensors_analog_get(sensor);  /* range: 0-1023 (10 bits) */
  norm_reading = ((raw_reading+1)*100) / 1024;  /* range: 0-100 % */

  return norm_reading;
}

void nx_sensors_analog_digi_set(U32 sensor, nx_sensors_data_pin pin) {
  NX_ASSERT(sensor < NXT_N_SENSORS);
  NX_ASSERT(sensors_mode[sensor] == ANALOG);

  *AT91C_PIOA_SODR = (pin == DIGI1 ? sensors_pinmap[sensor].sda :
                      sensors_pinmap[sensor].scl);
}

void nx_sensors_analog_digi_clear(U32 sensor, nx_sensors_data_pin pin) {
  NX_ASSERT(sensor < NXT_N_SENSORS);
  NX_ASSERT(sensors_mode[sensor] == ANALOG);

  *AT91C_PIOA_CODR = (pin == DIGI1 ? sensors_pinmap[sensor].sda :
                      sensors_pinmap[sensor].scl);
}

void nx_sensors_analog_disable(U32 sensor) {
  NX_ASSERT(sensor < NXT_N_SENSORS);
  NX_ASSERT(sensors_mode[sensor] == ANALOG);

  nx__sensors_disable(sensor);
}
