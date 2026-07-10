/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2015-2017 Josh Poimboeuf <jpoimboe@redhat.com>
 */

#ifndef _OBJTOOL_ARCH_CFI_H
#define _OBJTOOL_ARCH_CFI_H

#include <arch/cfi_regs.h>

#include <objtool/cfi.h>

void init_cfi_state(struct cfi_state *cfi);
bool cficmp(struct cfi_state *cfi1, struct cfi_state *cfi2);
struct cfi_state *cfi_hash_find_or_add(struct cfi_state *cfi);
void cfi_hash_add(struct cfi_state *cfi);
void *cfi_hash_alloc(unsigned long size);
void set_func_state(struct cfi_state *state);

extern unsigned long nr_cfi, nr_cfi_reused, nr_cfi_cache;
extern struct cfi_init_state initial_func_cfi;
extern struct cfi_state init_cfi;
extern struct cfi_state func_cfi;
extern struct cfi_state force_undefined_cfi;

#endif /* _OBJTOOL_ARCH_CFI_H */
