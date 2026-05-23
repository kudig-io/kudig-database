---
title: longhorn v0.3 Release Notes
description: longhorn v0.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- longhorn v0.3 Release Notes 是什么
- 如何 longhorn v0.3 Release Notes
trigger_keywords:
- longhorn
- v0.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Longhorn|longhorn]] v0.3 Release Notes

Source: [v0.3.3](https://github.com/longhorn/longhorn/releases/tag/v0.3.3)

Engine image: rancher/longhorn-engine:v0.3.3
Manager image: rancher/longhorn-manager:v0.3.3
UI Image: rancher/longhorn-ui:v0.3.3

New features:

1. #299 Support set the default number of replicas (replica count) and dynamic adjust replica count of an attached volume.
2. Live upgrade for Engine image became beta. Upgrade from Engine v0.3.0 to v0.3.3 can be done without volume downtime (except if you need the fix for #324 2TB volume size limitation)

Notice Longhorn Engine has been updated to v0.3.3. In addition to upgrade manager, please follow the steps [here](https://github.com/rancher/longhorn/blob/master/docs/upgrade.md#upgrade-engine-images) to upgrade engines.

See [here](https://github.com/rancher/longhorn/milestone/6?closed=1) for the list of bugs fixed.