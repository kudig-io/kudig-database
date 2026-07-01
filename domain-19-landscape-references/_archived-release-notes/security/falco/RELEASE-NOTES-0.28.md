---
title: falco v0.28 Release Notes
description: falco v0.28 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.28 Release Notes — Kubernetes 生产运维知识库
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
- falco v0.28 Release Notes 是什么
- 如何 falco v0.28 Release Notes
trigger_keywords:
- falco
- v0.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Falco|falco]] v0.28 Release Notes

Source: [0.28.1](https://github.com/falcosecurity/falco/releases/tag/0.28.1)

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm      | [![rpm](https://img.shields.io/badge/Falco-0.28.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/rpm/falco-0.28.1-x86_64.rpm)        |
| deb      | [![deb](https://img.shields.io/badge/Falco-0.28.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/deb/stable/falco-0.28.1-x86_64.deb) |
| tgz      | [![tgz](https://img.shields.io/badge/Falco-0.28.1-%2300aec7?style=flat-square)](https://download.falco.org/packages/bin/x86_64/falco-0.28.1-x86_64.tar.gz) |

| Images                                                                      |
| --------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.28.1`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.28.1`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.28.1`             |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.28.1`                 |

### Major Changes
* new: `--support` output now includes info about the Falco engine version [[#1581](https://github.com/falcosecurity/falco/pull/1581)] - [@mstemm](https://github.com/mstemm)
* new: Falco outputs an alert in the unlikely situation it's receiving too many consecutive timeouts without an event [[#1622](https://github.com/falcosecurity/falco/pull/1622)] - [@leodido](https://github.com/leodido)
* new: configuration field `syscall_event_timeouts.max_consecutive` to configure after how many consecutive timeouts without an event Falco must alert [[#1622](https://github.com/falcosecurity/falco/pull/1622)] - [@leodido](https://github.com/leodido)

### Minor Changes
* build: enforcing hardening flags by default [[#1604](https://github.com/falcosecurity/falco/pull/1604)] - [@leogr](https://github.com/leogr)

### Bug Fixes
* fix: do not stop the webserver for k8s audit logs when invalid data is coming in the event to be processed [[#1617](https://github.com/falcosecurity/falco/pull/1617)] - [@fntlnz](https://github.com/fntlnz)

### Rule Changes
* rule(macro: allowed_aws_ecr_registry_root_for_eks): new macro for AWS EKS images hosted on ECR to use in rule: Launch Privileged Container [[#1640](https://github.com/falcosecurity/falco/pull/1640)] - [@ismailyenigul](https://github.com/ismailyenigul)
* rule(macro: aws_eks_core_images): new macro for AWS EKS images hosted on ECR to use in rule: Launch Privileged Container [[#1640](https://github.com/falcosecurity/falco/pull/1640)] - [@ismailyenigul](https://github.com/ismailyenigul)
* rule(macro: aws_eks_image_sensitive_mount): new macro for AWS EKS images hosted on ECR to use in rule: Launch Privileged Container [[#1640](https://github.com/falcosecurity/falco/pull/1640)] - [@ismailyenigul](https://github.com/ismailyenigul)
* rule(list `falco_privileged_images`): remove deprecated Falco's OCI image repositories [[#1634](https://github.com/falcosecurity/falco/pull/1634)] - [@maxgio92](https://github.com/maxgio92)
* rule(list `falco_sensitive_mount_images`): remove deprecated Falco's OCI image repositories [[#1634](https://github.com/falcosecurity/falco/pull/1634)] - [@maxgio92](https://github.com/maxgio92)
* rule(macro `k8s_containers`): remove deprecated Falco's OCI image repositories [[#1634](https://github.com/falcosecurity/falco/pull/1634)] - [@maxgio92](https://github.com/maxgio92)
* rule(macro: python_running_sdchecks): macro removed [[#1620](https://github.com/falcosecurity/falco/pull/1620)] - [@leogr](https://github.com/leogr)
* rule(Change thread namespace): remove python_running_sdchecks exception [[#1620](https://github.com/falcosecurity/falco/pull/1620)] - [@leogr](https://github.com/leogr)

### Non user-facing changes
* urelease/docs: fix link and small refactor in the text [[#1636](https://github.com/falcosecurity/falco/pull/1636)] - [@cpanato](https://github.com/cpanato)
* Add Secureworks to adopters [[#1629](https://github.com/falcosecurity/falco/pull/1629)] - [@dwindsor-scwx](https://github.com/dwindsor-scwx)
* regression test for malformed k8s audit input (FAL-01-003) [[#1624](https://github.com/falcosecurity/falco/pull/1624)] - [@leodido](https://github.com/leodido)
* Add mathworks to adopterlist [[#1621](https://github.com/falcosecurity/falco/pull/1621)] - [@natchaphon-r](https://github.com/natchaphon-r)
* adding known users [[#1623](https://github.com/falcosecurity/falco/pull/1623)] - [@danpopSD](https://github.com/danpopSD)
* docs: update link for HackMD community call notes [[#1614](https://github.com/falcosecurity/falco/pull/1614)] - [@leodido](https://github.com/leodido)

### Statistics
| Merged PRs      | Number |
| --------------- | ------ |
| Not user-facing | 7      |
| Release note    | 7      |
| Total           | 14     |

\
**Release Manager** [@cpanato](https://github.com/cpanato)
