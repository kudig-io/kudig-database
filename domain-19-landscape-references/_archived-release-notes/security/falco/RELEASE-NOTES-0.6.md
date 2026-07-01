---
title: falco v0.6 Release Notes
description: falco v0.6 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- falco v0.6 Release Notes 是什么
- 如何 falco v0.6 Release Notes
trigger_keywords:
- falco
- v0.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Falco|falco]] v0.6 Release Notes

Source: [0.6.1](https://github.com/falcosecurity/falco/releases/tag/0.6.1)

Released 2016-05-15

### Major Changes

None

### Minor Changes

* Small changes to token bucket used to throttle falco events [[#234](https://github.com/draios/falco/pull/234)] [[#235](https://github.com/draios/falco/pull/235)] [[#236](https://github.com/draios/falco/pull/236)] [[#238](https://github.com/draios/falco/pull/238)]

### Bug Fixes

* Update the falco driver to work with kernel 4.11 [[#829](https://github.com/draios/sysdig/pull/829)]

### Rule Changes

* Don't allow apache2 to spawn shells in containers [[#231](https://github.com/draios/falco/issues/231)] [[#232](https://github.com/draios/falco/pull/232)]