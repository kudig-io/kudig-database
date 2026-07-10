// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (c) 2026, Google LLC.
 * Samiullah Khawaja <skhawaja@google.com>
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>

#define __EXPORTED_HEADERS__
#include <uapi/linux/types.h>
#include <linux/iommufd.h>
#include <linux/types.h>
#include <linux/vfio.h>
#include <linux/sizes.h>
#include <libliveupdate.h>

#include "../kselftest.h"

#define ksft_assert(condition) \
	do { \
		if (!(condition)) \
			fail_exit("Failed: %s", #condition); \
	} while (0)

static const char *device_cdev_path;
static char state_session[LIVEUPDATE_SESSION_NAME_LENGTH];
static char iommufd_session[LIVEUPDATE_SESSION_NAME_LENGTH];

static const uint64_t STATE_TOKEN;
static const uint64_t IOMMUFD_TOKEN = 0x123456;
static const uint64_t CDEV_TOKEN = 0x654321;
static const uint64_t HWPT_TOKEN = 0x789012;
static const uint64_t MEMFD_TOKEN = 0x890123;

static int open_cdev(const char *vfio_cdev_path)
{
	int cdev_fd;

	cdev_fd = open(vfio_cdev_path, O_RDWR);
	if (cdev_fd < 0)
		ksft_exit_skip("Failed to open VFIO cdev: %s\n", vfio_cdev_path);

	return cdev_fd;
}

static int open_iommufd(void)
{
	int iommufd;

	iommufd = open("/dev/iommu", O_RDWR);
	if (iommufd < 0)
		ksft_exit_skip("Failed to open /dev/iommu. IOMMUFD support not enabled.\n");

	return iommufd;
}

static int create_sealed_memfd(size_t size)
{
	int fd, ret;

	fd = memfd_create("buffer", MFD_ALLOW_SEALING);
	if (fd < 0)
		fail_exit("memfd_create failed");

	ret = ftruncate(fd, size);
	if (ret)
		fail_exit("ftruncate failed");

	ret = fcntl(fd, F_ADD_SEALS, F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL);
	if (ret)
		fail_exit("fcntl F_ADD_SEALS failed");

	return fd;
}

#define test_ioctl(fd, cmd, arg) \
	do { \
		if (ioctl(fd, cmd, arg)) \
			fail_exit("ioctl(%s) failed", #cmd); \
	} while (0)

#define test_luo_session_preserve_fd(session, fd, token) \
	do { \
		if (luo_session_preserve_fd(session, fd, token)) \
			fail_exit("luo_session_preserve_fd(%s) failed", #token); \
	} while (0)

#define test_luo_session_retrieve_fd(session, token) \
	({ \
		int _fd = luo_session_retrieve_fd(session, token); \
		if (_fd < 0) \
			fail_exit("luo_session_retrieve_fd(%s) failed", #token); \
		_fd; \
	})

static void setup_iommufd(int iommufd, int memfd, int cdev_fd)
{
	struct vfio_device_bind_iommufd bind = {
		.argsz = sizeof(bind),
		.flags = 0,
		.iommufd = iommufd,
	};
	struct iommu_ioas_alloc alloc_data = {
		.size = sizeof(alloc_data),
		.flags = 0,
	};
	struct iommu_hwpt_alloc hwpt_alloc = {
		.size = sizeof(hwpt_alloc),
		.flags = 0,
	};
	struct vfio_device_attach_iommufd_pt attach_data = {
		.argsz = sizeof(attach_data),
		.flags = 0,
	};
	struct iommu_hwpt_liveupdate_mark_preserve mark_preserve = {
		.size = sizeof(mark_preserve),
		.hwpt_token = HWPT_TOKEN,
	};
	struct iommu_ioas_map_file map_file = {
		.size = sizeof(map_file),
		.length = SZ_1M,
		.flags = IOMMU_IOAS_MAP_WRITEABLE | IOMMU_IOAS_MAP_READABLE,
		.iova = SZ_4G,
		.fd = memfd,
		.start = 0,
	};

	test_ioctl(cdev_fd, VFIO_DEVICE_BIND_IOMMUFD, &bind);

	test_ioctl(iommufd, IOMMU_IOAS_ALLOC, &alloc_data);

	hwpt_alloc.dev_id = bind.out_devid;
	hwpt_alloc.pt_id = alloc_data.out_ioas_id;
	test_ioctl(iommufd, IOMMU_HWPT_ALLOC, &hwpt_alloc);

	attach_data.pt_id = hwpt_alloc.out_hwpt_id;
	test_ioctl(cdev_fd, VFIO_DEVICE_ATTACH_IOMMUFD_PT, &attach_data);

	map_file.ioas_id = alloc_data.out_ioas_id;
	test_ioctl(iommufd, IOMMU_IOAS_MAP_FILE, &map_file);

	mark_preserve.hwpt_id = attach_data.pt_id;
	test_ioctl(iommufd, IOMMU_HWPT_LIVEUPDATE_MARK_PRESERVE, &mark_preserve);
}

static void before_kexec(int luo_fd)
{
	int iommufd, cdev_fd, memfd, session;

	create_state_file(luo_fd, state_session, STATE_TOKEN, /*next_stage=*/2);

	session = luo_create_session(luo_fd, iommufd_session);
	if (session < 0)
		fail_exit("luo_create_session failed");

	iommufd = open_iommufd();
	memfd = create_sealed_memfd(SZ_1M);
	cdev_fd = open_cdev(device_cdev_path);

	setup_iommufd(iommufd, memfd, cdev_fd);

	/* Cannot preserve cdev without iommufd */
	if (!luo_session_preserve_fd(session, cdev_fd, CDEV_TOKEN))
		fail_exit("Preserving cdev without iommufd should fail");

	/* Cannot preserve iommufd without preserving memfd. */
	if (!luo_session_preserve_fd(session, iommufd, IOMMUFD_TOKEN))
		fail_exit("Preserving iommufd without memfd should fail");

	test_luo_session_preserve_fd(session, memfd, MEMFD_TOKEN);
	test_luo_session_preserve_fd(session, iommufd, IOMMUFD_TOKEN);
	test_luo_session_preserve_fd(session, cdev_fd, CDEV_TOKEN);

	close(session);
	session = luo_create_session(luo_fd, iommufd_session);
	if (session < 0)
		fail_exit("luo_create_session failed");

	test_luo_session_preserve_fd(session, memfd, MEMFD_TOKEN);
	test_luo_session_preserve_fd(session, iommufd, IOMMUFD_TOKEN);
	test_luo_session_preserve_fd(session, cdev_fd, CDEV_TOKEN);

	close(luo_fd);
	daemonize_and_wait();
}

static void after_kexec(int luo_fd, int state_session_fd)
{
	int iommufd, cdev_fd, session, stage;
	struct vfio_device_bind_iommufd bind = {
		.argsz = sizeof(bind),
		.flags = 0,
	};

	restore_and_read_stage(state_session_fd, STATE_TOKEN, &stage);
	ksft_assert(stage == 2);

	session = luo_retrieve_session(luo_fd, iommufd_session);
	if (session < 0)
		fail_exit("luo_retrieve_session failed");

	cdev_fd = test_luo_session_retrieve_fd(session, CDEV_TOKEN);

	iommufd = luo_session_retrieve_fd(session, IOMMUFD_TOKEN);
	if (iommufd >= 0)
		fail_exit("iommufd should not be retrievable yet");

	iommufd = open_iommufd();

	bind.iommufd = iommufd;
	if (ioctl(cdev_fd, VFIO_DEVICE_BIND_IOMMUFD, &bind) == 0 || errno != EPERM)
		fail_exit("Binding cdev to new iommufd should fail with EPERM");

	/* Should fail */
	if (luo_session_finish(session) == 0)
		fail_exit("luo_session_finish should fail if iommufd is not restored");

	close(iommufd);
	close(cdev_fd);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <vfio_cdev_path>\n", argv[0]);
		return 1;
	}

	device_cdev_path = argv[1];
	sprintf(iommufd_session, "iommufd-test-%s", "cdev");
	sprintf(state_session, "state-%s", "iommufd-cdev");

	return luo_test(argc, argv, state_session, before_kexec, after_kexec);
}
