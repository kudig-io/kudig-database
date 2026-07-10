// SPDX-License-Identifier: GPL-2.0-only
#include <linux/types.h>
#include <linux/virtio.h>
#include <linux/virtio_blk.h>
#include "../virtio/virtio_pci_common.h"
#include <linux/virtio_config.h>

/* ext feature bit definition */
#define VIRTIO_BLK_EXT_F_RING_PAIR	(1U << 0)
#define VIRTIO_BLK_EXT_F_PT_ENABLE	(1U << 1)
#define VIRTIO_BLK_EXT_F_HIDE_BLOCK     (1U << 2)
#define VIRTIO_BLK_EXT_F_INVAL		(-1)

#define VIRTIO_PCI_VSF_MAGIC_NUM			0x0
#define VIRTIO_PCI_VSF_MAGIC_NUM_VAL			0x7D4FEE9D
#define VIRTIO_PCI_HOST_VNDR_SPEC_FEATURE_SELECT	0x04
/* A 32-bit r/o bitmask of the vendor specific features supported by the host */
#define VIRTIO_PCI_HOST_VNDR_SPEC_FEATURES		0x08

#define VIRTIO_PCI_GUEST_VNDR_SPEC_FEATURE_SELECT	0x0c
/* A 32-bit r/w bitmask of the vendor specific features activated by the guest */
#define VIRTIO_PCI_GUEST_VNDR_SPEC_FEATURES		0x10


/* vsc */
#define PCI_CAP_ID_VNDR                         0x09	/* Vendor specific */
#define PCI_VSC_CFGTYPE                 0xff

/* vsec */
#define PCI_EXT_CAP_ID_VNDR                     0x0B
#define PCI_EXP_VSEC_CFGTYPE            0xff
#define VSEC_VERSION                    2

#define XVCS_MAGIC                      0x53435658
#define XVCS_VSF_KEY                    "xvcs-vsf"
#define XVCS_VERSION                    1
#define XVCS_NUM_MAX                    32U
#define XVCS_KEY_MAX			16

#define XVCS_O_MAGIC			0
#define XVCS_O_VER			4
#define XVCS_O_ADDR			12
#define XVCS_O_F_CNT			16
#define XVCS_O_CUR                      16
#define XVCS_O_NEXT                     20
#define XVCS_O_VSF                      32
static void read_xvcs(struct pci_dev *d, u32 pos,
				u32 cap_len, u32 addr, u32 num, void *data)
{
	u32 idx, where;

	for (idx = 0; idx < num; idx += 4) {
		where = addr + idx;
		pci_write_config_dword(d, pos + cap_len - 8, where);
		pci_read_config_dword(d, pos + cap_len - 4, (u32 *)((u8 *)data + idx));
	}
}

static int vcs_find_vsf_bar0_offset(struct pci_dev *dev, uint32_t cap_len,
				uint32_t pos, u32 *bar0_offset)
{
	u8 buf[XVCS_KEY_MAX+1];
	u32 where;
	u32 idx, num;
	u32 reg;

	/* check xvcs magic */
	read_xvcs(dev, pos, cap_len, XVCS_O_MAGIC, sizeof(reg), &reg);
	if (reg != XVCS_MAGIC) {
		pr_err("%s: xvcs magic 0x%x not match\n", __func__, reg);
		return -1;
	}
	/* check xvcs version */
	read_xvcs(dev, pos, cap_len, XVCS_O_VER, sizeof(reg), &reg);
	if (reg != XVCS_VERSION) {
		pr_err("%s: xvcs version 0x%x not match\n", __func__, reg);
		return -1;
	}
	/* xvcs feat block addr */
	read_xvcs(dev, pos, cap_len, XVCS_O_ADDR, sizeof(reg), &reg);
	where = reg;
	/* xvcs feat cnt */
	read_xvcs(dev, pos, cap_len, XVCS_O_F_CNT, sizeof(reg), &reg);
	num = reg;
	for (idx = 0; (idx < min(XVCS_NUM_MAX, num)) && (where > 0); idx++) {
		memset(buf, 0, sizeof(buf));

		/* self addr check */
		read_xvcs(dev, pos, cap_len,
				where + XVCS_O_CUR, sizeof(reg), &reg);
		if (reg != where)
			return -1;

		/* check key */
		read_xvcs(dev, pos, cap_len, where, XVCS_KEY_MAX, buf);

		/* found vsf */
		if (strncmp(buf, XVCS_VSF_KEY, sizeof(XVCS_VSF_KEY)) == 0) {
			read_xvcs(dev, pos, cap_len, where + XVCS_O_VSF,
					sizeof(reg), &reg);
			*bar0_offset = reg;
			return 0;
		}
		/* next vcs feat */
		read_xvcs(dev, pos, cap_len,
					where + XVCS_O_NEXT, sizeof(reg), &reg);
		where = reg;
	}
	pr_err("%s: vsf offset not found\n", __func__);
	return -1;
}

int virtblk_get_ext_feature_bar(struct virtio_device *vdev, u32 *bar_offset)
{
	struct pci_dev *dev = to_vp_device(vdev)->pci_dev;
	int cap_len, vsec = 0;
	u16 val;
	u8 type, len = 0;
	bool found = false;

	/* try to find vsc */
	for (vsec = pci_find_capability(dev, PCI_CAP_ID_VNDR);
		 vsec > 0;
		 vsec = pci_find_next_capability(dev, vsec, PCI_CAP_ID_VNDR)) {
		pci_read_config_byte(dev, vsec + offsetof(struct virtio_pci_cap, cfg_type), &type);
		if (type == PCI_VSC_CFGTYPE) {
			pci_read_config_byte(dev,
					vsec + offsetof(struct virtio_pci_cap, cap_len), &len);
			cap_len = len;
			found = true;
			break;
		}
	}

	/* try to find vsec */
	if (!found) {
		vsec = 0;
		while ((vsec = pci_find_next_ext_capability(dev, vsec,
						PCI_EXT_CAP_ID_VNDR))) {
			pci_read_config_word(dev, vsec + 0x4, &val);
			/* vsec found */
			if (val == PCI_EXP_VSEC_CFGTYPE) {
				/* get vsec cap len */
				pci_read_config_word(dev, vsec + 0x6, &val);
				if ((val & 0xF) != VSEC_VERSION)
					continue;
				cap_len = (val >> 4) & (0xFFF);
				found = true;
				break;
			}
		}
	}

	return found ? vcs_find_vsf_bar0_offset(dev, cap_len, vsec, bar_offset) : -1;
}

int virtblk_get_ext_feature(void __iomem *ioaddr, u32 *host_features)
{
	int ret;

	/* read ext bar magci number */
	ret = ioread32(ioaddr);
	if (ret != VIRTIO_PCI_VSF_MAGIC_NUM_VAL)
		return -EOPNOTSUPP;

	iowrite32(0, ioaddr + VIRTIO_PCI_HOST_VNDR_SPEC_FEATURE_SELECT);
	*host_features = ioread32(ioaddr + VIRTIO_PCI_HOST_VNDR_SPEC_FEATURES);

	return 0;
}

void virtblk_set_ext_feature(void __iomem *ioaddr, u32 guest_ext_features)
{
	iowrite32(0, ioaddr + VIRTIO_PCI_GUEST_VNDR_SPEC_FEATURE_SELECT);
	iowrite32(guest_ext_features, ioaddr + VIRTIO_PCI_GUEST_VNDR_SPEC_FEATURES);
}
