/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/* Copyright (C) 2015-2017 Netronome Systems, Inc. */

/*
 * nfp6000_pcie.h
 * Author: Jason McMullan <jason.mcmullan@netronome.com>
 */

#ifndef NFP6000_PCIE_H
#define NFP6000_PCIE_H

#include "nfp_cpp.h"

/* Vendor specific register layout */
#define NFP_VNDR_HEADER_OFFSET	0x0
#define NFP_VNDR_PF_ID_OFFSET	0x4

struct nfp_pf;

struct nfp_cpp *
nfp_cpp_from_nfp6000_pcie(struct pci_dev *pdev, const struct nfp_dev_info *dev_info,
			  struct nfp_pf *pf);

#endif /* NFP6000_PCIE_H */
