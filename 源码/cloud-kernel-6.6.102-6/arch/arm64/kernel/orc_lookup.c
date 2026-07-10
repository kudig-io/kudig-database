// SPDX-License-Identifier: GPL-2.0-only
#include <linux/objtool.h>
#include <linux/module.h>
#include <linux/sort.h>
#include <asm/orc_types.h>
#include <asm/orc_lookup.h>

bool orc_init __ro_after_init;
static unsigned int lookup_num_blocks __ro_after_init;

static inline unsigned long orc_ip(const int *ip)
{
	return (unsigned long)ip + *ip;
}

static struct orc_entry *__orc_find(int *ip_table, struct orc_entry *u_table,
				    unsigned int num_entries, unsigned long ip)
{
	int *first = ip_table;
	int *last = ip_table + num_entries - 1;
	int *mid = first, *found = first;

	if (!num_entries)
		return NULL;

	/*
	 * Do a binary range search to find the rightmost duplicate of a given
	 * starting address.  Some entries are section terminators which are
	 * "weak" entries for ensuring there are no gaps.  They should be
	 * ignored when they conflict with a real entry.
	 */
	while (first <= last) {
		mid = first + ((last - first) / 2);

		if (orc_ip(mid) <= ip) {
			found = mid;
			first = mid + 1;
		} else
			last = mid - 1;
	}

	return u_table + (found - ip_table);
}

#ifdef CONFIG_MODULES
static struct orc_entry *orc_module_find(unsigned long ip)
{
	struct module *mod;

	mod = __module_address(ip);
	if (!mod || !mod->arch.orc_unwind || !mod->arch.orc_unwind_ip)
		return NULL;
	return __orc_find(mod->arch.orc_unwind_ip, mod->arch.orc_unwind,
			  mod->arch.num_orcs, ip);
}
#else
static struct orc_entry *orc_module_find(unsigned long ip)
{
	return NULL;
}
#endif

/*
 * If we crash with IP==0, the last successfully executed instruction
 * was probably an indirect function call with a NULL function pointer,
 * and we don't have unwind information for NULL.
 * This hardcoded ORC entry for IP==0 allows us to unwind from a NULL function
 * pointer into its parent and then continue normally from there.
 */
static struct orc_entry null_orc_entry = {
	.sp_offset = sizeof(long),
	.sp_reg = ORC_REG_SP,
	.fp_reg = ORC_REG_UNDEFINED,
	.type = ORC_TYPE_CALL
};

struct orc_entry *orc_find(unsigned long ip)
{
	static struct orc_entry *orc;

	if (ip == 0)
		return &null_orc_entry;

	/* For non-init vmlinux addresses, use the fast lookup table: */
	if (ip >= LOOKUP_START_IP && ip < LOOKUP_STOP_IP) {
		unsigned int idx, start, stop;

		idx = (ip - LOOKUP_START_IP) / LOOKUP_BLOCK_SIZE;

		if (unlikely((idx >= lookup_num_blocks-1))) {
			orc_warn("WARNING: bad lookup idx: idx=%u num=%u ip=%pB\n",
				 idx, lookup_num_blocks, (void *)ip);
			return NULL;
		}

		start = orc_lookup[idx];
		stop = orc_lookup[idx + 1] + 1;

		if (unlikely((__start_orc_unwind + start >= __stop_orc_unwind) ||
			     (__start_orc_unwind + stop > __stop_orc_unwind))) {
			orc_warn("WARNING: bad lookup value: idx=%u num=%u start=%u stop=%u ip=%pB\n",
				 idx, lookup_num_blocks, start, stop, (void *)ip);
			return NULL;
		}

		return __orc_find(__start_orc_unwind_ip + start,
				  __start_orc_unwind + start, stop - start, ip);
	}

	/* vmlinux .init slow lookup: */
	if (is_kernel_inittext(ip))
		return __orc_find(__start_orc_unwind_ip, __start_orc_unwind,
				  __stop_orc_unwind_ip - __start_orc_unwind_ip, ip);

	/* Module lookup: */
	orc = orc_module_find(ip);
	if (orc)
		return orc;

	return arch_orc_find(ip);
}

#ifdef CONFIG_MODULES

static DEFINE_MUTEX(sort_mutex);
static int *cur_orc_ip_table = __start_orc_unwind_ip;
static struct orc_entry *cur_orc_table = __start_orc_unwind;

