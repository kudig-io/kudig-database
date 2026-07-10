// SPDX-License-Identifier: GPL-2.0-only
/*
 * Userspace interface for CSV guest driver
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: fangbaoshun <fangbaoshun@hygon.cn>
 */
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/cc_platform.h>
#include <linux/cacheflush.h>
#include <linux/psp-hygon.h>

#include <uapi/linux/kvm_para.h>

#include <asm/csv.h>

#include "csv-guest.h"

/* Mutex to serialize the command handling. */
static DEFINE_MUTEX(csv_cmd_mutex);

/* The magic string is used to identify extended attestation aware requests. */
static char csv_attestation_magic[CSV_ATTESTATION_MAGIC_LEN] = CSV_ATTESTATION_MAGIC_STRING;

static int csv_get_report(unsigned long arg)
{
	u8	*csv_report;
	long	ret;
	struct	csv_report_req req;

	if (copy_from_user(&req, (void __user *)arg,
			   sizeof(struct csv_report_req)))
		return -EFAULT;

	if (req.len < CSV_REPORT_INPUT_DATA_LEN || !req.report_data)
		return -EINVAL;

	csv_report = kzalloc(req.len, GFP_KERNEL);
	if (!csv_report) {
		ret = -ENOMEM;
		goto out;
	}

	/* Save user input data */
	if (copy_from_user(csv_report, req.report_data, CSV_REPORT_INPUT_DATA_LEN)) {
		ret = -EFAULT;
		goto out;
	}

	/* Generate CSV_REPORT using "KVM_HC_VM_ATTESTATION" VMMCALL */
	ret = kvm_hypercall2(KVM_HC_VM_ATTESTATION, __pa(csv_report), req.len);
	if (ret)
		goto out;

	if (copy_to_user(req.report_data, csv_report, req.len))
		ret = -EFAULT;

out:
	kfree(csv_report);
	return ret;
}

static int csv3_get_report(unsigned long arg)
{
	struct csv_report_req input;
	struct page *page = NULL;
	struct csv3_data_attestation_report *cmd_buff = NULL;
	void *req_buff = NULL;
	void *resp_buff = NULL;
	struct csv_guest_user_data_attestation_ext *udata = NULL;
	int ret;

	if (copy_from_user(&input, (void __user *)arg, sizeof(input)))
		return -EFAULT;

	if (!input.len || !input.report_data)
		return -EINVAL;

	/* Use alloc_page for alignment */
	page = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
	if (!page)
		return -ENOMEM;
	cmd_buff = (struct csv3_data_attestation_report *)page_address(page);

	/*
	 * If user space issues an extended attestation aware request, then sync
	 * the flags to @cmd_buff.
	 */
	if (input.len >= CSV_ATTESTATION_USER_DATA_EXT_LEN) {
		udata = kzalloc(CSV_ATTESTATION_USER_DATA_EXT_LEN, GFP_KERNEL);
		if (!udata) {
			ret = -ENOMEM;
			goto err;
		}

		if (copy_from_user((void *)udata, input.report_data,
				   CSV_ATTESTATION_USER_DATA_EXT_LEN)) {
			ret = -EFAULT;
			goto err;
		}

		if (!strncmp((char *)udata->magic, csv_attestation_magic,
						CSV_ATTESTATION_MAGIC_LEN))
			cmd_buff->flags = udata->flags;
	}

	/*
	 * Query the firmware to get minimum length of request buffer and
	 * respond buffer.
	 */
	ret = csv3_issue_request_report(__pa(cmd_buff), sizeof(*cmd_buff));

	/*
	 * The input.len must be the maxinum length of the req and resp buffer
	 * at least, otherwise return with error.
	 */
	if (input.len < max(cmd_buff->req_len, cmd_buff->resp_len)) {
		ret = -EINVAL;
		goto err;
	}

	/* Use alloc_page for alignment */
	page = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
	if (!page) {
		ret = -ENOMEM;
		goto err;
	}
	req_buff = page_address(page);

	/* Use alloc_page for alignment */
	page = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
	if (!page) {
		ret = -ENOMEM;
		goto err;
	}
	resp_buff = page_address(page);

	/* Copy user's input data */
	if (copy_from_user(req_buff, input.report_data, cmd_buff->req_len)) {
		ret = -EFAULT;
		goto err;
	}

	/*
	 * The req_len and resp_len fields has already been filled by firmware
	 * when we query the lengths from firmware.
	 */
	cmd_buff->req_gpa  = __pa(req_buff);
	cmd_buff->resp_gpa = __pa(resp_buff);

	ret = csv3_issue_request_report(__pa(cmd_buff), sizeof(*cmd_buff));
	if (ret || (!ret && cmd_buff->fw_error_code)) {
		pr_err("%s: fail to generate report, fw_error:%#x ret:%d\n",
			__func__, cmd_buff->fw_error_code, ret);
		ret = -EIO;
		goto err;
	}

	/* Copy attestation report to user */
	if (copy_to_user(input.report_data, resp_buff, cmd_buff->resp_len))
		ret = -EFAULT;

err:
	if (resp_buff)
		free_page((unsigned long)resp_buff);
	if (req_buff)
		free_page((unsigned long)req_buff);
	if (cmd_buff)
		free_page((unsigned long)cmd_buff);

	kfree(udata);

	return ret;
}

