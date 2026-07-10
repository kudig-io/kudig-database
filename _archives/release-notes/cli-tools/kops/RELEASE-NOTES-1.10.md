---
title: kops v1.10 Release Notes
description: kops v1.10 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.10 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.10 Release Notes 是什么
- 如何 kops v1.10 Release Notes
trigger_keywords:
- kops
- v1.10
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




# kops v1.10 Release Notes

Source: [1.10.1](https://github.[[entities/kubernetes.md|kubernetes]]/kops/releases/tag/1.10.1)

Minor update for the kops 1.10 series.

Changes since 1.10:

* Upgrade DigitalOcean CCM to v0.1.7 [@andrewsykim](https://github.com/andrewsykim) [#5651](https://github.com/kubernetes/kops/pull/5651)
* amazon-vpc-routed-eni cloudprovider check [@mikesplain](https://github.com/mikesplain) [#5540](https://github.com/kubernetes/kops/pull/5540)
* Load client-auth plugins [@ripta](https://github.com/ripta) [#5513](https://github.com/kubernetes/kops/pull/5513)
* add kube-proxy hostname override [@andrewsykim](https://github.com/andrewsykim) [#5649](https://github.com/kubernetes/kops/pull/5649)


<!-- risk-assessed -->
