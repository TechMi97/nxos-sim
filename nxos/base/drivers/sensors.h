#ifndef __NXOS_SENSORS_H__
#define __NXOS_SENSORS_H__

#include "base/mytypes.h"

#define NXT_N_SENSORS 4

typedef enum {
  DIGI0 = 0,
  DIGI1 = 1,
} sensor_data_pin;

void nx_sensors_init();

void nx_sensors_analog_enable(U8 sensor);
void nx_sensors_analog_digi_set(U8 sensor, sensor_data_pin pin);
void nx_sensors_analog_digi_clear(U8 sensor, sensor_data_pin pin);
U32 nx_sensors_analog_get(U8 sensor);

void nx_sensors_disable(U8 sensor);

#endif