static int get_report(unsigned long arg)
{
	int ret = -ENOTTY;

	lockdep_assert_held(&csv_cmd_mutex);

	if (csv3_active())
		ret = csv3_get_report(arg);
	else if (cc_platform_has(CC_ATTR_GUEST_MEM_ENCRYPT))
		ret = csv_get_report(arg);
	return ret;
}

static int csv3_rtmr_status(struct csv_rtmr_req *req)
{
	int ret = 0;
	struct csv_guest_user_rtmr_status *status = NULL;
	struct csv_rtmr_subcmd_status *subcmd = NULL;
	struct csv_rtmr_subcmd_hdr *hdr = NULL;

	if (req->len < sizeof(*status)) {
		req->len = sizeof(*status);
		return -EINVAL;
	}

	status = kzalloc(req->len, GFP_KERNEL);
	if (!status)
		return -ENOMEM;

	/* Copy user request params to the kernel space. */
	if (copy_from_user(status, (void __user *)req->buf, req->len)) {
		ret = -EFAULT;
		goto err_free_status;
	}

	subcmd = kzalloc(sizeof(*subcmd), GFP_KERNEL);
	if (!subcmd) {
		ret = -ENOMEM;
		goto err_free_status;
	}

	hdr = &subcmd->hdr;
	hdr->subcmd_id = CSV3_SECURE_CMD_RTMR_STATUS;
	hdr->subcmd_size = sizeof(*subcmd);

	ret = csv3_issue_request_rtmr((void *)subcmd, sizeof(*subcmd));
	if (ret) /* -ENODEV */
		goto err_free_subcmd;

	status->version = subcmd->version;
	status->state = subcmd->state;

	req->fw_error_code = hdr->fw_error_code;

	/* Copy the response to user space. */
	if (copy_to_user((void __user *)req->buf, status, req->len))
		ret = -EFAULT;

err_free_subcmd:
	kfree(subcmd);
err_free_status:
	kfree(status);

	return ret;
}

static int csv3_rtmr_start(struct csv_rtmr_req *req)
{
	int ret = 0;
	struct csv_guest_user_rtmr_start *start = NULL;
	struct csv_rtmr_subcmd_start *subcmd = NULL;
	struct csv_rtmr_subcmd_hdr *hdr = NULL;

	if (req->len < sizeof(*start)) {
		req->len = sizeof(*start);
		return -EINVAL;
	}

	start = kzalloc(req->len, GFP_KERNEL);
	if (!start)
		return -ENOMEM;

	/* Copy user request params to the kernel space. */
	if (copy_from_user(start, (void __user *)req->buf, req->len)) {
		ret = -EFAULT;
		goto err_free_start;
	}

	/*
	 * If the request version of RTMR is invalid, return the maximum
	 * version support in the guest kernel.
	 */
	if (start->version > CSV_RTMR_VERSION_MAX ||
	    start->version < CSV_RTMR_VERSION_MIN) {
		ret = -EINVAL;
		start->version = CSV_RTMR_VERSION_MAX;
		goto err_copy_back;
	}

	subcmd = kzalloc(sizeof(*subcmd), GFP_KERNEL);
	if (!subcmd) {
		ret = -ENOMEM;
		goto err_free_start;
	}

	hdr = &subcmd->hdr;
	hdr->subcmd_id = CSV3_SECURE_CMD_RTMR_START;
	hdr->subcmd_size = sizeof(*subcmd);

	subcmd->version = start->version;

	ret = csv3_issue_request_rtmr((void *)subcmd, sizeof(*subcmd));
	if (ret) /* -ENODEV */
		goto err_free_subcmd;

	req->fw_error_code = hdr->fw_error_code;

err_copy_back:
	/* Copy the response to user space. */
	if (copy_to_user((void __user *)req->buf, start, req->len))
		ret = -EFAULT;

err_free_subcmd:
	kfree(subcmd);
err_free_start:
	kfree(start);

	return ret;
}

