/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/core.h"
#include "base/assert.h"
#include "marvin/_scheduler.h"

void mv_time_sleep(U32 ms) {
  /* TODO: Decide if this should be a graceful return. */
  NX_ASSERT(ms > 0);

  mv__scheduler_task_suspend(mv_scheduler_get_current_task(), ms);
}
