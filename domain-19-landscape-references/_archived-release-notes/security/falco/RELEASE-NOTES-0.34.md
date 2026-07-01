---
title: falco v0.34 Release Notes
description: falco v0.34 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.34 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.34 Release Notes 是什么
- 如何 falco v0.34 Release Notes
trigger_keywords:
- falco
- v0.34
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Falco|falco]] v0.34 Release Notes

Source: [0.34.1](https://github.com/falcosecurity/falco/releases/tag/0.34.1)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm-x86_64      | [![rpm](https://img.shields.io/badge/Falco-0.34.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.34.1-x86_64.rpm)        |
| deb-x86_64      | [![deb](https://img.shields.io/badge/Falco-0.34.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.34.1-x86_64.deb) |
| tgz-x86_64      | [![tgz](https://img.shields.io/badge/Falco-0.34.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.34.1-x86_64.tar.gz) |
| rpm-aarch64      | [![rpm](https://img.shields.io/badge/Falco-0.34.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.34.1-aarch64.rpm)        |
| deb-aarch64      | [![deb](https://img.shields.io/badge/Falco-0.34.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.34.1-aarch64.deb) |
| tgz-aarch64      | [![tgz](https://img.shields.io/badge/Falco-0.34.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/aarch64/falco-0.34.1-aarch64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.34.1`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.34.1`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.34.1`             |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.34.1`                 |
| `docker pull docker.io/falcosecurity/falcoctl:0.4.0`                 |

### Minor Changes

* fix(userspace/engine): correctly bump FALCO_ENGINE_VERSION after introduction of new fields [[#2418](https://github.com/falcosecurity/falco/pull/2418)] - [@loresuso](https://github.com/loresuso/)

### Statistics

| Merged PRs      | Number |
| --------------- | ------ |
| Not user-facing | 1      |
| Release note    | 1      |
| Total           | 2      |

#### Release Manager

@alacuku 