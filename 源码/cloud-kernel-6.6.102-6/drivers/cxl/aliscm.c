/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2023 Alibaba Group Holding Limited. All Rights Reserved. */
#include <linux/platform_device.h>
#include <linux/genalloc.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/acpi.h>
#include <linux/pci.h>
#include <linux/pci-doe.h>
#include <linux/mm.h>
#include "cxlmem.h"
#include "cxlpci.h"

static unsigned short aliscm_max_dev = 16;
module_param(aliscm_max_dev, ushort, 0644);
MODULE_PARM_DESC(aliscm_max_dev, "Maximum number of aliscm devices to support");

static DECLARE_RWSEM(cxl_aliscm_rwsem);

#define CXL_ALISCM_MAX_DEVS 65536

static int cxl_aliscm_major;
static DEFINE_IDA(cxl_aliscm_ida);

struct cxl_aliscm_dev {
	struct device dev;
	struct cdev cdev;
	int id;
};

struct cxl_aliscm_ctl {
	int dev_cnt;
	struct cxl_aliscm_dev *aliscm[];
};

struct cxl_aliscm_ctl *aliscm_ctl;

struct aliscm_cmd {
	void *request;
	void *response;
	size_t size_request;
	size_t size_response;
	u16 return_code;
};

static struct cxl_aliscm_dev *to_cxl_aliscm(struct device *dev)
{
	return container_of(dev, struct cxl_aliscm_dev, dev);
}

static void cxl_aliscm_release(struct device *dev)
{
	struct cxl_aliscm_dev *aliscm = to_cxl_aliscm(dev);

	ida_free(&cxl_aliscm_ida, aliscm->id);
	kfree(aliscm);
}

static char *cxl_aliscm_devnode(const struct device *dev, umode_t *mode, kuid_t *uid,
				kgid_t *gid)
{
	return kasprintf(GFP_KERNEL, "cxl/%s", dev_name(dev));
}

static struct attribute *cxl_aliscm_attributes[] = {
	NULL,
};

static struct attribute_group cxl_aliscm_attribute_group = {
	.attrs = cxl_aliscm_attributes,
};

static const struct attribute_group *cxl_aliscm_attribute_groups[] = {
	&cxl_aliscm_attribute_group,
	NULL,
};

static const struct device_type cxl_aliscm_type = {
	.name = "cxl_aliscm",
	.release = cxl_aliscm_release,
	.devnode = cxl_aliscm_devnode,
	.groups = cxl_aliscm_attribute_groups,
};

static void cxl_aliscm_unregister(void *_aliscm)
{
	struct cxl_aliscm_dev *aliscm = _aliscm;
	struct device *dev = &aliscm->dev;

	cdev_device_del(&aliscm->cdev, dev);
	put_device(dev);
}

static struct cxl_aliscm_dev *cxl_aliscm_alloc(struct device *host,
					const struct file_operations *fops)
{
	struct cxl_aliscm_dev *aliscm;
	struct device *dev;
	struct cdev *cdev;
	int rc;

	aliscm = kzalloc(sizeof(*aliscm), GFP_KERNEL);
	if (!aliscm)
		return ERR_PTR(-ENOMEM);

	rc = ida_alloc_max(&cxl_aliscm_ida, CXL_ALISCM_MAX_DEVS - 1, GFP_KERNEL);
	if (rc < 0)
		goto err;
	aliscm->id = rc;

	dev = &aliscm->dev;
	device_initialize(dev);
	dev->parent = host;
	dev->bus = &cxl_bus_type;
	dev->devt = MKDEV(cxl_aliscm_major, aliscm->id);
	dev->type = &cxl_aliscm_type;

	cdev = &aliscm->cdev;
	cdev_init(cdev, fops);
	return aliscm;

err:
	kfree(aliscm);
	return ERR_PTR(rc);
}

/*
 * AliSCM DOE interfaces
 */
