/** @file doc.h
 *  @brief Documentation group definitions.
 *
 * This file just defines documentation modules, to keep pan-file
 * definitions in one place. That way, each file can just addtogroup.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DOC_H__
#define __NXOS_BASE_DOC_H__

/** @defgroup kernel Kernel */
/*@{*/

/** @defgroup typesAndUtils Types and utilities
 *
 * This component contains fundamental type definitions and utility
 * functions that are used all over NxOS. These are basically things
 * that a libc implementation might provide, but that we provide
 * ourselves, since we don't link with a libc.
 */

/** @defgroup kernelinternal Internals
 *
 * This component documents kernel APIs that are internal to the
 * baseplate. Application kernels may not refer to these, only kernel
 * and baseplate device driver code.
 */

/*@}*/

/** @defgroup driver Device drivers */
/*@{*/

/** @defgroup driverinternal Internal device drivers
 *
 * This component documents device drivers which are internal to the
 * Baseplate. Application kernels may not refer to these.
 *
 * This component also documents the internal APIs of drivers that are
 * public.
 */

/*@}*/


#endif /* __NXOS_BASE_DOC_H__ */
