---
title: kops v1.23 Release Notes
description: kops v1.23 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.23 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- controller-manager
- cilium
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.23 Release Notes 是什么
- 如何 kops v1.23 Release Notes
trigger_keywords:
- kops
- v1.23
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kops v1.23 Release Notes

Source: [v1.23.4](https://github.com/kubernetes/kops/releases/tag/v1.23.4)

## What's Changed
* Automated cherry pick of #14081: aws-ebs-csi-driver: remove preStop hook by @hakman in https://github.com/kubernetes/kops/pull/14086
* [[Cilium|cilium]]: fix wrong pod annotations templating #1.23 by @sterchelen in https://github.com/kubernetes/kops/pull/14105
* Automated cherry pick of #14115: Disable some flags in kube-controller-manager and by @hakman in https://github.com/kubernetes/kops/pull/14120
* Automated cherry pick of #14188: Update runc to v1.1.4 by @hakman in https://github.com/kubernetes/kops/pull/14197
* Release 1.23.4 by @justinsb in https://github.com/kubernetes/kops/pull/14220


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.23.3...v1.23.4

<!-- risk-assessed -->
