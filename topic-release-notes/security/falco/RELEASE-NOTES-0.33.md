---
title: falco v0.33 Release Notes
description: falco v0.33 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.33 Release Notes 是什么
- 如何 falco v0.33 Release Notes
trigger_keywords:
- falco
- v0.33
- Release
- Notes
- release
- notes
---

# falco v0.33 Release Notes

Source: [0.33.1](https://github.com/falcosecurity/falco/releases/tag/0.33.1)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm-x86_64      | [![rpm](https://img.shields.io/badge/Falco-0.33.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.33.1-x86_64.rpm)        |
| deb-x86_64      | [![deb](https://img.shields.io/badge/Falco-0.33.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.33.1-x86_64.deb) |
| tgz-x86_64      | [![tgz](https://img.shields.io/badge/Falco-0.33.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.33.1-x86_64.tar.gz) |
| rpm-aarch64      | [![rpm](https://img.shields.io/badge/Falco-0.33.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.33.1-aarch64.rpm)        |
| deb-aarch64      | [![deb](https://img.shields.io/badge/Falco-0.33.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.33.1-aarch64.deb) |
| tgz-aarch64      | [![tgz](https://img.shields.io/badge/Falco-0.33.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/aarch64/falco-0.33.1-aarch64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.33.1`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.33.1`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.33.1`             |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.33.1`                 |

### Minor Changes

* update(falco): fix container-gvisor and kubernetes-gvisor print options [[#2288](https://github.com/falcosecurity/falco/pull/2288)]
* Update libs to 0.9.2, fixing potential CLBO on gVisor+Kubernetes and crash with eBPF when some CPUs are offline [[#2299](https://github.com/falcosecurity/falco/pull/2299)] - [@LucaGuerra](https://github.com/LucaGuerra)

### Statistics

| Merged PRs      | Number |
| --------------- | ------ |
| Not user-facing | 1      |
| Release note    | 2      |
| Total           | 3      |

#### Release Manager

@LucaGuerra