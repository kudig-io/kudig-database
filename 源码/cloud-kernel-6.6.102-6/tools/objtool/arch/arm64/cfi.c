// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015-2017 Josh Poimboeuf <jpoimboe@redhat.com>
 */

#include <stdlib.h>
#include <sys/mman.h>

#include <linux/objtool_types.h>
#include <objtool/cfi.h>
#include <arch/cfi.h>
#include <objtool/builtin.h>
#include <objtool/warn.h>

unsigned long nr_cfi, nr_cfi_reused, nr_cfi_cache;

struct cfi_init_state initial_func_cfi;
struct cfi_state init_cfi;
struct cfi_state func_cfi;
struct cfi_state force_undefined_cfi;

void init_cfi_state(struct cfi_state *cfi)
{
	int i;

	for (i = 0; i < CFI_NUM_REGS; i++) {
		cfi->regs[i].base = CFI_UNDEFINED;
		cfi->vals[i].base = CFI_UNDEFINED;
	}
	cfi->cfa.base = CFI_UNDEFINED;
	cfi->drap_reg = CFI_UNDEFINED;
	cfi->drap_offset = -1;
}

static struct cfi_state *cfi_alloc(void)
{
	struct cfi_state *cfi = calloc(sizeof(struct cfi_state), 1);

	if (!cfi) {
		WARN("calloc failed");
		exit(1);
	}
	nr_cfi++;
	return cfi;
}

static int cfi_bits;
static struct hlist_head *cfi_hash;

inline bool cficmp(struct cfi_state *cfi1, struct cfi_state *cfi2)
{
	return memcmp((void *)cfi1 + sizeof(cfi1->hash),
		      (void *)cfi2 + sizeof(cfi2->hash),
		      sizeof(struct cfi_state) - sizeof(struct hlist_node));
}

static inline u32 cfi_key(struct cfi_state *cfi)
{
	return jhash((void *)cfi + sizeof(cfi->hash),
		     sizeof(*cfi) - sizeof(cfi->hash), 0);
}

struct cfi_state *cfi_hash_find_or_add(struct cfi_state *cfi)
{
	struct hlist_head *head = &cfi_hash[hash_min(cfi_key(cfi), cfi_bits)];
	struct cfi_state *obj;

	hlist_for_each_entry(obj, head, hash) {
		if (!cficmp(cfi, obj)) {
			nr_cfi_cache++;
			return obj;
		}
	}

	obj = cfi_alloc();
	*obj = *cfi;
	hlist_add_head(&obj->hash, head);

	return obj;
}

void cfi_hash_add(struct cfi_state *cfi)
{
	struct hlist_head *head = &cfi_hash[hash_min(cfi_key(cfi), cfi_bits)];

	hlist_add_head(&cfi->hash, head);
}

void *cfi_hash_alloc(unsigned long size)
{
	cfi_bits = max(10, ilog2(size));
	cfi_hash = mmap(NULL, sizeof(struct hlist_head) << cfi_bits,
			PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANON, -1, 0);
	if (cfi_hash == (void *)-1L) {
		WARN("mmap fail cfi_hash");
		cfi_hash = NULL;
	}  else if (opts.stats) {
		printf("cfi_bits: %d\n", cfi_bits);
	}

	return cfi_hash;
}

void set_func_state(struct cfi_state *state)
{
	state->cfa = initial_func_cfi.cfa;
	memcpy(&state->regs, &initial_func_cfi.regs,
	       CFI_NUM_REGS * sizeof(struct cfi_reg));
	state->stack_size = initial_func_cfi.cfa.offset;
	state->type = UNWIND_HINT_TYPE_CALL;
}
