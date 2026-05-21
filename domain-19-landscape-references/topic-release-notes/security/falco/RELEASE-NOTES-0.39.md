---
title: falco v0.39 Release Notes
description: falco v0.39 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.39 Release Notes 是什么
- 如何 falco v0.39 Release Notes
trigger_keywords:
- falco
- v0.39
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# falco v0.39 Release Notes

Source: [0.39.2](https://github.com/falcosecurity/falco/releases/tag/0.39.2)

[![LIBS](https://img.shields.io/badge/LIBS-0.18.2-yellow)](https://github.com/falcosecurity/libs/releases/tag/0.18.2)
[![DRIVER](https://img.shields.io/badge/DRIVER-7.3.0+driver-yellow)](https://github.com/falcosecurity/libs/releases/tag/7.3.0+driver)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm-x86_64      | [![rpm](https://img.shields.io/badge/Falco-0.39.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.39.2-x86_64.rpm)        |
| deb-x86_64      | [![deb](https://img.shields.io/badge/Falco-0.39.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.39.2-x86_64.deb) |
| tgz-x86_64      | [![tgz](https://img.shields.io/badge/Falco-0.39.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.39.2-x86_64.tar.gz) |
| rpm-aarch64      | [![rpm](https://img.shields.io/badge/Falco-0.39.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.39.2-aarch64.rpm)        |
| deb-aarch64      | [![deb](https://img.shields.io/badge/Falco-0.39.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.39.2-aarch64.deb) |
| tgz-aarch64      | [![tgz](https://img.shields.io/badge/Falco-0.39.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/aarch64/falco-0.39.2-aarch64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.39.2`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.39.2`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.39.2`             |
| `docker pull docker.io/falcosecurity/falco-driver-loader-legacy:0.39.2`      |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.39.2`                 |
| `docker pull docker.io/falcosecurity/falco-distroless:0.39.2`                |

## v0.39.2

Released on 2024-11-21



### Minor Changes

* update(cmake): bumped falcoctl to v0.10.1. [[#3408](https://github.com/falcosecurity/falco/pull/3408)] - [@FedeDP](https://github.com/FedeDP)
* update(cmake): bump yaml-cpp to latest master. [[#3394](https://github.com/falcosecurity/falco/pull/3394)] - [@FedeDP](https://github.com/FedeDP)




### Non user-facing changes

* update(ci): use arm64 CNCF runners for GH actions [[#3386](https://github.com/falcosecurity/falco/pull/3386)] - [@LucaGuerra](https://github.com/LucaGuerra)

### Statistics

|   MERGED PRS    | NUMBER |
|-----------------|--------|
| Not user-facing |      1 |
| Release note    |      2 |
| Total           |      3 |

#### Release Manager @FedeDP