#define DOE_RESPONSE_MAX_SIZE		0x100
#define ALISCM_DOE_COMMAND_FLAG_ENABLED				BIT(31)
#define ALISCM_GET_CXL_REG_COMMAND_FLAG_ENABLED		BIT(30)
#define ALISCM_DOE_ID				0x01
#define ALISCM_GET_CXL_REG_ID		0x02

struct pcie_doe_prot {
	u16 vid;
	u8 type;
	u8 rsvd0;
	/* in dw */
	u32 len : 18;
	u32 rsvd : 14;
} __packed;

/*
 * AliSCM get CXL reg interfaces
 */
#define CXL_REG_TYPE_DVSEC 0
#define   CXL_DVSEC_MAX_ID 11
#define CXL_REG_TYPE_COMPONENT 1
#define   CXL_CM_CAP_ID_RAS 0x2
#define     RAS_REG_SIZE 0x58
#define   CXL_CM_CAP_ID_HDM 0x5
#define     HDM_DECODER_HEADER_SIZE 0x10
#define     HDM_DECODER_REG_SIZE 0x20
#define     HDM_DECODER_MAX_COUNT 10
#define CXL_REG_TYPE_DEVICE 2
#define   CXLDEV_CAP_ID_DEV_STATUS 0x0001
#define     DEV_STATUS_REG_SIZE 8
#define   CXLDEV_CAP_ID_PRI_MBOX 0x0002
#define     PRI_MBOX_REG_SIZE 0x20
#define   CXLDEV_CAP_ID_MEM_DEV_STATUS 0x4000
#define	    MEM_DEV_STATUS_REG_SIZE 8

struct aliscm_get_cxl_reg_request {
	u8  ucType;	// Three types of registers are supported : DVSEC / Component / Device
	u8  ucRsvd;
	u16 uwId;
} __packed;

// @ras: CXL 2.0 8.1
const size_t dvsec_reg_size[CXL_DVSEC_MAX_ID] = {
	[CXL_DVSEC_PCIE_DEVICE] = 0x38,			// PCIe DVSEC for CXL Device
	[CXL_DVSEC_FUNCTION_MAP] = 0x2c,		// Non-CXL Function Map DVSEC
	[CXL_DVSEC_PORT_EXTENSIONS] = 0x28,		// CXL 2.0 Extensions DVSEC for Ports
	[CXL_DVSEC_PORT_GPF] = 0x10,			// GPF DVSEC for CXL Ports
	[CXL_DVSEC_DEVICE_GPF] = 0x10,			// GPF DVSEC for CXL Device
	[CXL_DVSEC_PCIE_FLEXBUS_PORT] = 0x24,	// PCIe DVSEC for Flex Bus Ports
	[CXL_DVSEC_REG_LOCATOR] = 0x24,			// Register Locator DVSEC
};

static int aliscm_cmd_ctor(struct aliscm_cmd *cmd,
			     struct cxl_aliscm_dev *aliscm, size_t request_size,
			     size_t response_size, u64 request, size_t max_response_size)
{
	struct device *dev = aliscm->dev.parent;

	*cmd = (struct aliscm_cmd) {
		.size_request = request_size,
	};

	if (request_size) {
		cmd->request = vmemdup_user(u64_to_user_ptr(request),
						request_size);
		if (IS_ERR(cmd->request)) {
			dev_err(dev, "Failed to copy cmd request from user\n");
			return PTR_ERR(cmd->request);
		}
	}

	/* Prepare to handle a full payload for variable sized output */
	if (response_size > max_response_size)
		cmd->size_response = max_response_size;
	else
		cmd->size_response = response_size;

	if (cmd->size_response) {
		cmd->response = kvzalloc(cmd->size_response, GFP_KERNEL);
		if (!cmd->response) {
			kvfree(cmd->request);
			return -ENOMEM;
		}
	}
	return 0;
}

static void aliscm_cmd_dtor(struct aliscm_cmd *cmd)
{
	kvfree(cmd->request);
	kvfree(cmd->response);
}

