/** @file vm_op_arithmetic_logic.h
 *  @brief Implementation of arithmetic and logic operations.
 */

/* Copyright (c) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_SYSTEMS_LEGOVM_VM_OP_ARITHMETIC_LOGIC_H__
#define __NXOS_SYSTEMS_LEGOVM_VM_OP_ARITHMETIC_LOGIC_H__

#include "base/types.h"

void lego_vm_op_add(void);
void lego_vm_op_sub(void);
void lego_vm_op_neg(void);
void lego_vm_op_mul(void);
void lego_vm_op_div(void);
void lego_vm_op_mod(void);

void lego_vm_op_and(void);
void lego_vm_op_or(void);
void lego_vm_op_xor(void);
void lego_vm_op_not(void);

#endif /* __NXOS_SYSTEMS_LEGOVM_VM_OP_ARITHMETIC_LOGIC_H__ */
