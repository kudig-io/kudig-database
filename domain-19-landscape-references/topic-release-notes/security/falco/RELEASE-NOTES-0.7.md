---
title: falco v0.7 Release Notes
description: falco v0.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- falco
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.7 Release Notes 是什么
- 如何 falco v0.7 Release Notes
trigger_keywords:
- falco
- v0.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# falco v0.7 Release Notes

Source: [0.7.0](https://github.com/falcosecurity/falco/releases/tag/0.7.0)

Released 2016-05-30

### Major Changes

* Update the priorities of falco rules to use a wider range of priorities rather than just ERROR/WARNING. More info on the use of priorities in the ruleset can be found [here](https://github.com/draios/falco/wiki/Falco-Rules#rule-priorities). [[#244](https://github.com/draios/falco/pull/244)]

### Minor Changes

None.

### Bug Fixes

* Fix typos in various markdown files. Thanks @sublimino! [[#241](https://github.com/draios/falco/pull/241)]

### Rule Changes

* Add gitlab-mon as a gitlab binary, which allows it to run shells, etc. Thanks @dkerwin! [[#237](https://github.com/draios/falco/pull/237)]
* A new rule Terminal shell in container" that looks for shells spawned in a container with an attached terminal. [[#242](https://github.com/draios/falco/pull/242)]
* Fix some FPs related to the sysdig monitor agent. [[#243](https://github.com/draios/falco/pull/243)]
* Fix some FPs related to stating containers combined with missed events [[#243](https://github.com/draios/falco/pull/243)]