static int doe_validate_cmd_from_user(struct aliscm_cmd *cmd,
					  struct cxl_aliscm_dev *aliscm,
					  const struct cxl_send_command *send_cmd)
{
	if (send_cmd->rsvd != ALISCM_DOE_ID)
		return -EINVAL;

	if (!(send_cmd->flags & ALISCM_DOE_COMMAND_FLAG_ENABLED))
		return -EINVAL;

	if (send_cmd->in.size < sizeof(struct pcie_doe_prot))
		return -EINVAL;

	/* construct a aliscm_doe_cmd */
	return aliscm_cmd_ctor(cmd, aliscm,
					send_cmd->in.size, send_cmd->out.size,
					send_cmd->in.payload, DOE_RESPONSE_MAX_SIZE);
}

static int get_cxl_reg_validate_cmd_from_user(struct aliscm_cmd *cmd,
					  struct cxl_aliscm_dev *aliscm,
					  const struct cxl_send_command *send_cmd)
{
	if (send_cmd->rsvd != ALISCM_GET_CXL_REG_ID)
		return -EINVAL;

	if (!(send_cmd->flags & ALISCM_GET_CXL_REG_COMMAND_FLAG_ENABLED))
		return -EINVAL;

	if (send_cmd->in.size < sizeof(struct aliscm_get_cxl_reg_request))
		return -EINVAL;

	/* construct a aliscm_get_cxl_reg_cmd */
	return aliscm_cmd_ctor(cmd, aliscm,
					send_cmd->in.size, send_cmd->out.size,
					send_cmd->in.payload, SZ_4K);

}

static int handle_aliscm_doe_cmd_from_user(struct cxl_aliscm_dev *aliscm,
					  struct aliscm_cmd *cmd,
					  u64 response_payload, s32 *size_response,
					  u32 *retval)
{
	struct device *dev = aliscm->dev.parent;
	struct pcie_doe_prot doe_header;
	struct pci_doe_mb *doe_mb;
	void *request = cmd->request;
	int rc = 0;

	memcpy(&doe_header, request, sizeof(struct pcie_doe_prot));

	dev_dbg(dev,
		"Submitting DOE command for user\n"
		"\ttype: %x\n"
		"\tsize_request: %lu\n",
		doe_header.type,
		cmd->size_request);

	/* Find Data Object Exchange mailbox */
	doe_mb = pci_find_doe_mailbox(to_pci_dev(dev),
					  PCI_DVSEC_VENDOR_ID_CXL,
					  doe_header.type);
	if (!doe_mb) {
		dev_dbg(dev, "No DOE type %d mailbox\n", doe_header.type);
		return -ENODEV;
	}

	/* Perform Data Object Exchange */
	request += sizeof(struct pcie_doe_prot);
	rc = pci_doe(doe_mb, PCI_DVSEC_VENDOR_ID_CXL,
		     doe_header.type,
		     request, cmd->size_request - sizeof(struct pcie_doe_prot),
		     cmd->response, *size_response);
	if (rc < 0) {
		dev_dbg(dev, "DOE failed: %d", rc);
		goto out;
	}
	cmd->size_response = rc;

	if (cmd->size_response) {
		cmd->size_response = (cmd->size_response > *size_response ?
				*size_response : cmd->size_response);

		if (copy_to_user(u64_to_user_ptr(response_payload),
				 cmd->response, cmd->size_response)) {
			rc = -EFAULT;
			goto out;
		}
	}

	*size_response = cmd->size_response;
	*retval = rc;
	rc = 0;

out:
	aliscm_cmd_dtor(cmd);
	return rc;
}

static int cxl_get_dvsec_reg(struct pci_dev *pdev, u16 devsc_id, struct aliscm_cmd *cmd)
{
	int dvsec;
	int offset;
	int reg_size;

	if (devsc_id >= CXL_DVSEC_MAX_ID)
		return -EINVAL;

	dvsec = pci_find_dvsec_capability(pdev, PCI_DVSEC_VENDOR_ID_CXL, devsc_id);
	if (!dvsec) {
		dev_err(&pdev->dev, "Error, failed to find DVSEC(%d)\n", devsc_id);
		return -ENXIO;
	}

	reg_size = min(dvsec_reg_size[devsc_id], cmd->size_response);
	for (offset = 0; offset < reg_size; ++offset)
		pci_read_config_byte(pdev, dvsec + offset, cmd->response + offset);

	return reg_size;
}

