---
title: falco v0.38 Release Notes
description: falco v0.38 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.38 Release Notes — Kubernetes 生产运维知识库
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
- falco v0.38 Release Notes 是什么
- 如何 falco v0.38 Release Notes
trigger_keywords:
- falco
- v0.38
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Falco|falco]] v0.38 Release Notes

Source: [0.38.2](https://github.com/falcosecurity/falco/releases/tag/0.38.2)

[![LIBS](https://img.shields.io/badge/LIBS-0.17.3-yellow)](https://github.com/falcosecurity/libs/releases/tag/0.17.3)
[![DRIVER](https://img.shields.io/badge/DRIVER-7.2.1+driver-yellow)](https://github.com/falcosecurity/libs/releases/tag/7.2.1+driver)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm-x86_64      | [![rpm](https://img.shields.io/badge/Falco-0.38.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.38.2-x86_64.rpm)        |
| deb-x86_64      | [![deb](https://img.shields.io/badge/Falco-0.38.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.38.2-x86_64.deb) |
| tgz-x86_64      | [![tgz](https://img.shields.io/badge/Falco-0.38.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.38.2-x86_64.tar.gz) |
| rpm-aarch64      | [![rpm](https://img.shields.io/badge/Falco-0.38.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.38.2-aarch64.rpm)        |
| deb-aarch64      | [![deb](https://img.shields.io/badge/Falco-0.38.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.38.2-aarch64.deb) |
| tgz-aarch64      | [![tgz](https://img.shields.io/badge/Falco-0.38.2-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/aarch64/falco-0.38.2-aarch64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.38.2`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.38.2`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.38.2`             |
| `docker pull docker.io/falcosecurity/falco-driver-loader-legacy:0.38.2`      |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.38.2`                 |
| `docker pull docker.io/falcosecurity/falco-distroless:0.38.2`                |

## v0.38.2

Released on 2024-08-19




### Bug Fixes

* fix(engine): fix metrics names to better adhere to best practices [[#3272](https://github.com/falcosecurity/falco/pull/3272)] - [@incertum](https://github.com/incertum)
* fix(ci): use vault.centos.org for centos:7 CI build. [[#3274](https://github.com/falcosecurity/falco/pull/3274)] - [@FedeDP](https://github.com/FedeDP)



### Statistics

|   MERGED PRS    | NUMBER |
|-----------------|--------|
| Not user-facing |      0 |
| Release note    |      2 |
| Total           |      2 |

#### Release Manager @LucaGuerra


<!-- risk-assessed -->