static void orc_sort_swap(void *_a, void *_b, int size)
{
	struct orc_entry *orc_a, *orc_b;
	int *a = _a, *b = _b, tmp;
	int delta = _b - _a;

	/* Swap the .orc_unwind_ip entries: */
	tmp = *a;
	*a = *b + delta;
	*b = tmp - delta;

	/* Swap the corresponding .orc_unwind entries: */
	orc_a = cur_orc_table + (a - cur_orc_ip_table);
	orc_b = cur_orc_table + (b - cur_orc_ip_table);
	swap(*orc_a, *orc_b);
}

static int orc_sort_cmp(const void *_a, const void *_b)
{
	struct orc_entry *orc_a;
	const int *a = _a, *b = _b;
	unsigned long a_val = orc_ip(a);
	unsigned long b_val = orc_ip(b);

	if (a_val > b_val)
		return 1;
	if (a_val < b_val)
		return -1;

	/*
	 * The "weak" section terminator entries need to always be first
	 * to ensure the lookup code skips them in favor of real entries.
	 * These terminator entries exist to handle any gaps created by
	 * whitelisted .o files which didn't get objtool generation.
	 */
	orc_a = cur_orc_table + (a - cur_orc_ip_table);
	return orc_a->type == ORC_TYPE_UNDEFINED ? -1 : 1;
}

void orc_lookup_module_init(struct module *mod,
			    void *_orc_ip, size_t orc_ip_size,
			    void *_orc, size_t orc_size)
{
	int *orc_ip = _orc_ip;
	struct orc_entry *orc = _orc;
	unsigned int num_entries = orc_ip_size / sizeof(int);

	WARN_ON_ONCE(orc_ip_size % sizeof(int) != 0 ||
		     orc_size % sizeof(*orc) != 0 ||
		     num_entries != orc_size / sizeof(*orc));

	/*
	 * The 'cur_orc_*' globals allow the orc_sort_swap() callback to
	 * associate an .orc_unwind_ip table entry with its corresponding
	 * .orc_unwind entry so they can both be swapped.
	 */
	mutex_lock(&sort_mutex);
	cur_orc_ip_table = orc_ip;
	cur_orc_table = orc;
	sort(orc_ip, num_entries, sizeof(int), orc_sort_cmp, orc_sort_swap);
	mutex_unlock(&sort_mutex);

	mod->arch.orc_unwind_ip = orc_ip;
	mod->arch.orc_unwind = orc;
	mod->arch.num_orcs = num_entries;
}
#endif

void __init orc_lookup_init(void)
{
	size_t orc_ip_size = (void *)__stop_orc_unwind_ip - (void *)__start_orc_unwind_ip;
	size_t orc_size = (void *)__stop_orc_unwind - (void *)__start_orc_unwind;
	size_t num_entries = orc_ip_size / sizeof(int);
	struct orc_entry *orc;
	int i;

	if (!num_entries || orc_ip_size % sizeof(int) != 0 ||
	    orc_size % sizeof(struct orc_entry) != 0 ||
	    num_entries != orc_size / sizeof(struct orc_entry)) {
		orc_warn("WARNING: Bad or missing .orc_unwind table.  Disabling unwinder.\n");
		return;
	}

	/*
	 * Note, the orc_unwind and orc_unwind_ip tables were already
	 * sorted at build time via the 'sorttable' tool.
	 * It's ready for binary search straight away, no need to sort it.
	 */

	/* Initialize the fast lookup table: */
	lookup_num_blocks = orc_lookup_end - orc_lookup;
	for (i = 0; i < lookup_num_blocks-1; i++) {
		orc = __orc_find(__start_orc_unwind_ip, __start_orc_unwind,
				 num_entries,
				 LOOKUP_START_IP + (LOOKUP_BLOCK_SIZE * i));
		if (!orc) {
			orc_warn("WARNING: Corrupt .orc_unwind table.  Disabling unwinder.\n");
			return;
		}

		orc_lookup[i] = orc - __start_orc_unwind;
	}

	/* Initialize the ending block: */
	orc = __orc_find(__start_orc_unwind_ip, __start_orc_unwind, num_entries,
			 LOOKUP_STOP_IP);
	if (!orc) {
		orc_warn("WARNING: Corrupt .orc_unwind table.  Disabling unwinder.\n");
		return;
	}
	orc_lookup[lookup_num_blocks-1] = orc - __start_orc_unwind;

	orc_init = true;
}

__weak struct orc_entry *arch_orc_find(unsigned long ip)
{
	return NULL;
}
