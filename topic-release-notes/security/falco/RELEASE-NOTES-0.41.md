---
title: falco v0.41 Release Notes
description: falco v0.41 Release Notes — Kubernetes 生产运维知识库
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
- falco v0.41 Release Notes 是什么
- 如何 falco v0.41 Release Notes
trigger_keywords:
- falco
- v0.41
- Release
- Notes
- release
- notes
---

# falco v0.41 Release Notes

Source: [0.41.3](https://github.com/falcosecurity/falco/releases/tag/0.41.3)

[![LIBS](https://img.shields.io/badge/LIBS-0.21.0-yellow)](https://github.com/falcosecurity/libs/releases/tag/0.21.0)
[![DRIVER](https://img.shields.io/badge/DRIVER-8.1.0+driver-yellow)](https://github.com/falcosecurity/libs/releases/tag/8.1.0+driver)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm-x86_64      | [![rpm](https://img.shields.io/badge/Falco-0.41.3-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.41.3-x86_64.rpm)        |
| deb-x86_64      | [![deb](https://img.shields.io/badge/Falco-0.41.3-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.41.3-x86_64.deb) |
| tgz-x86_64      | [![tgz](https://img.shields.io/badge/Falco-0.41.3-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.41.3-x86_64.tar.gz) |
| tgz-static-x86_64      | [![tgz-static](https://img.shields.io/badge/Falco-0.41.3-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.41.3-static-x86_64.tar.gz) |
| rpm-aarch64      | [![rpm](https://img.shields.io/badge/Falco-0.41.3-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.41.3-aarch64.rpm)        |
| deb-aarch64      | [![deb](https://img.shields.io/badge/Falco-0.41.3-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.41.3-aarch64.deb) |
| tgz-aarch64      | [![tgz](https://img.shields.io/badge/Falco-0.41.3-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/aarch64/falco-0.41.3-aarch64.tar.gz) |

| Images                                                                    |
|---------------------------------------------------------------------------|
| `docker pull docker.io/falcosecurity/falco:0.41.3`                      |
| `docker pull public.ecr.aws/falcosecurity/falco:0.41.3`                 |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.41.3`        |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.41.3-buster` |
| `docker pull docker.io/falcosecurity/falco:0.41.3-debian`               |

## v0.41.3

### Minor Changes

* update: bump container plugin to v0.3.1 [[#3629](https://github.com/falcosecurity/falco/pull/3629)] - [@FedeDP](https://github.com/FedeDP)




### Statistics

|   MERGED PRS    | NUMBER |
|-----------------|--------|
| Not user-facing |      0 |
| Release note    |      1 |
| Total           |      1 |

#### Release Manager @leogr @ekoops 
