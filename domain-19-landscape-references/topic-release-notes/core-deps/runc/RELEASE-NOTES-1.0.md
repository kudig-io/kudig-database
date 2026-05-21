---
title: runc v1.0 Release Notes
description: runc v1.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- runc v1.0 Release Notes 是什么
- 如何 runc v1.0 Release Notes
trigger_keywords:
- runc
- v1.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

# runc v1.0 Release Notes

Source: [v1.0.3](https://github.com/opencontainers/runc/releases/tag/v1.0.3)

This is the third stable release in the 1.0 branch, fixing a handful of medium
priority issues related to mounts and cgroups, as well as a potential security
vulnerability.

This release is expected to be the last point release in the 1.0 branch, as we
are planning to release runc 1.1 in the near future.

Security:
 * A potential vulnerability was discovered in runc (related to an internal
   usage of netlink), however upon further investigation we discovered that
   while this bug was exploitable on the master branch of runc, no released
   version of runc could be exploited using this bug. The exploit required
   being able to create a netlink attribute with a length that would overflow a
   uint16 but this was not possible in any released version of runc. For more
   information, see [GHSA-v95c-p5hm-xq8f](https://github.com/opencontainers/runc/security/advisories/GHSA-v95c-p5hm-xq8f) and CVE-2021-43784.

   Due to an abundance of caution we decided to do an emergency release with
   this fix, but to reiterate *we do not believe this vulnerability was
   possible to exploit*. Thanks to Felix Wilhelm from Google Project Zero for
   discovering and reporting this vulnerability so quickly.

Bugfixes:
 * Fixed inability to start a container with read-write bind mount of a
   read-only fuse host mount (#3292)
 * Fixed inability to start when read-only /dev in set in spec (#3277)
 * Fixed not removing sub-cgroups upon container delete, when rootless cgroup v2
   is used with older systemd (#3297)
 * Fixed returning error from GetStats when hugetlb is unsupported (which causes
   excessive logging for kubernetes) (#3295)
 * [CI only] Fixed criu 3.16 compatibility issue (#3282)
 * [CI only] Add Go 1.17 to the testing matrix (#3299)

Enhancements:
 * Improved an error message when dbus-user-session is not installed and
   rootless + cgroup2 + systemd are used (#3212)

### Static Linking Notices ###

The `runc` binary distributed with this release are *statically linked* with
the following [GNU LGPL-2.1][lgpl-2.1] licensed libraries, with `runc` acting
as a "work that uses the Library":

[lgpl-2.1]: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html

  - [libseccomp](https://github.com/seccomp/libseccomp)

The versions of these libraries were not modified from their upstream versions,
but in order to comply with the LGPL-2.1 (&sect;6(a)), we have attached the
complete source code for those libraries which (when combined with the attached
runc source code) may be used to exercise your rights under the LGPL-2.1.

However we strongly suggest that you make use of your distribution's packages
or download them from the authoritative upstream sources, especially since
these libraries are related to the security of your containers.

<hr/>

Thanks to all of the contributors who made this release possible:

 * Akihiro Suda <akihiro.suda.cz@hco.ntt.co.jp>
 * Aleksa Sarai <cyphar@cyphar.com>
 * Kailun Qin <kailun.qin@intel.com>
 * Kang Chen <kongchen28@gmail.com>
 * Kir Kolyshkin <kolyshkin@gmail.com>
 * Odin Ugedal <odin@uged.al>
 * Sebastiaan van Stijn <thaJeztah@users.noreply.github.com>

Signed-off-by: Aleksa Sarai <cyphar@cyphar.com>