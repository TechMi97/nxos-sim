#ifndef __NXOS_SENSORS_H__
#define __NXOS_SENSORS_H__

#include "mytypes.h"

#define NXT_N_SENSORS 4

typedef enum {
  DIGI0 = 0,
  DIGI1 = 1,
} sensor_data_pin;

/* Each sensor port has two DIGI pins, whose use varies from sensor
 * to sensor. We remember which two pins each sensor has using the
 * sensor_pins structure.
 */
typedef struct {
  U32 scl; /* DIGI0 - I2C clock. */
  U32 sda; /* DIGI1 - I2C data. */
} sensor_pins;

void sensors_init();

sensor_pins sensors_get_pins(U8 sensor);

void sensors_analog_digi_set(U8 sensor, sensor_data_pin pin);
void sensors_analog_digi_clear(U8 sensor, sensor_data_pin pin);
U32 sensors_analog_get(U8 sensor);

void sensors_analog_enable(U8 sensor);
void sensors_i2c_enable(U8 sensor);

void sensors_disable(U8 sensor);

#endif