static int cxl_get_component_reg(struct pci_dev *pdev, u16 type_id, struct aliscm_cmd *cmd)
{
	struct cxl_dev_state *cxlds = pci_get_drvdata(pdev);
	struct cxl_memdev *cxlmd = NULL;
	struct cxl_port *port = NULL;
	struct cxl_hdm *cxlhdm = NULL;
	void __iomem *base = NULL;
	u32 decoder_count, hdm_cap;
	int reg_size, offset;

	if (!cxlds) {
		dev_err(&pdev->dev, "Error, failed to get dev state\n");
		return -ENXIO;
	}

	switch (type_id) {
	case CXL_CM_CAP_ID_RAS:
		base = cxlds->regs.ras;
		reg_size = RAS_REG_SIZE;
		break;
	case CXL_CM_CAP_ID_HDM:
		cxlmd = cxlds->cxlmd;
		if (cxlmd)
			port = cxlmd->endpoint;
		if (port)
			cxlhdm = dev_get_drvdata(&port->dev);
		if (!cxlhdm)
			return -ENXIO;

		base = cxlhdm->regs.hdm_decoder;
		reg_size = HDM_DECODER_HEADER_SIZE;
		if (base) {
			hdm_cap = readl(base + CXL_HDM_DECODER_CAP_OFFSET);
			decoder_count = cxl_hdm_decoder_count(hdm_cap);
			if (decoder_count > HDM_DECODER_MAX_COUNT)
				decoder_count = 0;

			reg_size += HDM_DECODER_REG_SIZE * decoder_count;
		}
		break;
	default:
		return -EINVAL;
	}

	if (!base) {
		dev_err(&pdev->dev, "Error, failed to find component reg\n");
		return -ENXIO;
	}

	reg_size = min((size_t)reg_size, cmd->size_response);
	for (offset = 0; offset < reg_size; ++offset)
		*((u8 *)(cmd->response + offset)) = readb(base + offset);

	return reg_size;
}

static int cxl_get_device_reg(struct pci_dev *pdev, u16 capability_id,
					  struct aliscm_cmd *cmd)
{
	struct cxl_dev_state *cxlds = pci_get_drvdata(pdev);
	int reg_size;
	int offset;
	void __iomem *base = NULL;

	if (!cxlds) {
		dev_err(&pdev->dev, "Error, failed to get dev state\n");
		return -ENXIO;
	}

	switch (capability_id) {
	case CXLDEV_CAP_ID_DEV_STATUS:
		reg_size = DEV_STATUS_REG_SIZE;
		base = cxlds->regs.status;
		break;
	case CXLDEV_CAP_ID_PRI_MBOX:
		reg_size = PRI_MBOX_REG_SIZE;
		base = cxlds->regs.mbox;
		break;
	case CXLDEV_CAP_ID_MEM_DEV_STATUS:
		reg_size = MEM_DEV_STATUS_REG_SIZE;
		base = cxlds->regs.memdev;
		break;
	default:
		return -EINVAL;
	}

	if (!base) {
		dev_err(&pdev->dev, "Error, failed to find device reg\n");
		return -ENXIO;
	}

	reg_size = min((size_t)reg_size, cmd->size_response);
	for (offset = 0; offset < reg_size; ++offset)
		*((u8 *)(cmd->response + offset)) = readb(base + offset);

	return reg_size;
}

