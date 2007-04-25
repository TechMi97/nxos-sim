#ifndef __NXTOS_MOTORS_H__
#define __NXTOS_MOTORS_H__

#include "mytypes.h"

void motors_init();
void motors_stop(U8 motor, bool brake);
void motors_rotate(U8 motor, S8 speed);
void motors_rotate_angle(U8 motor, S8 speed, U32 angle, bool brake);
void motors_rotate_time(U8 motor, S8 speed, U32 ms, bool brake);
U32 motors_get_tach_count(U8 motor);

#endif