static int csv3_rtmr_read(struct csv_rtmr_req *req)
{
	int ret = 0, i, cnt;
	struct csv_guest_user_rtmr_read *read = NULL;
	struct csv_rtmr_subcmd_read *subcmd = NULL;
	struct csv_rtmr_subcmd_hdr *hdr = NULL;
	uint32_t subcmd_len;

	if (req->len < sizeof(*read)) {
		req->len = sizeof(*read);
		return -EINVAL;
	}

	/* The uapi must provide correct size of RTMR register. */
	if (sizeof(read->data) != sizeof(subcmd->data)) {
		pr_err("error: CSV RTMR size confusion\n");
		return -EINVAL;
	}

	read = kzalloc(req->len, GFP_KERNEL);
	if (!read)
		return -ENOMEM;

	/* Copy user request params to the kernel space. */
	if (copy_from_user(read, (void __user *)req->buf, req->len)) {
		ret = -EFAULT;
		goto err_free_read;
	}

	/*
	 * Check if the bitmap and the corresponding storage buffer are
	 * valid.
	 *
	 * The @len field in struct csv_rtmr_req indicates the size of the
	 * @storage buffer@ (located starting from the @data field of struct
	 * csv_guest_user_rtmr_read). The size of the @storage buffer@ should
	 * match the number of registers described by the @bitmap field of
	 * struct csv_guest_user_rtmr_read. The @data field of struct
	 * csv_guest_user_rtmr_read is only used to store the value of the
	 * first RTMR register specified in the @bitmap field. If multiple RTMR
	 * register values need to be stored, the size of the @storage buffer@
	 * must be set to N times the size of the @data field in struct
	 * csv_guest_user_rtmr_read, and the @len field of struct csv_rtmr_req
	 * should be set to the size of the @storage buffer@ plus 4 (where 4 is
	 * the size of the @bitmap field in struct csv_guest_user_rtmr_read).
	 */
	cnt = 0;
	for (i = 0; i <= CSV_RTMR_REG_INDEX_MAX; i++) {
		if (read->bitmap & (1 << i))
			cnt++;
	}

	ret = -EINVAL;

	/* Return directly if no RTMR register is specified. */
	if (cnt == 0)
		goto err_free_read;

	if (req->len < (sizeof(read->bitmap) + cnt * sizeof(read->data))) {
		req->len = sizeof(read->bitmap) + cnt * sizeof(read->data);
		goto err_free_read;
	}

	/* Filter invalid bits in the @bitmap field. */
	read->bitmap &= (1UL << (CSV_RTMR_REG_INDEX_MAX + 1)) - 1;

	subcmd_len = sizeof(*subcmd) + (cnt - 1) * sizeof(subcmd->data);
	subcmd = kzalloc(subcmd_len, GFP_KERNEL);
	if (!subcmd) {
		ret = -ENOMEM;
		goto err_free_read;
	}

	hdr = &subcmd->hdr;
	hdr->subcmd_id = CSV3_SECURE_CMD_RTMR_READ;
	hdr->subcmd_size = subcmd_len;

	subcmd->rtmr_reg_bitmap = read->bitmap;

	ret = csv3_issue_request_rtmr((void *)subcmd, subcmd_len);
	if (ret) /* -ENODEV */
		goto err_free_subcmd;

	/*
	 * Copy the values of the requested RTMR registers from the
	 * subcommand's storage buffer.
	 */
	for (i = 0; i < cnt; i++) {
		memcpy(read->data + (i * sizeof(read->data)),
		       subcmd->data + (i * sizeof(subcmd->data)), sizeof(read->data));
	}

	/*
	 * The @bitmap of struct csv_guest_user_rtmr_read should indicate which
	 * RTMR register values are returned.
	 */
	read->bitmap = subcmd->rtmr_reg_bitmap;

	req->fw_error_code = hdr->fw_error_code;

	/* Copy the response to user space. */
	if (copy_to_user((void __user *)req->buf, read, req->len))
		ret = -EFAULT;

err_free_subcmd:
	kfree(subcmd);
err_free_read:
	kfree(read);

	return ret;
}

