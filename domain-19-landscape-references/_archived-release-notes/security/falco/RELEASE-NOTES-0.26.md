---
title: falco v0.26 Release Notes
description: falco v0.26 Release Notes — Kubernetes 生产运维知识库
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
- falco v0.26 Release Notes 是什么
- 如何 falco v0.26 Release Notes
trigger_keywords:
- falco
- v0.26
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Falco|falco]] v0.26 Release Notes

Source: [0.26.2](https://github.com/falcosecurity/falco/releases/tag/0.26.2)

Released on 2020-10-01

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm      | [![rpm](https://img.shields.io/badge/Falco-0.26.2-%2300aec7?style=flat-square)](https://dl.bintray.com/falcosecurity/rpm/falco-0.26.2-x86_64.rpm)        |
| deb      | [![deb](https://img.shields.io/badge/Falco-0.26.2-%2300aec7?style=flat-square)](https://dl.bintray.com/falcosecurity/deb/stable/falco-0.26.2-x86_64.deb) |
| tgz      | [![tgz](https://img.shields.io/badge/Falco-0.26.2-%2300aec7?style=flat-square)](https://dl.bintray.com/falcosecurity/bin/x86_64/falco-0.26.2-x86_64.deb) |

| Images                                                          |
| --------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.26.2`               |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.26.2` |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.26.2`     |

### Major Changes

- update: DRIVERS_REPO now defaults to https://download.falco.org/driver [[#1460](https://github.com/falcosecurity/falco/pull/1460)] - [@leodido](https://github.com/leodido)