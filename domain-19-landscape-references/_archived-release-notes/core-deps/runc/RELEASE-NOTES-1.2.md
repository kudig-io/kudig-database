---
title: runc v1.2 Release Notes
description: runc v1.2 Release Notes — Kubernetes 生产运维知识库
summary: runc v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- runc v1.2 Release Notes 是什么
- 如何 runc v1.2 Release Notes
trigger_keywords:
- runc
- v1.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# runc v1.2 Release Notes

Source: [v1.2.9](https://github.com/opencontainers/runc/releases/tag/v1.2.9)

This is the ninth patch release of the 1.2.z release series of runc, and
primarily contains a few fixes for some regressions introduced in 1.2.8.


### Fixed
 * libct: fix mips compilation. (#4962, #4965)
 * When configuring a `tmpfs` mount, only set the `mode=` argument if the
   target path already existed. This fixes a regression introduced in our
   [CVE-2025-52881][] mitigation patches. (#4971, #4974)
 * Fix various file descriptor leaks and add additional tests to detect them as
   comprehensively as possible. (#5007, #5021, #5027)

### Changed
 * Downgrade `github.com/cyphar/filepath-securejoin` dependency to `v0.5.2`,
   which should make it easier for some downstreams to import `runc` without
   pulling in too many extra packages. (#5027)

[CVE-2025-52881]: https://github.com/opencontainers/runc/security/advisories/GHSA-cgrx-mc8f-2prm


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

However we strongly suggest that you make use of your [[Distribution|distribution]]'s packages
or download them from the authoritative upstream sources, especially since
these libraries are related to the security of your containers.

<hr>

Thanks to the following contributors for making this release possible:

 * Aleksa Sarai <cyphar@cyphar.com>
 * Kir Kolyshkin <kolyshkin@gmail.com>
 * Li Fu Bang <lifubang@acmcoder.com>
 * Tianon Gravi <admwiggin@gmail.com>

Signed-off-by: Aleksa Sarai <cyphar@cyphar.com>