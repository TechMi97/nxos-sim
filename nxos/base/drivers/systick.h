#ifndef __NXOS_SYSTICK_H__
#define __NXOS_SYSTICK_H__

#include "base/mytypes.h"

void systick_init();
U32 systick_get_ms();
void systick_wait_ms(U32 ms);
void systick_wait_ns(U32 ns);
void systick_install_scheduler(closure_t scheduler_cb);

#endif