static int csv3_rtmr_extend(struct csv_rtmr_req *req)
{
	int ret = 0;
	struct csv_guest_user_rtmr_extend *extend = NULL;
	struct csv_rtmr_subcmd_extend *subcmd = NULL;
	struct csv_rtmr_subcmd_hdr *hdr = NULL;

	if (req->len < sizeof(*extend)) {
		req->len = sizeof(*extend);
		return -EINVAL;
	}

	/* The uapi must provide correct size of the extend data. */
	if (sizeof(extend->data) != sizeof(subcmd->data)) {
		pr_err("error: CSV RTMR extend data size confusion\n");
		return -EINVAL;
	}

	extend = kzalloc(req->len, GFP_KERNEL);
	if (!extend)
		return -ENOMEM;

	/* Copy user request params to the kernel space. */
	if (copy_from_user(extend, (void __user *)req->buf, req->len)) {
		ret = -EFAULT;
		goto err_free_extend;
	}

	ret = -EINVAL;

	/*
	 * In the post-OS stage, the requested index must be greater than 2. If
	 * the index is less than or equal to 2, return 3 to the user space. If
	 * the index exceeds the supported maximum index, return the maximum
	 * index supported in the guest kernel.
	 */
	if (extend->index <= 2) {
		extend->index = 3;
		goto err_copy_back;
	} else if (extend->index > CSV_RTMR_REG_INDEX_MAX) {
		extend->index = CSV_RTMR_REG_INDEX_MAX;
		goto err_copy_back;
	}

	/*
	 * The @data_len equals to the size of @data field of struct
	 * csv_guest_user_rtmr_extend.
	 */
	if (sizeof(extend->data) != extend->data_len)
		goto err_free_extend;

	subcmd = kzalloc(sizeof(*subcmd), GFP_KERNEL);
	if (!subcmd) {
		ret = -ENOMEM;
		goto err_free_extend;
	}

	hdr = &subcmd->hdr;
	hdr->subcmd_id = CSV3_SECURE_CMD_RTMR_EXTEND;
	hdr->subcmd_size = sizeof(*subcmd);

	subcmd->index = extend->index;
	subcmd->data_length = extend->data_len;
	memcpy(subcmd->data, extend->data, extend->data_len);

	ret = csv3_issue_request_rtmr((void *)subcmd, sizeof(*subcmd));
	if (ret) /* -ENODEV */
		goto err_free_subcmd;

	req->fw_error_code = hdr->fw_error_code;

err_copy_back:
	/* Copy the response to user space. */
	if (copy_to_user((void __user *)req->buf, extend, req->len))
		ret = -EFAULT;

err_free_subcmd:
	kfree(subcmd);
err_free_extend:
	kfree(extend);

	return ret;
}

static int csv3_rtmr(unsigned long arg)
{
	int ret = 0;
	struct csv_rtmr_req req;

	lockdep_assert_held(&csv_cmd_mutex);

	if (!csv3_active())
		return -ENOTTY;

	/* Copy user rtmr request to kernel space. */
	if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
		return -EFAULT;

	if (!req.buf)
		return -EINVAL;

	switch (req.subcmd_id) {
	case CSV_GUEST_USER_RTMR_STATUS:
		ret = csv3_rtmr_status(&req);
		break;
	case CSV_GUEST_USER_RTMR_START:
		ret = csv3_rtmr_start(&req);
		break;
	case CSV_GUEST_USER_RTMR_READ:
		ret = csv3_rtmr_read(&req);
		break;
	case CSV_GUEST_USER_RTMR_EXTEND:
		ret = csv3_rtmr_extend(&req);
		break;
	default:
		ret = -EINVAL;
		goto err;
	}

	/* Return directly if the RTMR is unsupported. */
	if (ret == -ENODEV)
		goto err;

	/* Copy kernel rtmr response to user space. */
	if (copy_to_user((void __user *)arg, &req, sizeof(req)))
		ret = -EFAULT;

err:
	return ret;
}

static long csv_guest_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -ENOTTY;

	mutex_lock(&csv_cmd_mutex);

	switch (cmd) {
	case CSV_CMD_GET_REPORT:
		ret = get_report(arg);
		break;
	case CSV_CMD_RTMR:
		ret = csv3_rtmr(arg);
		break;
	default:
		break;
	}

	mutex_unlock(&csv_cmd_mutex);

	return ret;
}

static const struct file_operations csv_guest_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = csv_guest_ioctl,
	.compat_ioctl = csv_guest_ioctl,
};

static struct miscdevice csv_guest_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "csv-guest",
	.fops = &csv_guest_fops,
	.mode = 0777,
};

static int __init csv_guest_init(void)
{
	// This module only working on CSV guest vm.
	if (!cc_platform_has(CC_ATTR_GUEST_MEM_ENCRYPT))
		return -ENODEV;

	return misc_register(&csv_guest_dev);
}

static void __exit csv_guest_exit(void)
{
	misc_deregister(&csv_guest_dev);
}

MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("HYGON CSV Guest Driver");
module_init(csv_guest_init);
module_exit(csv_guest_exit);