static int handle_aliscm_get_cxl_reg_cmd_from_user(struct cxl_aliscm_dev *aliscm,
					  struct aliscm_cmd *cmd,
					  u64 response_payload, s32 *size_response,
					  u32 *retval)
{
	struct device *dev;
	struct pci_dev *pdev;
	struct aliscm_get_cxl_reg_request get_cxl_reg_request;
	void *request;
	int rc;

	if (!aliscm || !cmd)
		return -EINVAL;

	dev = aliscm->dev.parent;
	request = cmd->request;
	pdev = to_pci_dev(dev);

	memcpy(&get_cxl_reg_request, request, sizeof(struct aliscm_get_cxl_reg_request));

	dev_dbg(dev,
		"Submitting get_cxl_reg command for user\n"
		"\ttype: %x\n"
		"\tsize_request: %lu\n",
		get_cxl_reg_request.ucType,
		cmd->size_request);

	switch (get_cxl_reg_request.ucType) {
	case CXL_REG_TYPE_DVSEC:
		rc = cxl_get_dvsec_reg(pdev, get_cxl_reg_request.uwId, cmd);
		if (rc < 0) {
			dev_dbg(dev, "Get DVSEC reg failed: %d", rc);
			goto out;
		}
		break;
	case CXL_REG_TYPE_COMPONENT:
		rc = cxl_get_component_reg(pdev, get_cxl_reg_request.uwId, cmd);
		if (rc < 0) {
			dev_dbg(dev, "Get component reg failed: %d", rc);
			goto out;
		}
		break;
	case CXL_REG_TYPE_DEVICE:
		rc = cxl_get_device_reg(pdev, get_cxl_reg_request.uwId, cmd);
		if (rc < 0) {
			dev_dbg(dev, "Get device reg failed: %d", rc);
			goto out;
		}
		break;
	default:
		rc = -EINVAL;
		goto out;
	}

	cmd->size_response = rc;
	if (cmd->size_response > 0) {
		if (copy_to_user(u64_to_user_ptr(response_payload),
				 cmd->response, cmd->size_response)) {
			rc = -EFAULT;
			goto out;
		}
	}

	*size_response = cmd->size_response;
	*retval = rc;
	rc = 0;
out:
	return rc;
}

int cxl_aliscm_send_cmd(struct cxl_aliscm_dev *aliscm, struct cxl_send_command __user *s)
{
	struct device *dev = &aliscm->dev;
	struct cxl_send_command send;
	struct aliscm_cmd cmd;
	int rc;

	dev_dbg(dev, "Send IOCTL to AliSCM\n");

	if (copy_from_user(&send, s, sizeof(send)))
		return -EFAULT;

	if (send.rsvd == ALISCM_DOE_ID) {
		rc = doe_validate_cmd_from_user(&cmd, aliscm, &send);
		if (rc)
			return rc;

		rc = handle_aliscm_doe_cmd_from_user(aliscm, &cmd, send.out.payload,
							&send.out.size, &send.retval);
		if (rc)
			return rc;
	} else if (send.rsvd == ALISCM_GET_CXL_REG_ID) {
		rc = get_cxl_reg_validate_cmd_from_user(&cmd, aliscm, &send);
		if (rc)
			return rc;

		rc = handle_aliscm_get_cxl_reg_cmd_from_user(aliscm, &cmd, send.out.payload,
							&send.out.size, &send.retval);
		if (rc)
			return rc;
	} else {
		dev_warn(dev, "Unsupported command type: %x\n", send.rsvd);
		return -EINVAL;
	}

	if (copy_to_user(s, &send, sizeof(send)))
		return -EFAULT;

	return 0;
}

static long __cxl_aliscm_ioctl(struct cxl_aliscm_dev *aliscm, unsigned int cmd,
				   unsigned long arg)
{
	switch (cmd) {
	case CXL_MEM_SEND_COMMAND:
		return cxl_aliscm_send_cmd(aliscm, (void __user *)arg);
	default:
		return -ENOTTY;
	}
}

static long cxl_aliscm_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	struct cxl_aliscm_dev *aliscm = file->private_data;
	int rc = -ENXIO;

	down_read(&cxl_aliscm_rwsem);
	if (aliscm)
		rc = __cxl_aliscm_ioctl(aliscm, cmd, arg);
	up_read(&cxl_aliscm_rwsem);

	return rc;
}

