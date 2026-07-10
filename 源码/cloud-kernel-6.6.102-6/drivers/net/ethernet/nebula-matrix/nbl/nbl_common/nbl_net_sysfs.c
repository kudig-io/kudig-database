// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_common.h"

#ifndef NO_SYSFS_PREALLOC
#define NBL_SET_RO_ATTR(rep_attr, attr_name, attr_show) do {				\
	typeof(rep_attr) _rep_attr = (rep_attr);					\
	(_rep_attr)->attr.name = __stringify(attr_name);				\
	(_rep_attr)->attr.mode = SYSFS_PREALLOC | VERIFY_OCTAL_PERMISSIONS(0444);	\
	(_rep_attr)->show = attr_show;							\
	(_rep_attr)->store = NULL;							\
} while (0)
#else
#define NBL_SET_RO_ATTR(rep_attr, attr_name, attr_show) do {				\
	typeof(rep_attr) _rep_attr = (rep_attr);					\
	(_rep_attr)->attr.name = __stringify(attr_name);				\
	(_rep_attr)->attr.mode = 0444;							\
	(_rep_attr)->show = attr_show;							\
	(_rep_attr)->store = NULL;							\
} while (0)
#endif

#define NBL_NET_REP_ID_LEN 4

static ssize_t net_rep_show(struct device *dev,
			    struct nbl_netdev_rep_attr *attr, char *buf)
{
	return scnprintf(buf, NBL_NET_REP_ID_LEN, "%d\n", attr->rep_id);
}

void nbl_net_addr_rep_attr(struct nbl_netdev_rep_attr *attr, int rep_id)
{
	NBL_SET_RO_ATTR(attr, rep_id, net_rep_show);
	attr->rep_id = rep_id;
}
