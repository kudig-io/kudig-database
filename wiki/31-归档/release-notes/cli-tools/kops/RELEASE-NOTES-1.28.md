---
title: kops v1.28 Release Notes
description: kops v1.28 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.28 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- controller-manager
- calico
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.28 Release Notes 是什么
- 如何 kops v1.28 Release Notes
trigger_keywords:
- kops
- v1.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kops v1.28 Release Notes

Source: [v1.28.7](https://github.com/kubernetes/kops/releases/tag/v1.28.7)

(Note that v1.28.6 was not released, due to a problem in the release process)

## What's Changed (since v1.28.5)

* Support definition of kube-controller-manager by @chubchubsancho in https://github.com/kubernetes/kops/pull/16609
* Update Calico to v3.27.3 by @rifelpet in https://github.com/kubernetes/kops/pull/16613
* Update golang to 1.22.5 by @justinsb in https://github.com/kubernetes/kops/pull/16652
* Create a dedicated staging bucket for kops builds by @justinsb in https://github.com/kubernetes/kops/pull/16678
* Fix cluster-autoscaler priority expander config by @rifelpet in https://github.com/kubernetes/kops/pull/16673
* Bump cloudbuild to go 1.22.5 by @justinsb in https://github.com/kubernetes/kops/pull/16683


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.28.5...v1.28.7

<!-- risk-assessed -->