static int cxl_aliscm_open(struct inode *inode, struct file *file)
{
	struct cxl_aliscm_dev *aliscm =
		container_of(inode->i_cdev, typeof(*aliscm), cdev);

	get_device(&aliscm->dev);
	file->private_data = aliscm;

	return 0;
}

static int cxl_aliscm_release_file(struct inode *inode, struct file *file)
{
	struct cxl_aliscm_dev *aliscm =
		container_of(inode->i_cdev, typeof(*aliscm), cdev);

	put_device(&aliscm->dev);

	return 0;
}

static const struct file_operations cxl_aliscm_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = cxl_aliscm_ioctl,
	.open = cxl_aliscm_open,
	.release = cxl_aliscm_release_file,
};

struct cxl_aliscm_dev *devm_cxl_add_aliscm(struct device *host)
{
	struct cxl_aliscm_dev *aliscm;
	struct device *dev;
	struct cdev *cdev;
	int rc;

	aliscm = cxl_aliscm_alloc(host, &cxl_aliscm_fops);
	if (IS_ERR(aliscm)) {
		dev_warn(host, "cxl_aliscm_alloc failed\n");
		return NULL;
	}

	dev = &aliscm->dev;
	rc = dev_set_name(dev, "aliscm%d", aliscm->id);
	if (rc)
		goto err;

	cdev = &aliscm->cdev;
	rc = cdev_device_add(cdev, dev);
	if (rc) {
		dev_dbg(host, "cdev_device_add failed\n");
		goto err;
	}

	return aliscm;

err:
	put_device(dev);
	return NULL;
}

static int cxl_mem_enumerate(struct device *dev, void *data)
{
	struct cxl_aliscm_dev *aliscm;
	struct pci_dev *pdev;

	if (!dev_is_pci(dev))
		return 0;

	pdev = to_pci_dev(dev);
	if (pdev->class != (PCI_CLASS_MEMORY_CXL << 8 | CXL_MEMORY_PROGIF))
		return 0;

	if (pdev->vendor != PCI_VENDOR_ID_ALISCM || pdev->device != 0x0ddb)
		return 0;

	if (aliscm_ctl->dev_cnt >= aliscm_max_dev) {
		dev_warn(dev, "Maximum number of aliscm devices reached\n");
		return 0;
	}

	aliscm = devm_cxl_add_aliscm(dev);
	if (!aliscm)
		return -ENOMEM;

	aliscm_ctl->aliscm[aliscm_ctl->dev_cnt++] = aliscm;

	return 0;
}

static __init int cxl_aliscm_init(void)
{
	dev_t devt;
	size_t size_ctl;
	int rc;

	rc = alloc_chrdev_region(&devt, 0, CXL_ALISCM_MAX_DEVS, "cxl_extend");
	if (rc)
		return rc;

	cxl_aliscm_major = MAJOR(devt);

	size_ctl = sizeof(struct cxl_aliscm_ctl) + sizeof(struct cxl_aliscm_dev *) * aliscm_max_dev;
	aliscm_ctl = kzalloc(size_ctl, GFP_KERNEL);
	if (!aliscm_ctl)
		return -ENOMEM;

	/* scan for AliSCM devices */
	bus_for_each_dev(&pci_bus_type, NULL, NULL, cxl_mem_enumerate);

	return 0;
}

static __exit void cxl_aliscm_exit(void)
{
	int i = 0;

	for (i = 0; i < aliscm_ctl->dev_cnt; i++) {
		struct cxl_aliscm_dev *aliscm = aliscm_ctl->aliscm[i];

		if (aliscm)
			cxl_aliscm_unregister(aliscm);
	}
	kfree(aliscm_ctl);

	unregister_chrdev_region(MKDEV(cxl_aliscm_major, 0), CXL_ALISCM_MAX_DEVS);
}

module_init(cxl_aliscm_init);
module_exit(cxl_aliscm_exit);

MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(CXL);
